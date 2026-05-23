// ETCP_cli.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <limits.h>
#include <etcp_api.h>
#include <etcp_epoll.h>
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAX_CPUS
#define MAX_CPUS 16
#endif

#define IP_RANGE 1

/* tuning */
#define EPOLL_WAIT_MS 100
#define CONNECT_TIMEOUT_MS 50000   // 5s
#define MAXEVENTS_FACTOR 3

/*----------------------------------------------------------------------------*/
static pthread_t app_thread[MAX_CPUS];
static int done[MAX_CPUS];
static int core_limit = 1;

static in_addr_t daddr;
static in_port_t dport;
static in_addr_t saddr;

static int total_flows;
static int flows[MAX_CPUS];

static int concurrency = 100;
static int max_fds;

/*----------------------------------------------------------------------------*/
static inline uint64_t now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)(tv.tv_usec / 1000);
}

/*----------------------------------------------------------------------------*/
struct conn_stat {
    uint64_t waits, events;

    uint64_t sock_created;
    uint64_t connect_calls;

    uint64_t connect_ok;
    uint64_t connect_fail;
    uint64_t connect_timeout;

    uint64_t closed;

    uint64_t epoll_err_events;
    uint64_t errors;
};

struct conn_vars {
    int in_use;             // 1 if socket is tracked
    uint64_t start_ms;      // connect start time
};

struct thread_context {
    int core;
    etcp_engine_t ectx;
    int ep;

    int target;
    int started;
    int pending;
    int done_cnt;

    int errors;

    struct conn_stat st;

    struct conn_vars *cv;   // indexed by sockid (bounds-checked)
    int *pending_list;      // list of sockids currently pending
    int pending_list_len;
};
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
static void SignalHandler(int signum)
{
    (void)signum;
    for (int i = 0; i < core_limit; i++) done[i] = TRUE;
}

/*----------------------------------------------------------------------------*/
static void remove_from_pending_list(struct thread_context *ctx, int sockid)
{
    // swap-remove
    for (int i = 0; i < ctx->pending_list_len; i++) {
        if (ctx->pending_list[i] == sockid) {
            ctx->pending_list[i] = ctx->pending_list[ctx->pending_list_len - 1];
            ctx->pending_list_len--;
            return;
        }
    }
}

/*----------------------------------------------------------------------------*/
static inline void CloseConnection(struct thread_context *ctx, int sockid)
{
    // best-effort epoll del (even if not added)
    etcp_epoll_ctl(ctx->ectx, ctx->ep, etcp_EPOLL_CTL_DEL, sockid, NULL);
    etcp_close(ctx->ectx, sockid);
    ctx->st.closed++;

    if (sockid >= 0 && sockid < max_fds && ctx->cv[sockid].in_use) {
        ctx->cv[sockid].in_use = 0;
        remove_from_pending_list(ctx, sockid);
        ctx->pending--;
        if (ctx->pending < 0) ctx->pending = 0; // safety
    }

    ctx->done_cnt++;
}

