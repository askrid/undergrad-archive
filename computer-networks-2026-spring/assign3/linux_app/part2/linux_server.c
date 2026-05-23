// sample_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>

#define BACKLOG 128
#define BUF_SIZE 8192

static volatile sig_atomic_t running = 1;
static int server_fd = -1;

void handle_sigint(int sig) {
    (void)sig;
    running = 0;

    if (server_fd >= 0) {
        shutdown(server_fd, SHUT_RDWR);   // accept 깨우기
        close(server_fd);
        server_fd = -1;
    }
}

// Thread to handle each client
void *client_thread(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    char buf[BUF_SIZE];
    ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        close(client_fd);
        return NULL;
    }
    buf[n] = '\0';
    printf("=== Request ===\n%s\n", buf);

    // Extract file name from "GET /file"
    char filename[256] = {0};
    sscanf(buf, "GET /%255s", filename);

    if (strlen(filename) == 0 || strcmp(filename, "HTTP/1.1") == 0) {
        strcpy(filename, "index.html");
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        const char *resp404 =
            "HTTP/1.0 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "Connection: Close\r\n\r\n";
        write(client_fd, resp404, strlen(resp404));
        close(client_fd);
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        close(client_fd);
        return NULL;
    }

    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.0 200 OK\r\n"
        "Content-Length: %ld\r\n"
        "Connection: Close\r\n\r\n",
        (long)st.st_size);
    write(client_fd, header, header_len);

    ssize_t r;
    while ((r = read(fd, buf, sizeof(buf))) > 0) {
        ssize_t sent = 0;
        while (sent < r) {
            ssize_t w = write(client_fd, buf + sent, r - sent);
            if (w < 0) {
                perror("write");
                break;
            }
            sent += w;
        }
    }

    close(fd);
    close(client_fd);
    printf("Finished serving %s\n", filename);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;   // SA_RESTART 안 줘야 accept가 EINTR/에러로 깸
    sigaction(SIGINT, &sa, NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        exit(1);
    }

    printf("Linux TCP server running on %s:%s\n", argv[1], argv[2]);

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        int *client_fd = malloc(sizeof(int));
        if (!client_fd) {
            perror("malloc");
            break;
        }

        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (*client_fd < 0) {
            free(client_fd);

            if (!running) break;
            if (errno == EINTR) break;

            perror("accept");
            continue;
        }

        printf("Accepted connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, client_fd) != 0) {
            perror("pthread_create");
            close(*client_fd);
            free(client_fd);
            continue;
        }
        pthread_detach(tid);
    }

    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }

    printf("Server shutting down\n");
    return 0;
}