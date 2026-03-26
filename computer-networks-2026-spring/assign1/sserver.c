#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#include "macro.h"

#define NUM_CHILDREN 5
#define BAD_REQUEST  "SIMPLE/1.0 400 Bad Request\r\n\r\n"
/*--------------------------------------------------------------------------------*/
static ssize_t
writen(int fd, const void *buf, size_t len)
{
    size_t left = len;
    while (left > 0) {
        ssize_t n = write(fd, buf, left);
        if (n <= 0)
            return -1;
        buf = (const char *)buf + n;
        left -= n;
    }
    return (ssize_t)len;
}
/*--------------------------------------------------------------------------------*/
/* read exactly len bytes; returns 0 on success, -1 on error/EOF */
static int
readn(int fd, char *buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
        ssize_t n = read(fd, buf + total, len - total);
        if (n <= 0)
            return -1;
        total += n;
    }
    return 0;
}
/*--------------------------------------------------------------------------------*/
/* read header byte-by-byte until \r\n\r\n; returns header length or -1 */
static int
read_hdr(int fd, char *buf, size_t cap)
{
    size_t len = 0;
    while (len < cap) {
        ssize_t n = read(fd, buf + len, 1);
        if (n <= 0)
            return -1;
        len++;
        if (len >= 4 &&
            buf[len-4] == '\r' && buf[len-3] == '\n' &&
            buf[len-2] == '\r' && buf[len-1] == '\n')
            return (int)len;
    }
    return -1;
}
/*--------------------------------------------------------------------------------*/
static char *
sskip(char *p)
{
    while (*p != '\0' && isspace((unsigned char)*p))
        p++;
    return p;
}
/*--------------------------------------------------------------------------------*/
static void
rtrim(char *start)
{
    char *end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1)))
        end--;
    *end = '\0';
}
/*--------------------------------------------------------------------------------*/
/* parse request header; returns 0 on success, -1 on bad request.
   hdr is modified in place (null-terminated at line boundaries). */
static int
parse_hdr(char *hdr, int hdr_len, long *content_len_out)
{
    int has_host = FALSE;
    int has_content_length = FALSE;

    /* walk lines delimited by \r\n; header ends at \r\n\r\n (already consumed) */
    char *p = hdr;
    char *end = hdr + hdr_len - 2; /* points to final \r\n before the empty line */
    int lineno = 0;

    while (p < end) {
        /* find the \r\n ending this line */
        char *crlf = strstr(p, "\r\n");
        if (crlf == NULL || crlf > end)
            return -1;
        *crlf = '\0'; /* null-terminate this line */

        if (lineno == 0) {
            /* request line: "POST" must start at beginning */
            char *q = p;
            if (strncmp(q, "POST", 4) != 0)
                return -1;
            q += 4;
            if (!isspace((unsigned char)*q))
                return -1;
            q = sskip(q);
            if (strncmp(q, "message", 7) != 0)
                return -1;
            q += 7;
            if (!isspace((unsigned char)*q))
                return -1;
            q = sskip(q);
            rtrim(q);
            if (strcmp(q, "SIMPLE/1.0") != 0)
                return -1;
        } else {
            /* header field: no leading whitespace before field name */
            char *colon = strchr(p, ':');
            if (colon == NULL)
                return -1;

            size_t name_len = colon - p;
            char *value = sskip(colon + 1);
            rtrim(value);

            if (name_len == 4 && strncasecmp(p, "Host", 4) == 0) {
                if (strlen(value) == 0)
                    return -1;
                has_host = TRUE;
            } else if (name_len == 14 && strncasecmp(p, "Content-Length", 14) == 0) {
                char *endptr;
                long cl = strtol(value, &endptr, 10);
                if (endptr == value || *endptr != '\0' || cl < 0 || cl > MAX_CONT)
                    return -1;
                *content_len_out = cl;
                has_content_length = TRUE;
            }
        }

        lineno++;
        p = crlf + 2; /* advance past \r\n */
    }

    if (!has_host || !has_content_length)
        return -1;

    return 0;
}
/*--------------------------------------------------------------------------------*/
static void
handle_client(int connfd)
{
    char *body = NULL;

    /* read request header */
    char hdr[MAX_HDR];
    int hdr_len = read_hdr(connfd, hdr, MAX_HDR);
    if (hdr_len < 0) {
        writen(connfd, BAD_REQUEST, strlen(BAD_REQUEST));
        goto bye;
    }

    /* parse request header */
    long content_len = 0;
    if (parse_hdr(hdr, hdr_len, &content_len) < 0) {
        writen(connfd, BAD_REQUEST, strlen(BAD_REQUEST));
        goto bye;
    }

    /* read request body (exactly content_len bytes) */
    if (content_len > 0) {
        body = (char *)malloc(content_len);
        if (body == NULL) {
            writen(connfd, BAD_REQUEST, strlen(BAD_REQUEST));
            goto bye;
        }
        if (readn(connfd, body, content_len) < 0) {
            writen(connfd, BAD_REQUEST, strlen(BAD_REQUEST));
            goto bye;
        }
    }

    /* build and send response header */
    char resp_hdr[MAX_HDR];
    int resp_hdr_len = snprintf(resp_hdr, MAX_HDR,
        "SIMPLE/1.0 200 OK\r\n"
        "Content-Length: %ld\r\n"
        "\r\n",
        content_len);

    if (writen(connfd, resp_hdr, resp_hdr_len) < 0)
        goto bye;

    /* send response body (echo back) */
    if (content_len > 0)
        writen(connfd, body, content_len);

bye:
    free(body);
    close(connfd);
}
/*--------------------------------------------------------------------------------*/
int
main(const int argc, const char** argv)
{
    int sockfd = -1;

    /* ignore SIGCHLD to auto-reap zombie children */
    signal(SIGCHLD, SIG_IGN);

    /* ignore SIGPIPE to prevent process kill on early connection close */
    signal(SIGPIPE, SIG_IGN);

    /* argument parsing */
    int port = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && (i+1) < argc) {
            port = atoi(argv[i+1]);
            i++;
        }
    }
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "usage: %s -p port\n", argv[0]);
        exit(-1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket() failed");
        goto cleanup;
    }

    /* allow port reuse */
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("bind() failed");
        goto cleanup;
    }

    if (listen(sockfd, 1024) < 0) {
        perror("listen() failed");
        goto cleanup;
    }

    /* prefork worker processes */
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork() failed");
            goto cleanup;
        }
        if (pid == 0) {
            /* child: accept and handle connections in a loop */
            for (;;) {
                int connfd = accept(sockfd, NULL, NULL);
                if (connfd < 0)
                    continue;
                handle_client(connfd);
            }
        }
    }

    /* parent: wait until killed */
    for (;;)
        pause();

cleanup:
    if (sockfd >= 0)
        close(sockfd);
    return -1;
}