/*----------------------------------------------------------------------------*/
static inline void HandleConnectComplete(struct thread_context *ctx, int sockid)
{
    int err = 0;
    socklen_t len = sizeof(err);

    if (etcp_getsockopt(ctx->ectx, sockid, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
        ctx->st.errors++;
        ctx->errors++;
        ctx->st.connect_fail++;
        CloseConnection(ctx, sockid);
        return;
    }

    if (err != 0) {
        ctx->st.connect_fail++;
        ctx->st.errors++;
        ctx->errors++;
        CloseConnection(ctx, sockid);
        return;
    }

    // connect success
    ctx->st.connect_ok++;
    CloseConnection(ctx, sockid); // connection management only: immediately close
}

/*----------------------------------------------------------------------------*/
static inline int CreateConnection(struct thread_context *ctx)
{
    struct sockaddr_in addr;
    struct etcp_epoll_event ev;

    int sockid = etcp_socket(ctx->ectx, AF_INET, SOCK_STREAM, 0);
    if (sockid < 0) {
        ctx->st.errors++;
        return -1;
    }
    ctx->st.sock_created++;

    // bounds check: avoid memory corruption (random stuck)
    if (sockid >= max_fds) {
        etcp_close(ctx->ectx, sockid);
        ctx->st.errors++;
        return -1;
    }

    if (etcp_setsock_nonblock(ctx->ectx, sockid) < 0) {
        etcp_close(ctx->ectx, sockid);
        ctx->st.errors++;
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = daddr;
    addr.sin_port = dport;

    uint64_t t0 = now_ms();
    int ret = etcp_connect(ctx->ectx, sockid, (struct sockaddr *)&addr, sizeof(addr));
    ctx->st.connect_calls++;

    // track started regardless of sync/async; it counts toward target
    ctx->started++;

    if (ret == 0) {
        // IMPORTANT: immediate success -> do not rely on epoll to fire
        ctx->st.connect_ok++;
        ctx->st.closed++;
        etcp_close(ctx->ectx, sockid);
        ctx->done_cnt++;
        return sockid;
    }

    if (ret < 0 && errno != EINPROGRESS) {
        // immediate failure
        ctx->st.connect_fail++;
        ctx->st.errors++;
        ctx->errors++;
        etcp_close(ctx->ectx, sockid);
        ctx->done_cnt++;
        return -1;
    }

    // EINPROGRESS: register for both IN/OUT/ERR (implementation differences)
    ctx->cv[sockid].in_use = 1;
    ctx->cv[sockid].start_ms = t0;
    ctx->pending_list[ctx->pending_list_len++] = sockid;
    ctx->pending++;

    ev.events = etcp_EPOLLIN | etcp_EPOLLOUT | etcp_EPOLLERR;
    ev.data.sockid = sockid;
    etcp_epoll_ctl(ctx->ectx, ctx->ep, etcp_EPOLL_CTL_ADD, sockid, &ev);

    return sockid;
}

/*----------------------------------------------------------------------------*/
static void CheckConnectTimeouts(struct thread_context *ctx)
{
    uint64_t t = now_ms();

    // iterate over pending list (may mutate via CloseConnection)
    int i = 0;
    while (i < ctx->pending_list_len) {
        int sockid = ctx->pending_list[i];

        if (sockid < 0 || sockid >= max_fds || !ctx->cv[sockid].in_use) {
            // stale entry (shouldn't happen often)
            ctx->pending_list[i] = ctx->pending_list[ctx->pending_list_len - 1];
            ctx->pending_list_len--;
            continue;
        }

        uint64_t age = t - ctx->cv[sockid].start_ms;
        if (age >= CONNECT_TIMEOUT_MS) {
            ctx->st.connect_timeout++;
            ctx->st.errors++;
            ctx->errors++;
            CloseConnection(ctx, sockid);
            // CloseConnection swap-removes; do not i++
            continue;
        }

        i++;
    }
}
/*----------------------------------------------------------------------------*/
static void *RunConnMain(void *arg)
{
    int core = *(int *)arg;
    struct thread_context *ctx = (struct thread_context *)calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;

    ctx->core = core;
    ctx->ectx = etcp_create_context(core);
    if (!ctx->ectx) {
        free(ctx);
        return NULL;
    }

    etcp_init_rss(ctx->ectx, saddr, IP_RANGE, daddr, dport);

    int n = flows[core];
    if (n <= 0) {
        etcp_destroy_context(ctx->ectx);
        free(ctx);
        return NULL;
    }
    ctx->target = n;

    int maxevents = max_fds * MAXEVENTS_FACTOR;
    ctx->ep = etcp_epoll_create(ctx->ectx, maxevents);
    if (ctx->ep < 0) {
        etcp_destroy_context(ctx->ectx);
        free(ctx);
        return NULL;
    }

    struct etcp_epoll_event *events =
        (struct etcp_epoll_event *)calloc(maxevents, sizeof(*events));
    if (!events) {
        etcp_destroy_context(ctx->ectx);
        free(ctx);
        return NULL;
    }

    ctx->cv = (struct conn_vars *)calloc(max_fds, sizeof(struct conn_vars));
    ctx->pending_list = (int *)calloc(max_fds, sizeof(int));
    if (!ctx->cv || !ctx->pending_list) {
        free(events);
        if (ctx->cv) free(ctx->cv);
        if (ctx->pending_list) free(ctx->pending_list);
        etcp_destroy_context(ctx->ectx);
        free(ctx);
        return NULL;
    }

    struct timeval tv_now, tv_prev;
    gettimeofday(&tv_now, NULL);
    tv_prev = tv_now;

    while (!done[core]) {
        // keep launching until reach concurrency or target
        while (ctx->pending < concurrency && ctx->started < ctx->target) {
            (void)CreateConnection(ctx); 
        }

        int nevents = etcp_epoll_wait(ctx->ectx, ctx->ep, events, maxevents, EPOLL_WAIT_MS);
        ctx->st.waits++;

        if (nevents < 0) {
            if (errno != EINTR) {
                ctx->st.errors++;
                ctx->errors++;
            }
            break;
        }
        ctx->st.events += nevents;

        for (int i = 0; i < nevents; i++) {
            int sockid = events[i].data.sockid;

            if (events[i].events & etcp_EPOLLERR) {
                ctx->st.epoll_err_events++;
                ctx->st.errors++;
                ctx->errors++;
                CloseConnection(ctx, sockid);
                continue;
            }

            if (events[i].events & (etcp_EPOLLIN | etcp_EPOLLOUT)) {
                HandleConnectComplete(ctx, sockid);
                continue;
            }
            // ignore any other unexpected event
            ctx->st.errors++;
            ctx->errors++;
            CloseConnection(ctx, sockid);
        }

        // timeout scan prevents infinite stuck
        CheckConnectTimeouts(ctx);

        gettimeofday(&tv_now, NULL);
        if (core == 0 && tv_now.tv_sec > tv_prev.tv_sec) {
            tv_prev = tv_now;
        }

        if (ctx->done_cnt >= ctx->target) break;
    }
    free(events);
    free(ctx->cv);
    free(ctx->pending_list);
    etcp_destroy_context(ctx->ectx);
    free(ctx);
    return NULL;
}

/*----------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    daddr = inet_addr(argv[1]);
    dport = htons((uint16_t)atoi(argv[2]));
    total_flows = mystrtol(argv[3], 10);
    if (total_flows <= 0) {
        fprintf(stderr, "flows must be > 0\n");
        return -1;
    }
    saddr = INADDR_ANY;

    char *conf_file = NULL;
    int total_concurrency = 100;

    optind = 4;
    int o;
    while ((o = getopt(argc, argv, "f:")) != -1) {
        switch (o) {
        case 'f':
            conf_file = optarg;
            break;
        default:
            return -1;
        }
    }

    if (!conf_file) {
        fprintf(stderr, "(-f <etcp.conf>) is required\n");
        return -1;
    }

    if (total_concurrency > 0) concurrency = total_concurrency;

    // Must be >= maximum possible sockid returned by etcp_socket.
    // Using 3x concurrency is typical, but you can bump if you see "sockid >= max_fds".
    max_fds = concurrency * 3;

    struct etcp_conf mcfg;
    etcp_getconf(&mcfg);
    mcfg.num_cores = core_limit;
    etcp_setconf(&mcfg);

    if (etcp_init(conf_file)) {
        fprintf(stderr, "etcp_init failed\n");
        return -1;
    }

    etcp_getconf(&mcfg);
    mcfg.max_concurrency = max_fds;
    mcfg.max_num_buffers = max_fds;
    etcp_setconf(&mcfg);

    etcp_register_signal(SIGINT, SignalHandler);

    int core = 0;
    int cores[MAX_CPUS] = {0};
    flows[core] = total_flows;
    done[core] = FALSE;

    if (pthread_create(&app_thread[core], NULL, RunConnMain, &cores[core])) {
        perror("pthread_create");
        etcp_destroy();
        return -1;
    }

    pthread_join(app_thread[core], NULL);
    etcp_destroy();
    return 0;
}