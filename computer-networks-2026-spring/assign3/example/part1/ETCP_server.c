#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <etcp_api.h>
#include <etcp_epoll.h>

// #include "netlib.h"
// #include "debug.h"

#define MAX_FLOW_NUM 10000
#define MAX_EVENTS   (MAX_FLOW_NUM * 3)

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef MAX_CPUS
#define MAX_CPUS 16
#endif

/*----------------------------------------------------------------------------*/
enum conn_state {
    CONN_UNUSED = 0,
    CONN_ESTABLISHED,
    CONN_CLOSING,
    CONN_CLOSED
};
/*----------------------------------------------------------------------------*/
struct conn_vars
{
    uint8_t used;
    uint8_t handshake_done;
    uint8_t teardown_done;
    enum conn_state state;
};
/*----------------------------------------------------------------------------*/
struct thread_context
{
    etcp_engine_t ectx;
    int ep;
    struct conn_vars *cvars;
};
/*----------------------------------------------------------------------------*/
static int server_port;
static int core_limit;
static pthread_t app_thread[MAX_CPUS];
static int done[MAX_CPUS];
static char *conf_file = NULL;
static int backlog = -1;
/*---------------------------------------------------------------------------*/
int
mystrtol(const char *nptr, int base)
{
	int rval;
	char *endptr;

	errno = 0;
	rval = strtol(nptr, &endptr, 10);
	/* check for strtol errors */
	if ((errno == ERANGE && (rval == LONG_MAX ||
				 rval == LONG_MIN))
	    || (errno != 0 && rval == 0)) {
		perror("strtol");
		exit(EXIT_FAILURE);
	}
	if (endptr == nptr) {
		fprintf(stderr, "Parsing strtol error!\n");
		exit(EXIT_FAILURE);
	}

	return rval;
}
/*----------------------------------------------------------------------------*/
void CleanConnVariable(struct conn_vars *cv)
{
    cv->used = FALSE;
    cv->handshake_done = FALSE;
    cv->teardown_done = FALSE;
    cv->state = CONN_UNUSED;
}
/*----------------------------------------------------------------------------*/
void CloseConnection(struct thread_context *ctx, int sockid, struct conn_vars *cv)
{
    if (!cv->teardown_done) {
        printf("[sock %d] 4-way teardown completed / connection closed\n", sockid);
        cv->teardown_done = TRUE;
        cv->state = CONN_CLOSED;
    }

    etcp_epoll_ctl(ctx->ectx, ctx->ep, etcp_EPOLL_CTL_DEL, sockid, NULL);
    etcp_close(ctx->ectx, sockid);
    CleanConnVariable(cv);
}
/*----------------------------------------------------------------------------*/
int AcceptConnection(struct thread_context *ctx, int listener)
{
    etcp_engine_t ectx = ctx->ectx;
    struct conn_vars *cv;
    struct etcp_epoll_event ev;
    int c;

    c = etcp_accept(ectx, listener, NULL, NULL);
    if (c >= 0)
    {
        if (c >= MAX_FLOW_NUM)
        {
            return -1;
        }

        cv = &ctx->cvars[c];
        CleanConnVariable(cv);
        cv->used = TRUE;
        cv->handshake_done = TRUE;
        cv->state = CONN_ESTABLISHED;

        printf("[sock %d] 3-way handshake completed (ESTABLISHED)\n", c);

        ev.events = etcp_EPOLLIN;
        ev.data.sockid = c;
        etcp_setsock_nonblock(ctx->ectx, c);
        etcp_epoll_ctl(ectx, ctx->ep, etcp_EPOLL_CTL_ADD, c, &ev);
    }
    return c;
}
/*----------------------------------------------------------------------------*/
static int HandleReadEvent(struct thread_context *ctx, int sockid, struct conn_vars *cv)
{
    char buf[1024];
    int ret;

    ret = etcp_read(ctx->ectx, sockid, buf, sizeof(buf));

    if (ret > 0) {
        /* connection-management-only server:
         * we intentionally ignore payload data.
         */
        printf("[sock %d] received %d bytes (ignored)\n", sockid, ret);
    }

    return ret;
}
/*----------------------------------------------------------------------------*/
struct thread_context *InitializeServerThread(int core)
{
    struct thread_context *ctx;

    ctx = (struct thread_context *)calloc(1, sizeof(struct thread_context));
    if (!ctx)
    {
        return NULL;
    }

    ctx->ectx = etcp_create_context(core);
    if (!ctx->ectx)
    {
        free(ctx);
        return NULL;
    }

    ctx->ep = etcp_epoll_create(ctx->ectx, MAX_EVENTS);
    if (ctx->ep < 0)
    {
        etcp_destroy_context(ctx->ectx);
        free(ctx);
        return NULL;
    }

    ctx->cvars = (struct conn_vars *)calloc(MAX_FLOW_NUM, sizeof(struct conn_vars));
    if (!ctx->cvars)
    {
        etcp_close(ctx->ectx, ctx->ep);
        etcp_destroy_context(ctx->ectx);
        free(ctx);
        return NULL;
    }

