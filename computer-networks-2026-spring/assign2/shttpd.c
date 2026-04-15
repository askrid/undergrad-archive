#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include "macro.h"

#define MAX_URL     1024
#define MAX_EVENTS  64
#define MAX_FD      65536
#define SENDFILE_CHUNK (1 << 20) /* 1 MB */

static const char *g_rootDir = "./";

/* ---------- per-connection state machine ---------- */

enum conn_state {
    STATE_READ_REQ,
    STATE_SEND_HDR,
    STATE_SEND_BODY,
};

typedef struct {
    int fd;
    enum conn_state state;

    /* request accumulation (+1 for null terminator) */
    char req_buf[MAX_HDR + 1];
    int req_len;

    /* parsed fields */
    char url[MAX_URL];
    int keep_alive;

    /* response header */
    char resp_hdr[MAX_HDR];
    int resp_hdr_len;
    int resp_hdr_sent;

    /* file body */
    int file_fd;
    off_t file_size;
    off_t file_sent;
} conn_t;

static conn_t *conns[MAX_FD];
static int g_epfd;
static int g_listenfd;

/* ---------- helpers ---------- */

static void
set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static conn_t *
conn_new(int fd)
{
    conn_t *c = calloc(1, sizeof(*c));
    if (!c)
        return NULL;
    c->fd = fd;
    c->state = STATE_READ_REQ;
    c->file_fd = -1;
    conns[fd] = c;
    return c;
}

static void
conn_reset(conn_t *c)
{
    if (c->file_fd >= 0) {
        close(c->file_fd);
        c->file_fd = -1;
    }
    c->state = STATE_READ_REQ;
    c->req_len = 0;
    c->url[0] = '\0';
    c->resp_hdr_len = 0;
    c->resp_hdr_sent = 0;
    c->file_size = 0;
    c->file_sent = 0;
}

static void
conn_free(conn_t *c)
{
    if (!c)
        return;
    epoll_ctl(g_epfd, EPOLL_CTL_DEL, c->fd, NULL);
    close(c->fd);
    if (c->file_fd >= 0)
        close(c->file_fd);
    conns[c->fd] = NULL;
    free(c);
}

static void
epoll_mod(int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events | EPOLLET;
    ev.data.fd = fd;
    epoll_ctl(g_epfd, EPOLL_CTL_MOD, fd, &ev);
}

/* ---------- HTTP parsing ---------- */

/* case-insensitive header name match (RFC 1945: field names are case-insensitive) */
static int
header_match(const char *line, const char *name)
{
    size_t nlen = strlen(name);
    if (strncasecmp(line, name, nlen) != 0)
        return 0;
    if (line[nlen] != ':')
        return 0;
    return 1;
}

/*
 * parse_request: 0 = incomplete, 1 = ok, -1 = malformed
 */
