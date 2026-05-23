// linux_cli_block.c
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char **argv) {
    int concurrency = atoi(argv[3]);
    if (concurrency <= 0) {
        fprintf(stderr, "concurrency must be > 0\n");
        return 1;
    }

    int *socks = malloc(sizeof(int) * concurrency);
    if (!socks) die("malloc");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) != 1) die("inet_pton");

    for (int i = 0; i < concurrency; i++) {
        socks[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (socks[i] < 0) die("socket");

        if (connect(socks[i], (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            fprintf(stderr, "connect failed at connection %d\n", i);
            die("connect");
        }
    }

    for (int i = 0; i < concurrency; i++) {
        close(socks[i]);
    }

    free(socks);

    printf("Linux client: %d connect+close operations OK\n", concurrency);
    return 0;
}