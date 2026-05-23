#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/queue.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>

#include <etcp_api.h>
#include <etcp_epoll.h>

#define MAX_URL_LEN 128
#define FILE_LEN 128
#define FILE_IDX 10
#define MAX_FILE_LEN (FILE_LEN + FILE_IDX)
#define HTTP_HEADER_LEN 1024

#define IP_RANGE 1
#define MAX_IP_STR_LEN 16

#define BUF_SIZE (8 * 1024)

#define CALC_MD5SUM FALSE

#define TIMEVAL_TO_MSEC(t) ((t.tv_sec * 1000) + (t.tv_usec / 1000))
#define TIMEVAL_TO_USEC(t) ((t.tv_sec * 1000000) + (t.tv_usec))
#define TS_GT(a, b) ((int64_t)((a) - (b)) > 0)

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef ERROR
#define ERROR (-1)
#endif

#ifndef MAX_CPUS
#define MAX_CPUS 16
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define HTTP_STR           "HTTP"
#define HTTPV0_STR         "HTTP/1.0"
#define HTTPV1_STR         "HTTP/1.1"
#define HTTP_GET           "GET"
#define HTTP_POST          "POST"
#define HTTP_CLOSE         "Close"
#define HTTP_KEEP_ALIVE    "Keep-Alive"
#define HOST_HDR           "\nHost:"
#define CONTENT_LENGTH_HDR "\nContent-Length:"
#define CONTENT_TYPE_HDR   "\nContent-Type:"
#define CACHE_CONTROL_HDR  "\nCache-Control:"
#define CONNECTION_HDR     "\nConnection:"
#define DATE_HDR           "\nDate:"
#define EXPIRES_HDR        "\nExpires:"
#define AGE_HDR            "\nAge:"
#define LAST_MODIFIED_HDR	"\nLast-Modified:"
#define IF_MODIFIED_SINCE_HDR	"\nIf-Modified_Since:"
#define PRAGMA_HDR              "\nPragma:"
#define RANGE_HDR               "\nRange:"
#define IF_RANGE_HDR            "\nIf-Range:"
#define ETAG_HDR                "\nETag:"
#define SPACE_OR_TAB(x)  ((x) == ' '  || (x) == '\t')
#define CR_OR_NEWLINE(x) ((x) == '\r' || (x) == '\n')

/*----------------------------------------------------------------------------*/
static pthread_t app_thread[MAX_CPUS];
static etcp_engine_t g_etcp_engine[MAX_CPUS];
static int done[MAX_CPUS];
/*----------------------------------------------------------------------------*/
// static int num_cores;
static int core_limit;
/*----------------------------------------------------------------------------*/
static int fio = FALSE;
static char outfile[FILE_LEN + 1];
/*----------------------------------------------------------------------------*/
static char host[MAX_IP_STR_LEN + 1] = {'\0'};
static char url[MAX_URL_LEN + 1] = {'\0'};
static in_addr_t daddr;
static in_port_t dport;
static in_addr_t saddr;
/*----------------------------------------------------------------------------*/
static int total_flows;
static int flows[MAX_CPUS];
static int flowcnt = 0;
static int concurrency;
static int max_fds;
static uint64_t response_size = 0;
/*----------------------------------------------------------------------------*/
struct wget_stat
{
	uint64_t waits;
	uint64_t events;
	uint64_t connects;
	uint64_t reads;
	uint64_t writes;
	uint64_t completes;

	uint64_t errors;
	uint64_t timedout;

	uint64_t sum_resp_time;
	uint64_t max_resp_time;
};
/*----------------------------------------------------------------------------*/
struct thread_context
{
	int core;

	etcp_engine_t etcp_engine;
	int ep;
	struct wget_vars *wvars;

	int target;
	int started;
	int errors;
	int incompletes;
	int done;
	int pending;

	struct wget_stat stat;
};
typedef struct thread_context *thread_context_t;
/*----------------------------------------------------------------------------*/
struct wget_vars
{
	int request_sent;

