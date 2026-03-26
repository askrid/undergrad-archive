#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

#include "macro.h"
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
/* read into buf, up to cap bytes; returns bytes read, 0 on empty input */
static size_t
readcap(int fd, char *buf, size_t cap)
{
    size_t total = 0;
    ssize_t n;
    while (total < cap &&
           (n = read(fd, buf + total, cap - total)) > 0) {
        total += n;
    }
    return total;
}
/*--------------------------------------------------------------------------------*/
static size_t
drain(int fd)
{
    size_t total = 0;
    ssize_t n;
    char buf[4096];
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        total += n;
    }
    return total;
}
/*--------------------------------------------------------------------------------*/
/* parse Content-Length from response header; returns value or -1 if not found */
static long
parse_content_length(const char *hdr, const char *end)
{
    const char *p = strstr(hdr, "\r\n"); /* skip the status line */
    while (p != NULL && p < end) {
        p += 2;
        if (strncasecmp(p, "Content-Length:", 15) == 0)
            return strtol(p + 15, NULL, 10);
        p = strstr(p, "\r\n");
    }
    return -1;
}
/*--------------------------------------------------------------------------------*/
/* handle response in buffer; returns 0 on success, -1 on error.
   resp must have at least len+1 bytes allocated (for null terminator). */
static int
handle_resp(char *resp, size_t len)
{
    resp[len] = '\0';

    /* find end of header (\r\n\r\n) */
    char *sep = strstr(resp, "\r\n\r\n");
    char *body = sep ? sep + 4 : NULL;

    /* check if status line contains "200 OK" */
    int status_ok = FALSE;
    char *status_end = strstr(resp, "\r\n");
    if (status_end != NULL) {
        *status_end = '\0';
        if (strstr(resp, "200 OK") != NULL)
            status_ok = TRUE;
        *status_end = '\r';
    }

    if (status_ok && body != NULL) {
        /* validate Content-Length against actual body */
        size_t body_len = len - (body - resp);
        long expected = parse_content_length(resp, sep);
        if (expected >= 0 && body_len < (size_t)expected) {
            fprintf(stderr, "error: response body shorter than Content-Length\n");
            return -1;
        }
        writen(STDOUT_FILENO, body, body_len);
    } else {
        /* error response or no body: write the response header */
        writen(STDOUT_FILENO, resp, len);
    }

    return 0;
}
/*--------------------------------------------------------------------------------*/
int
main(const int argc, const char** argv)
{
    int ret = -1;
    int sockfd = -1;
    char *buf = NULL;

    signal(SIGPIPE, SIG_IGN);

    /* argument processing */
    const char *server = NULL;
    int port = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && (i + 1) < argc) {
            port = atoi(argv[i+1]);
            i++;
        } else if (strcmp(argv[i], "-s") == 0 && (i + 1) < argc) {
            server = argv[i+1];
            i++;
        }
    }

    /* check arguments */
    if (port < 0 || server == NULL) {
        fprintf(stderr, "usage: %s -p port -s server-ip\n", argv[0]);
        exit(-1);
    }
    if (port < 1024 || port > 65535) {
        fprintf(stderr, "port number should be between 1024 ~ 65535.\n");
        exit(-1);
    }

    /* single buffer: MAX_HDR for request header, MAX_CONT for content/response */
    buf = (char *)malloc(MAX_HDR + MAX_CONT + 1);
    if (buf == NULL) {
        perror("malloc() failed");
        goto cleanup;
    }

    /* read message content from stdin */
    char *content = buf + MAX_HDR;
    size_t content_len = readcap(STDIN_FILENO, content, MAX_CONT);
    if (content_len == MAX_CONT) {
        drain(STDIN_FILENO);
    }
    if (content_len == 0) {
        fprintf(stderr, "input should be bigger than 0\n");
        goto cleanup;
    }

    /* build request header */
    int hdr_len = snprintf(buf, MAX_HDR,
        "POST message SIMPLE/1.0\r\n"
        "Host: %s\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",
        server, content_len);

    /* create socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket() failed");
        goto cleanup;
    }

    /* resolve server address */
    struct hostent *hp = gethostbyname(server);
    if (hp == NULL) {
        perror("gethostbyname() failed");
        goto cleanup;
    }

    /* connect */
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    memcpy(&saddr.sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
    saddr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("connect() failed");
        goto cleanup;
    }

    /* send header + content */
    if (writen(sockfd, buf, hdr_len) < 0) {
        perror("write() header failed");
        goto cleanup;
    }
    if (writen(sockfd, content, content_len) < 0) {
        perror("write() content failed");
        goto cleanup;
    }

    /* read entire response into buffer */
    size_t resp_len = readcap(sockfd, buf, MAX_HDR + MAX_CONT);

    /* parse and output response */
    if (handle_resp(buf, resp_len) < 0)
        goto cleanup;

    ret = 0;

cleanup:
    if (sockfd >= 0)
        close(sockfd);
    free(buf);
    return ret;
}
