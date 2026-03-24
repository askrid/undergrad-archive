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
int
main(const int argc, const char** argv)
{
    const char *server = NULL;
    int port = -1;
    int ret = -1;
    int sockfd = -1;
    char *iobuf = NULL;
    char *content;
    size_t content_len = 0;
    ssize_t nread;
    int hdr_len;
    struct hostent *hostent;
    struct sockaddr_in servaddr;
    size_t resp_len = 0;
    size_t resp_cap = MAX_HDR + MAX_CONT;
    char *sep;
    char *body;
    char *status_end;
    int status_ok = FALSE;
    int i;

    /* argument processing */
    for (i = 1; i < argc; i++) {
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
    iobuf = (char *)malloc(MAX_HDR + MAX_CONT + 1);
    if (iobuf == NULL) {
        perror("malloc() failed");
        exit(-1);
    }

    content = iobuf + MAX_HDR;

    /* read message content from stdin (up to MAX_CONT bytes) */
    while (content_len < (size_t)MAX_CONT &&
           (nread = read(STDIN_FILENO, content + content_len, MAX_CONT - content_len)) > 0) {
        content_len += nread;
    }

    /* discard remaining input if exceeds MAX_CONT */
    if (content_len == (size_t)MAX_CONT) {
        char discard[4096];
        while (read(STDIN_FILENO, discard, sizeof(discard)) > 0)
            ;
    }

    /* empty input: error */
    if (content_len == 0) {
        fprintf(stderr, "error: empty input\n");
        goto cleanup;
    }

    /* build request header */
    hdr_len = snprintf(iobuf, MAX_HDR,
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
    if ((hostent = gethostbyname(server)) == NULL) {
        fprintf(stderr, "gethostbyname() failed: %s\n", strerror(errno));
        goto cleanup;
    }

    /* connect */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    memcpy(&servaddr.sin_addr.s_addr, hostent->h_addr_list[0], hostent->h_length);
    servaddr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect() failed");
        goto cleanup;
    }

    /* send header + content */
    if (writen(sockfd, iobuf, hdr_len) < 0) {
        perror("write() header failed");
        goto cleanup;
    }
    if (writen(sockfd, content, content_len) < 0) {
        perror("write() content failed");
        goto cleanup;
    }

    /* read entire response into iobuf (reuse the buffer) */
    while ((nread = read(sockfd, iobuf + resp_len, resp_cap - resp_len)) > 0) {
        resp_len += nread;
    }

    /* parse response: find end of header (\r\n\r\n) */
    iobuf[resp_len] = '\0';
    sep = strstr(iobuf, "\r\n\r\n");
    body = sep ? sep + 4 : NULL;

    /* check if status line contains "200 OK" */
    status_end = strstr(iobuf, "\r\n");
    if (status_end != NULL) {
        *status_end = '\0';
        if (strstr(iobuf, "200 OK") != NULL)
            status_ok = TRUE;
        *status_end = '\r';
    }

    if (status_ok && body != NULL) {
        /* success: write only the body */
        size_t body_len = resp_len - (body - iobuf);
        fwrite(body, 1, body_len, stdout);
    } else {
        /* error: write entire response header */
        if (body != NULL)
            fwrite(iobuf, 1, body - iobuf, stdout);
        else
            fwrite(iobuf, 1, resp_len, stdout);
    }

    ret = 0;

cleanup:
    if (sockfd >= 0)
        close(sockfd);
    free(iobuf);
    return ret;
}