static int
parse_request(conn_t *c)
{
    char *end = memmem(c->req_buf, c->req_len, "\r\n\r\n", 4);
    if (!end) {
        if (c->req_len >= MAX_HDR)
            return -1; /* header too large */
        return 0; /* need more data */
    }

    /* null-terminate for string ops (safe: end+4 <= req_buf+MAX_HDR) */
    *(end + 4) = '\0';

    char *line = c->req_buf;
    char *crlf = strstr(line, "\r\n");
    if (!crlf)
        return -1;

    /* parse request line: GET <url> HTTP/1.x (one or more spaces between tokens) */
    if (strncmp(line, "GET", 3) != 0 || (line[3] != ' ' && line[3] != '\t'))
        return -1;

    char *url_start = line + 4;
    while (*url_start == ' ' || *url_start == '\t')
        url_start++;
    if (*url_start != '/')
        return -1;

    char *url_end = url_start + 1;
    while (*url_end != ' ' && *url_end != '\t' && url_end < crlf)
        url_end++;
    if (url_end >= crlf)
        return -1;

    size_t url_len = url_end - url_start;
    if (url_len >= MAX_URL)
        return -1;
    memcpy(c->url, url_start, url_len);
    c->url[url_len] = '\0';

    char *ver = url_end + 1;
    while (*ver == ' ' || *ver == '\t')
        ver++;
    int is_11;
    if (crlf - ver == 8 && strncmp(ver, "HTTP/1.1", 8) == 0)
        is_11 = 1;
    else if (crlf - ver == 8 && strncmp(ver, "HTTP/1.0", 8) == 0)
        is_11 = 0;
    else
        return -1;

    /* defaults */
    c->keep_alive = is_11; /* HTTP/1.1 = persistent, HTTP/1.0 = ephemeral */

    /* scan headers */
    int has_host = 0;
    line = crlf + 2;
    while (line < end) {
        crlf = strstr(line, "\r\n");
        if (!crlf)
            break;

        if (header_match(line, "Host")) {
            has_host = 1;
        } else if (header_match(line, "Connection")) {
            char *val = strchr(line, ':') + 1;
            while (*val == ' ' || *val == '\t')
                val++;
            if (strncasecmp(val, "keep-alive", 10) == 0 &&
                (val[10] == '\r' || val[10] == ' ' || val[10] == '\0'))
                c->keep_alive = 1;
            else if (strncasecmp(val, "close", 5) == 0 &&
                (val[5] == '\r' || val[5] == ' ' || val[5] == '\0'))
                c->keep_alive = 0;
        }

        line = crlf + 2;
    }

    if (!has_host)
        return -1;

    return 1;
}

/* ---------- response ---------- */

static void
build_error(conn_t *c, int code)
{
    const char *reason = (code == 404) ? "Not Found" : "Bad Request";
    c->resp_hdr_len = snprintf(c->resp_hdr, MAX_HDR,
        "HTTP/1.0 %d %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        code, reason);
    c->resp_hdr_sent = 0;
    c->keep_alive = 0;
    c->file_fd = -1;
    c->file_size = 0;
    c->file_sent = 0;
    c->state = STATE_SEND_HDR;
}

static int
prepare_response(conn_t *c)
{
    /* build file path */
    char path[MAX_URL + 256];
    size_t rlen = strlen(g_rootDir);
    /* avoid double slash */
    if (rlen > 0 && g_rootDir[rlen - 1] == '/')
        snprintf(path, sizeof(path), "%s%s", g_rootDir, c->url + 1);
    else
        snprintf(path, sizeof(path), "%s%s", g_rootDir, c->url);

    struct stat st;
    if (stat(path, &st) < 0) {
        if (errno == ENOENT)
            build_error(c, 404);
        else
            build_error(c, 400);
        return 0;
    }
    if (!S_ISREG(st.st_mode)) {
        build_error(c, 400);
        return 0;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        build_error(c, 400);
        return 0;
    }

    c->file_fd = fd;
    c->file_size = st.st_size;
    c->file_sent = 0;

    const char *conn_str = c->keep_alive ? "Keep-Alive" : "close";
    c->resp_hdr_len = snprintf(c->resp_hdr, MAX_HDR,
        "HTTP/1.0 200 OK\r\n"
        "Content-length: %ld\r\n"
        "Connection: %s\r\n"
        "\r\n",
        (long)c->file_size, conn_str);
    c->resp_hdr_sent = 0;
    c->state = STATE_SEND_HDR;
    return 0;
}

/* ---------- event handlers ---------- */

static void
handle_accept(void)
{
    /* edge-triggered: must drain all pending connections */
    for (;;) {
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
        int fd = accept(g_listenfd, (struct sockaddr *)&addr, &addrlen);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            perror("accept");
            break;
        }
        if (fd >= MAX_FD) {
            close(fd);
            continue;
        }
        set_nonblocking(fd);
        conn_new(fd);

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = fd;
        epoll_ctl(g_epfd, EPOLL_CTL_ADD, fd, &ev);
    }
}