    return ctx;
}
/*----------------------------------------------------------------------------*/
int CreateListeningSocket(struct thread_context *ctx)
{
    int listener;
    struct etcp_epoll_event ev;
    struct sockaddr_in saddr;
    int ret;

    listener = etcp_socket(ctx->ectx, AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
    {
        return -1;
    }

    ret = etcp_setsock_nonblock(ctx->ectx, listener);
    if (ret < 0)
    {
        return -1;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(server_port);

    ret = etcp_bind(ctx->ectx, listener,
                    (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (ret < 0)
    {
        return -1;
    }

    ret = etcp_listen(ctx->ectx, listener, backlog);
    if (ret < 0)
    {
        return -1;
    }

    ev.events = etcp_EPOLLIN;
    ev.data.sockid = listener;
    etcp_epoll_ctl(ctx->ectx, ctx->ep, etcp_EPOLL_CTL_ADD, listener, &ev);

    printf("[listener %d] server is listening on port %d\n", listener, server_port);
    return listener;
}
/*----------------------------------------------------------------------------*/
void *RunServerThread(void *arg)
{
    int core = *(int *)arg;
    struct thread_context *ctx;
    etcp_engine_t ectx;
    int listener;
    int ep;
    struct etcp_epoll_event *events;
    int nevents;
    int i, ret;
    int do_accept;

    ctx = InitializeServerThread(core);
    if (!ctx)
    {
        return NULL;
    }

    ectx = ctx->ectx;
    ep = ctx->ep;

    events = (struct etcp_epoll_event *)calloc(MAX_EVENTS, sizeof(struct etcp_epoll_event));
    if (!events)
    {
        exit(-1);
    }

    listener = CreateListeningSocket(ctx);
    if (listener < 0)
    {
        exit(-1);
    }

    while (!done[core])
    {
        nevents = etcp_epoll_wait(ectx, ep, events, MAX_EVENTS, -1);
        if (nevents < 0)
        {
            if (errno != EINTR)
                perror("etcp_epoll_wait");
            break;
        }

        do_accept = FALSE;

        for (i = 0; i < nevents; i++)
        {
            int sockid = events[i].data.sockid;

            if (sockid == listener)
            {
                do_accept = TRUE;
            }
            else if (events[i].events & etcp_EPOLLERR)
            {
                int err;
                socklen_t len = sizeof(err);

                if (etcp_getsockopt(ectx, sockid,
                                    SOL_SOCKET, SO_ERROR, (void *)&err, &len) == 0)
                {
                    fprintf(stderr, "[sock %d] socket error: %s\n",
                            sockid, strerror(err));
                }
                else
                {
                    perror("etcp_getsockopt");
                }

                CloseConnection(ctx, sockid, &ctx->cvars[sockid]);
            }
            else if (events[i].events & etcp_EPOLLIN)
            {
                ret = HandleReadEvent(ctx, sockid, &ctx->cvars[sockid]);

                if (ret == 0)
                {
                    /* peer performed active close; server observed FIN/EOF */
                    printf("[sock %d] peer initiated close\n", sockid);
                    CloseConnection(ctx, sockid, &ctx->cvars[sockid]);
                }
                else if (ret < 0)
                {
                    if (errno != EAGAIN)
                    {
                        fprintf(stderr, "[sock %d] read error: %s\n",
                                sockid, strerror(errno));
                        CloseConnection(ctx, sockid, &ctx->cvars[sockid]);
                    }
                }
            }
        }

        if (do_accept)
        {
            while (1)
            {
                ret = AcceptConnection(ctx, listener);
                if (ret < 0)
                    break;
            }
        }
    }

    etcp_destroy_context(ectx);
    pthread_exit(NULL);
    return NULL;
}
/*----------------------------------------------------------------------------*/
void SignalHandler(int signum)
{
    int i;

    for (i = 0; i < core_limit; i++)
    {
        if (app_thread[i] == pthread_self())
        {
            done[i] = TRUE;
        }
        else
        {
            if (!done[i])
                pthread_kill(app_thread[i], signum);
        }
    }
}
/*----------------------------------------------------------------------------*/
static void printHelp(const char *prog_name)
{
    printf("Usage: %s -P <port> -f <etcp.conf> [-b backlog]\n", prog_name);
    exit(EXIT_SUCCESS);
}
/*----------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    struct etcp_conf mcfg;
    int cores[MAX_CPUS];
    int o;
    int ret;

    core_limit = 1;

    if (argc < 2)
    {
        printHelp(argv[0]);
    }

    etcp_getconf(&mcfg);
    mcfg.num_cores = 1;
    etcp_setconf(&mcfg);    
    server_port = atoi(argv[1]);

    while ((o = getopt(argc, argv, "f:")) != -1)
    {
        switch (o)
        {
        case 'f':
            conf_file = optarg;
            break;
        }
    }

    if (!conf_file)
    {
        fprintf(stderr, "You must pass -f <etcp.conf>\n");
        return -1;
    }

    ret = etcp_init(conf_file);
    if (ret)
    {
        fprintf(stderr, "Failed to initialize ETCP\n");
        return -1;
    }

    etcp_getconf(&mcfg);

    if (backlog == -1)
        backlog = 4096;

    if (backlog > mcfg.max_concurrency)
    {
        fprintf(stderr, "backlog cannot exceed CONFIG.max_concurrency\n");
        etcp_destroy();
        return -1;
    }

    etcp_register_signal(SIGINT, SignalHandler);

    cores[0] = 0;
    done[0] = FALSE;

    if (pthread_create(&app_thread[0], NULL, RunServerThread, (void *)&cores[0]))
    {
        perror("pthread_create");
        etcp_destroy();
        return -1;
    }

    pthread_join(app_thread[0], NULL);
    etcp_destroy();
    return 0;
}