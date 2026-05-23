// linux_ser_block.c
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

static void die(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char **argv)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        die("socket");

    int one = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0)
        die("setsockopt");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        die("bind");
    if (listen(s, 1024) < 0)
        die("listen");

    fprintf(stderr, "Linux server listening on %s:%s\n",argv[1], argv[2]);
    int flow_num = 0;
    while (1)
    {
        flow_num++;
        int c = accept(s, NULL, NULL);
        if (c < 0)
        {
            if (errno == EINTR)
                continue;
            die("accept");
        }
        printf("accepted connection, flow_id: %d\n", flow_num);     
        char buf[1024];
        while (1)
        {
            ssize_t n = read(c, buf, sizeof(buf));
            if (n > 0)
            {
                /* Ignore any received data. This test is for connection management only. */
                continue;
            }
            else if (n == 0)
            {
                /* Peer closed first: Linux server becomes passive closer. */
                break;
            }
            else
            {
                if (errno == EINTR)
                    continue;
                perror("read");
                break;
            }
        }
        close(c);
        printf("flow: %d connection complete\n",flow_num);
    }
}