static void
handle_read(conn_t *c)
{
    /* edge-triggered: drain all available data */
    for (;;) {
        int space = MAX_HDR - c->req_len;
        if (space <= 0) {
            /* header too large */
            build_error(c, 400);
            epoll_mod(c->fd, EPOLLOUT);
            return;
        }
        ssize_t n = read(c->fd, c->req_buf + c->req_len, space);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            conn_free(c);
            return;
        }
        if (n == 0) {
            /* client closed */
            conn_free(c);
            return;
        }
        c->req_len += n;
    }

    int ret = parse_request(c);
    if (ret == 0)
        return; /* incomplete, wait for more */
    if (ret < 0) {
        build_error(c, 400);
        epoll_mod(c->fd, EPOLLOUT);
        return;
    }

    /* request parsed successfully */
    prepare_response(c);
    epoll_mod(c->fd, EPOLLOUT);
}

static void
handle_write(conn_t *c)
{
    /* send header */
    while (c->resp_hdr_sent < c->resp_hdr_len) {
        ssize_t n = write(c->fd,
                          c->resp_hdr + c->resp_hdr_sent,
                          c->resp_hdr_len - c->resp_hdr_sent);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return; /* wait for next EPOLLOUT */
            conn_free(c);
            return;
        }
        c->resp_hdr_sent += n;
    }

    /* header done — send file body via sendfile */
    if (c->file_fd >= 0) {
        while (c->file_sent < c->file_size) {
            off_t remaining = c->file_size - c->file_sent;
            size_t chunk = remaining < SENDFILE_CHUNK ? remaining : SENDFILE_CHUNK;
            off_t off = c->file_sent;
            ssize_t n = sendfile(c->fd, c->file_fd, &off, chunk);
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    return; /* wait for next EPOLLOUT */
                conn_free(c);
                return;
            }
            if (n == 0) {
                conn_free(c);
                return;
            }
            c->file_sent += n;
        }
    }

    /* transfer complete */
    if (c->keep_alive) {
        int fd = c->fd;
        conn_reset(c);
        epoll_mod(fd, EPOLLIN);
        /* edge-triggered: data may already be buffered — try reading now
           to avoid stall when no new EPOLLIN edge arrives */
        handle_read(c);
        /* handle_read may have freed c — caller must not touch c after */
    } else {
        conn_free(c);
    }
}

/* ---------- main ---------- */

static void
PrintUsage(const char *prog)
{
    printf("usage: %s -p port -d rootDirectory(optional)\n", prog);
}

int
main(const int argc, const char **argv)
{
    int i;
    int port = -1;

    /* argument parsing */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && (i + 1) < argc) {
            port = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-d") == 0 && (i + 1) < argc) {
            g_rootDir = argv[i + 1];
            i++;
        }
    }
    if (port <= 0 || port > 65535) {
        PrintUsage(argv[0]);
        exit(-1);
    }
    if (access(g_rootDir, R_OK | X_OK) < 0) {
        fprintf(stderr, "root dir %s inaccessible, errno=%d\n",
                g_rootDir, errno);
        PrintUsage(argv[0]);
        exit(-1);
    }

    signal(SIGPIPE, SIG_IGN);

    /* create listening socket */
    g_listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listenfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    setsockopt(g_listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(port);

    if (bind(g_listenfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(g_listenfd, 128) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    set_nonblocking(g_listenfd);

    /* create epoll */
    g_epfd = epoll_create(1);
    if (g_epfd < 0) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = g_listenfd;
    if (epoll_ctl(g_epfd, EPOLL_CTL_ADD, g_listenfd, &ev) < 0) {
        perror("epoll_ctl: listen");
        exit(EXIT_FAILURE);
    }

    /* event loop */
    struct epoll_event events[MAX_EVENTS];
    for (;;) {
        int nfds = epoll_wait(g_epfd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;

            if (fd == g_listenfd) {
                handle_accept();
                continue;
            }

            conn_t *c = conns[fd];
            if (!c) {
                epoll_ctl(g_epfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
                continue;
            }

            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                conn_free(c);
                continue;
            }

            if (events[i].events & EPOLLIN)
                handle_read(c);

            /* re-check: handle_read may have freed c */
            c = conns[fd];
            if (!c)
                continue;

            if (events[i].events & EPOLLOUT)
                handle_write(c);
        }
    }

    return 0;
}
