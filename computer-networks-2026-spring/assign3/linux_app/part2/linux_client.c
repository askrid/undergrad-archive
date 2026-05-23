#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define BUF_SIZE (128 * 1024)
#define SOCK_BUF_SIZE (1 << 20)   // 1 MB
#define HEADER_BUF_SIZE 8192

typedef struct {
    char server_ip[64];
    int port;
    char file_name[256];
    int flow_id;
} flow_arg_t;

void *download_flow(void *arg)
{
    flow_arg_t *farg = (flow_arg_t *)arg;
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE];
    int n;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "[flow %d] socket error: %s\n", farg->flow_id, strerror(errno));
        return NULL;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(farg->port);

    if (inet_pton(AF_INET, farg->server_ip, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "[flow %d] inet_pton error for %s\n", farg->flow_id, farg->server_ip);
        close(sock);
        return NULL;
    }

    int size = SOCK_BUF_SIZE;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) < 0) {
        fprintf(stderr, "[flow %d] setsockopt SO_RCVBUF failed: %s\n",
                farg->flow_id, strerror(errno));
    }
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) < 0) {
        fprintf(stderr, "[flow %d] setsockopt SO_SNDBUF failed: %s\n",
                farg->flow_id, strerror(errno));
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "[flow %d] connect error: %s\n", farg->flow_id, strerror(errno));
        close(sock);
        return NULL;
    }

    char req[1024];
    snprintf(req, sizeof(req),
             "GET /%s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             farg->file_name, farg->server_ip);

    printf("[flow %d] Request:\n%s", farg->flow_id, req);

    if (write(sock, req, strlen(req)) < 0) {
        fprintf(stderr, "[flow %d] write error: %s\n", farg->flow_id, strerror(errno));
        close(sock);
        return NULL;
    }

    char out_name[512];
    snprintf(out_name, sizeof(out_name), "downloaded_flow_%d_%s.txt", farg->flow_id, farg->file_name);

    FILE *fp = fopen(out_name, "wb");
    if (!fp) {
        fprintf(stderr, "[flow %d] fopen error for %s: %s\n",
                farg->flow_id, out_name, strerror(errno));
        close(sock);
        return NULL;
    }

    fprintf(stderr, "[flow %d] Saving to file: %s\n", farg->flow_id, out_name);

    int header_done = 0;
    char header_buf[HEADER_BUF_SIZE];
    int header_len = 0;

    while ((n = read(sock, buffer, BUF_SIZE)) > 0) {
        if (!header_done) {
            if (header_len + n > HEADER_BUF_SIZE) {
                fprintf(stderr, "[flow %d] HTTP header too large\n", farg->flow_id);
                break;
            }

            memcpy(header_buf + header_len, buffer, n);
            header_len += n;

            int body_offset = -1;
            for (int i = 0; i <= header_len - 4; i++) {
                if (header_buf[i] == '\r' &&
                    header_buf[i + 1] == '\n' &&
                    header_buf[i + 2] == '\r' &&
                    header_buf[i + 3] == '\n') {
                    body_offset = i + 4;
                    break;
                }
            }

            if (body_offset != -1) {
                header_done = 1;
                int body_len = header_len - body_offset;
                if (body_len > 0) {
                    size_t written = fwrite(header_buf + body_offset, 1, body_len, fp);
                    if (written != (size_t)body_len) {
                        fprintf(stderr, "[flow %d] fwrite error\n", farg->flow_id);
                        break;
                    }
                }
            }
        } else {
            size_t written = fwrite(buffer, 1, n, fp);
            if (written != (size_t)n) {
                fprintf(stderr, "[flow %d] fwrite error\n", farg->flow_id);
                break;
            }
        }
    }

    if (n < 0) {
        fprintf(stderr, "[flow %d] read error: %s\n", farg->flow_id, strerror(errno));
    }

    fclose(fp);
    close(sock);

    printf("[flow %d] Download complete: saved to %s\n", farg->flow_id, out_name);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <concurrency> [file_name]\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int concurrency = atoi(argv[3]);
    char *file_name = (argc >= 5) ? argv[4] : "1mb.txt";

    if (concurrency <= 0) {
        fprintf(stderr, "concurrency must be > 0\n");
        return 1;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * concurrency);
    flow_arg_t *args = malloc(sizeof(flow_arg_t) * concurrency);
    if (!threads || !args) {
        perror("malloc");
        free(threads);
        free(args);
        return 1;
    }

    for (int i = 0; i < concurrency; i++) {
        memset(&args[i], 0, sizeof(flow_arg_t));
        snprintf(args[i].server_ip, sizeof(args[i].server_ip), "%s", server_ip);
        snprintf(args[i].file_name, sizeof(args[i].file_name), "%s", file_name);
        args[i].port = port;
        args[i].flow_id = i;

        if (pthread_create(&threads[i], NULL, download_flow, &args[i]) != 0) {
            fprintf(stderr, "pthread_create failed for flow %d\n", i);
            threads[i] = 0;
        }
    }

    for (int i = 0; i < concurrency; i++) {
        if (threads[i]) {
            pthread_join(threads[i], NULL);
        }
    }

    free(threads);
    free(args);
    return 0;
}