	char response[HTTP_HEADER_LEN];
	int resp_len;
	int headerset;
	uint32_t header_len;
	uint64_t file_len;
	uint64_t recv;
	uint64_t write;

	struct timeval t_start;
	struct timeval t_end;

	int fd;
};
/*----------------------------------------------------------------------------*/
static struct thread_context *g_ctx[MAX_CPUS] = {0};
static struct wget_stat *g_stat[MAX_CPUS] = {0};
/*----------------------------------------------------------------------------*/
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
/*---------------------------------------------------------------------------*/
static char* 
nre_strcasestr(const char* buf, const char* key)
{
    int n = strlen(key) - 1;
    const char *p = buf;

	while (*p) {
		while (*p && *p != *key) /* first character match */
			p++;

		if (*p == '\0') 
			return (NULL);
		
		if (!strncasecmp(p + 1, key + 1, n)) 
			return (char *)p;
		p++;
    }
	return NULL;
}
/*---------------------------------------------------------------------------*/
char *
http_header_str_val(const char* buf, const char *key, const int keylen, 
					char* value, int value_len)
{
	char *temp = nre_strcasestr(buf, key);
	int i = 0;
	
	if (temp == NULL) {
		*value = 0;
		return NULL;
	}

	/* skip whitespace or tab */
	temp += keylen;
	while (*temp && SPACE_OR_TAB(*temp))
		temp++;

	/* if we reached the end of the line, forget it */
	if (*temp == '\0' || CR_OR_NEWLINE(*temp)) {
		*value = 0;
		return NULL;
	}

	/* copy value data */
	while (*temp && !CR_OR_NEWLINE(*temp) && i < value_len-1)
		value[i++] = *temp++;
	value[i] = 0;
	
	if (i == 0) {
		*value = 0;
		return NULL;
	}

	return value;
}
/*----------------------------------------------------------------------------*/
long int 
http_header_long_val(const char * response, const char* key, int key_len)
{
#define C_TYPE_LEN 50
	long int len;
	char value[C_TYPE_LEN];
	char *temp = http_header_str_val(response, key, key_len, value, C_TYPE_LEN);

	if (temp == NULL)
		return -1;

	len = strtol(temp, NULL, 10);
	if (errno == EINVAL || errno == ERANGE)
		return -1;

	return len;
}
/*--------------------------------------------------------------------------*/
thread_context_t
CreateContext(int core)
{
	thread_context_t ctx;

	ctx = (thread_context_t)calloc(1, sizeof(struct thread_context));
	if (!ctx)
	{
		perror("malloc");
		fprintf(stderr,"Failed to allocate memory for thread context.\n");
		return NULL;
	}
	ctx->core = core;

	ctx->etcp_engine = etcp_create_context(core);
	if (!ctx->etcp_engine)
	{
		fprintf(stderr,"Failed to create etcp context.\n");
		free(ctx);
		return NULL;
	}
	g_etcp_engine[core] = ctx->etcp_engine;

	return ctx;
}
/*----------------------------------------------------------------------------*/
void DestroyContext(thread_context_t ctx)
{
	g_stat[ctx->core] = NULL;
	etcp_destroy_context(ctx->etcp_engine);
	free(ctx);
}
/*----------------------------------------------------------------------------*/
static inline int
CreateConnection(thread_context_t ctx)
{
	etcp_engine_t etcp_engine = ctx->etcp_engine;
	struct etcp_epoll_event ev;
	struct sockaddr_in addr;
	int sockid;
	int ret;

	sockid = etcp_socket(etcp_engine, AF_INET, SOCK_STREAM, 0);
	if (sockid < 0)
	{
		return -1;
	}

	memset(&ctx->wvars[sockid], 0, sizeof(struct wget_vars));
	ret = etcp_setsock_nonblock(etcp_engine, sockid);
	if (ret < 0)
	{
		fprintf(stderr,"Failed to set socket in nonblocking mode.\n");
		exit(-1);
	}
 
 
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr =daddr;
	addr.sin_port = dport;

	ret = etcp_connect(etcp_engine, sockid,
					   (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret < 0)
	{
		if (errno != EINPROGRESS)
		{
			perror("etcp_connect");
			etcp_close(etcp_engine, sockid);
			return -1;
		}
	}

	ctx->started++;
	ctx->pending++;
	ctx->stat.connects++;

	ev.events = etcp_EPOLLOUT;
	ev.data.sockid = sockid;
	etcp_epoll_ctl(etcp_engine, ctx->ep, etcp_EPOLL_CTL_ADD, sockid, &ev);

	return sockid;
}
/*----------------------------------------------------------------------------*/
static inline void
CloseConnection(thread_context_t ctx, int sockid)
{
	etcp_epoll_ctl(ctx->etcp_engine, ctx->ep, etcp_EPOLL_CTL_DEL, sockid, NULL);
	etcp_close(ctx->etcp_engine, sockid);
	ctx->pending--;
	ctx->done++;
	assert(ctx->pending >= 0);
	while (ctx->pending < concurrency && ctx->started < ctx->target)
	{
		if (CreateConnection(ctx) < 0)
		{
			done[ctx->core] = TRUE;
			break;
		}
	}
}
/*----------------------------------------------------------------------------*/
static inline int
SendHTTPRequest(thread_context_t ctx, int sockid, struct wget_vars *wv)
{
	char request[HTTP_HEADER_LEN];
	struct etcp_epoll_event ev;
	int wr;
	int len;

	wv->headerset = FALSE;
	wv->recv = 0;
	wv->header_len = wv->file_len = 0;

	snprintf(request, HTTP_HEADER_LEN, "GET %s HTTP/1.0\r\n"
									   "User-Agent: Wget/1.12 (linux-gnu)\r\n"
									   "Accept: */*\r\n"
									   "Host: %s\r\n"
									   "Connection: Close\r\n\r\n",
			 url, host);
	len = strlen(request);

	wr = etcp_write(ctx->etcp_engine, sockid, request, len);
	if (wr < len)
	{
		fprintf(stderr,"Socket %d: Sending HTTP request failed. "
					"try: %d, sent: %d\n",
					sockid, len, wr);
	}
	ctx->stat.writes += wr;
	wv->request_sent = TRUE;

	ev.events = etcp_EPOLLIN;
	ev.data.sockid = sockid;
	etcp_epoll_ctl(ctx->etcp_engine, ctx->ep, etcp_EPOLL_CTL_MOD, sockid, &ev);

	gettimeofday(&wv->t_start, NULL);

	char fname[MAX_FILE_LEN + 1];
	if (fio)
	{
		snprintf(fname, MAX_FILE_LEN, "%s.%d", outfile, flowcnt++);
		wv->fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (wv->fd < 0)
		{
			exit(1);
		}
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static inline int
DownloadComplete(thread_context_t ctx, int sockid, struct wget_vars *wv)
{
#ifdef APP
	etcp_engine_t etcp_engine = ctx->etcp_engine;
#endif
	uint64_t tdiff;

	printf("Socket %d File download complete!\n", sockid);
	gettimeofday(&wv->t_end, NULL);
	CloseConnection(ctx, sockid);
	ctx->stat.completes++;
	if (response_size == 0)
	{
		response_size = wv->recv;
		fprintf(stderr, "Response size set to %lu\n", response_size);
	}
	else
	{
		if (wv->recv != response_size)
		{
			fprintf(stderr, "Response size mismatch! mine: %lu, theirs: %lu\n",
					wv->recv, response_size);
		}
	}
	tdiff = (wv->t_end.tv_sec - wv->t_start.tv_sec) * 1000000 +
			(wv->t_end.tv_usec - wv->t_start.tv_usec);
	ctx->stat.sum_resp_time += tdiff;
	if (tdiff > ctx->stat.max_resp_time)
		ctx->stat.max_resp_time = tdiff;

	if (fio && wv->fd > 0)
		close(wv->fd);

	return 0;
}
/*----------------------------------------------------------------------------*/
static inline int
HandleReadEvent(thread_context_t ctx, int sockid, struct wget_vars *wv)
{
	etcp_engine_t etcp_engine = ctx->etcp_engine;
	char buf[BUF_SIZE];
	int rd;

	while (1)
	{
		rd = etcp_read(etcp_engine, sockid, buf, BUF_SIZE);
		if (rd <= 0)
			break;

		ctx->stat.reads += rd;

		if (!wv->headerset)
		{
			/* Look for end of header in this buffer */
			char *hdr_end = strstr(buf, "\r\n\r\n");
			if (hdr_end)
			{
				int header_len_in_buf = (hdr_end - buf) + 4;
				/* Copy header into wv->response for parsing */
				int copy_len = MIN(header_len_in_buf, HTTP_HEADER_LEN - wv->resp_len);
				memcpy(wv->response + wv->resp_len, buf, copy_len);
				wv->resp_len += copy_len;
				wv->response[wv->resp_len] = '\0';

				/* Parse Content-Length */
				wv->file_len = http_header_long_val(
					wv->response,
					CONTENT_LENGTH_HDR,
					sizeof(CONTENT_LENGTH_HDR) - 1);
				if (wv->file_len < 0)
				{
					CloseConnection(ctx, sockid);
					return 0;
				}
				wv->headerset = TRUE;
				wv->header_len = wv->resp_len;

				/* Leftover body starts after header in this buf */
				int body_bytes = rd - header_len_in_buf;
				if (body_bytes > 0)
				{
					char *body_start = buf + header_len_in_buf;
					if (fio && wv->fd > 0)
					{
						int wr = write(wv->fd, body_start, body_bytes);
						if (wr != body_bytes)
						{
							perror("write");
							CloseConnection(ctx, sockid);
							return -1;
						}
						wv->write += wr;
					}
					wv->recv += body_bytes;
				}
			}
			else
			{
				/* Header not finished yet, just accumulate */
				int copy_len = MIN(rd, HTTP_HEADER_LEN - wv->resp_len);
				memcpy(wv->response + wv->resp_len, buf, copy_len);
				wv->resp_len += copy_len;
			}
		}
		else
		{
			/* Header already parsed → everything is body */
			if (fio && wv->fd > 0)
			{
				int wr = write(wv->fd, buf, rd);
				if (wr != rd)
				{
					perror("write");
					CloseConnection(ctx, sockid);
					return -1;
				}
				wv->write += wr;
			}
			wv->recv += rd;
		}

		/* Completion check */
		if (wv->headerset && wv->recv >= wv->file_len)
		{
			DownloadComplete(ctx, sockid, wv);
			return 0;
		}
	}

	if (rd == 0)
	{
		if (wv->headerset && wv->recv >= wv->file_len)
		{
			DownloadComplete(ctx, sockid, wv);
		}
		else
		{
			ctx->stat.errors++;
			ctx->incompletes++;
			CloseConnection(ctx, sockid);
		}
	}
	else if (rd < 0 && errno != EAGAIN)
	{
		fprintf(stderr,"Socket %d: etcp_read() error %s\n",
				  sockid, strerror(errno));
		ctx->stat.errors++;
		ctx->errors++;
		CloseConnection(ctx, sockid);
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static void
PrintStats()
{
	struct wget_stat total = {0};
	struct wget_stat *st;
	uint64_t avg_resp_time;
	uint64_t total_resp_time = 0;
	int i;

	for (i = 0; i < core_limit; i++)
	{
		st = g_stat[i];

		if (st == NULL)
			continue;
		avg_resp_time = st->completes ? st->sum_resp_time / st->completes : 0;

		total.waits += st->waits;
		total.events += st->events;
		total.connects += st->connects;
		total.reads += st->reads;
		total.writes += st->writes;
		total.completes += st->completes;
		total_resp_time += avg_resp_time;
		if (st->max_resp_time > total.max_resp_time)
			total.max_resp_time = st->max_resp_time;
		total.errors += st->errors;
		total.timedout += st->timedout;

		memset(st, 0, sizeof(struct wget_stat));
	}
}
/*----------------------------------------------------------------------------*/
void *
RunWgetMain(void *arg)
{
	thread_context_t ctx;
	etcp_engine_t etcp_engine;
	int core = *(int *)arg;
	struct in_addr daddr_in;
	int n, maxevents;
	int ep;
	struct etcp_epoll_event *events;
	int nevents;
	struct wget_vars *wvars;
	int i;
	struct timeval cur_tv, prev_tv;

	ctx = CreateContext(core);
	if (!ctx)
	{
		return NULL;
	}
	etcp_engine = ctx->etcp_engine;
	g_ctx[core] = ctx;
	g_stat[core] = &ctx->stat;
	srand(time(NULL));

	etcp_init_rss(etcp_engine, saddr, IP_RANGE, daddr, dport);

	n = flows[core];  // number of total_flows
	if (n == 0)
	{
		fprintf(stderr,"Application thread %d finished.\n", core);
		pthread_exit(NULL);
		return NULL;
	}
	ctx->target = n;

	daddr_in.s_addr = daddr;
	fprintf(stderr, "Thread %d handles %d flows. connecting to %s:%u\n",
			core, n, inet_ntoa(daddr_in), ntohs(dport));

	/* Initialization */
	maxevents = max_fds * 3;
	/*create epoll file descriptor*/
	ep = etcp_epoll_create(etcp_engine, maxevents);
	if (ep < 0)
	{
		fprintf(stderr,"Failed to create epoll struct!n");
		exit(EXIT_FAILURE);
	}

	events = (struct etcp_epoll_event *)
		calloc(maxevents, sizeof(struct etcp_epoll_event));
	if (!events)
	{
		fprintf(stderr,"Failed to allocate events!\n");
		exit(EXIT_FAILURE);
	}
	ctx->ep = ep;

	wvars = (struct wget_vars *)calloc(max_fds, sizeof(struct wget_vars));
	if (!wvars)
	{
		fprintf(stderr,"Failed to create wget variables!\n");
		exit(EXIT_FAILURE);
	}
	ctx->wvars = wvars;

	ctx->started = ctx->done = ctx->pending = 0;
	ctx->errors = ctx->incompletes = 0;

	gettimeofday(&cur_tv, NULL);
	prev_tv = cur_tv;

	while (!done[core])
	{
		gettimeofday(&cur_tv, NULL);
		/* print statistics every second */
		if (core == 0 && cur_tv.tv_sec > prev_tv.tv_sec)
		{
			PrintStats();
			prev_tv = cur_tv;
		}

		while (ctx->pending < concurrency && ctx->started < ctx->target)
		{
			if (CreateConnection(ctx) < 0)
			{
				done[core] = TRUE;
				break;
			}
		}

		nevents = etcp_epoll_wait(etcp_engine, ep, events, maxevents, -1);
		ctx->stat.waits++;

		if (nevents < 0)
		{
			if (errno != EINTR)
			{
				fprintf(stderr,"etcp_epoll_wait failed! ret: %d\n", nevents);
			}
			done[core] = TRUE;
			break;
		}
		else
		{
			ctx->stat.events += nevents;
		}

		for (i = 0; i < nevents; i++)
		{

			if (events[i].events & etcp_EPOLLERR)
			{
				int err;
				socklen_t len = sizeof(err);

				ctx->stat.errors++;
				ctx->errors++;
				if (etcp_getsockopt(etcp_engine, events[i].data.sockid,
									SOL_SOCKET, SO_ERROR, (void *)&err, &len) == 0)
				{
					if (err == ETIMEDOUT)
						ctx->stat.timedout++;
				}
				CloseConnection(ctx, events[i].data.sockid);
			}

			else if (events[i].events & etcp_EPOLLIN)
			{
				HandleReadEvent(ctx,
								events[i].data.sockid, &wvars[events[i].data.sockid]);
			}

			else if (events[i].events == etcp_EPOLLOUT)
			{
				struct wget_vars *wv = &wvars[events[i].data.sockid];

				if (!wv->request_sent)
				{
					SendHTTPRequest(ctx, events[i].data.sockid, wv);
				}
			}

			else
			{
				fprintf(stderr,"Socket %d: event: %s\n",
							events[i].data.sockid, ConvertEventToString(events[i].events));
				assert(0);
			}
		}

		if (ctx->done >= ctx->target)
		{
			fprintf(stdout, "[CPU %d] Completed %d connections, "
							"errors: %d incompletes: %d\n",
					ctx->core, ctx->done, ctx->errors, ctx->incompletes);
			break;
		}
	}
	DestroyContext(ctx);

	fprintf(stderr,"Wget thread %d finished.\n", core);
	pthread_exit(NULL);
	return NULL;
}
/*----------------------------------------------------------------------------*/
void SignalHandler(int signum)
{
	int i;

	for (i = 0; i < core_limit; i++)
	{
		done[i] = TRUE;
	}
}
/*----------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	struct etcp_conf mcfg;
	char *conf_file;
	int cores[MAX_CPUS];
	int flow_per_thread;
	int total_concurrency = 0;
	int ret;
	int i, o;

	if (argc < 3)
	{
		return FALSE;
	}

	if (strlen(argv[1]) > MAX_URL_LEN)
	{
		return FALSE;
	}

	char *slash_p = strchr(argv[1], '/');
	if (slash_p)
	{
		strncpy(host, argv[1], slash_p - argv[1]);
		strncpy(url, strchr(argv[1], '/'), MAX_URL_LEN);
	}
	else
	{
		strncpy(host, argv[1], MAX_IP_STR_LEN);
		strncpy(url, "/", 2);
	}

	conf_file = NULL;
	daddr = inet_addr(host);
	dport = htons(atoi(argv[2])); // server port is 8080
	saddr = INADDR_ANY;

	total_flows = atoi(argv[3]);	
	if (total_flows <= 0)
	{
		return FALSE;
	}

	core_limit = 1;
	concurrency = 100;
	etcp_getconf(&mcfg);
	mcfg.num_cores = core_limit;
	etcp_setconf(&mcfg);

	while (-1 != (o = getopt(argc, argv, "c:o:n:f:")))
	{
		switch (o)
		{
		case 'c':
			total_concurrency = mystrtol(optarg, 10);
			break;
		case 'o':
			if (strlen(optarg) > MAX_FILE_LEN)
			{
				return FALSE;
			}
			fio = TRUE;
			strncpy(outfile, optarg, FILE_LEN);
			break;
		case 'f':
			conf_file = optarg;
			break;
		}
	}

	/* per-core concurrency = total_concurrency / # cores */
	if (total_concurrency > 0)
		concurrency = total_concurrency;

	/* set the max number of fds 3x larger than concurrency */
	max_fds = concurrency * 3;
	if (conf_file == NULL)
	{
		fprintf(stderr,"etcp configuration file is not set!\n");
		exit(EXIT_FAILURE);
	}

	ret = etcp_init(conf_file);
	if (ret)
	{
		fprintf(stderr,"Failed to initialize etcp.\n");
		exit(EXIT_FAILURE);
	}
	etcp_getconf(&mcfg);
	mcfg.max_concurrency = max_fds;
	mcfg.max_num_buffers = max_fds;
	etcp_setconf(&mcfg);

	etcp_register_signal(SIGINT, SignalHandler);

	flow_per_thread = total_flows;

	i = 0;
	cores[i] = i;
	done[i] = FALSE;
	flows[i] = flow_per_thread;

	if (pthread_create(&app_thread[i],
					   NULL, RunWgetMain, (void *)&cores[i]))
	{
		perror("pthread_create");
		fprintf(stderr,"Failed to create wget thread.\n");
		exit(-1);
	}

	pthread_join(app_thread[i], NULL);
	etcp_destroy();
	return 0;
}
/*----------------------------------------------------------------------------*/
