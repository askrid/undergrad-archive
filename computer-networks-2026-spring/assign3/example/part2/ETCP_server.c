#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <etcp_api.h>
#include <etcp_epoll.h>

#define MAX_FLOW_NUM (10000)

#define RCVBUF_SIZE (64 * 1024)
#define SNDBUF_SIZE (64 * 1024) 

#define MAX_EVENTS (MAX_FLOW_NUM * 3)

#define HTTP_HEADER_LEN 1024
#define URL_LEN 128

#define MAX_FILES 30

#define NAME_LIMIT 256
#define FULLNAME_LIMIT 512

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef ERROR
#define ERROR (-1)
#endif

#define HT_SUPPORT FALSE

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

static int server_port;
/*----------------------------------------------------------------------------*/
struct file_cache
{
	char name[NAME_LIMIT];
	char fullname[FULLNAME_LIMIT];
	uint64_t size;
	char *file;
};
/*----------------------------------------------------------------------------*/
struct server_vars
{
	char request[HTTP_HEADER_LEN];
	int recv_len;
	int request_len;
	long int total_read, total_sent;
	uint8_t done;
	uint8_t rspheader_sent;
	uint8_t keep_alive;

	int fidx;				// file cache index
	char fname[NAME_LIMIT]; // file name
	long int fsize;			// file size
};
/*----------------------------------------------------------------------------*/
struct thread_context
{
	etcp_engine_t etcp_engine;
	int ep;
	struct server_vars *svars;
};
/*----------------------------------------------------------------------------*/
static int core_limit;
static pthread_t app_thread[MAX_CPUS];
static int done[MAX_CPUS];
static char *conf_file = NULL;
static int backlog = -1;
/*----------------------------------------------------------------------------*/
const char *www_main;
static struct file_cache fcache[MAX_FILES];
static int nfiles;
/*----------------------------------------------------------------------------*/
static int finished;
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
/*--------------------------------------------------------------------------*/
int 
find_http_header(char *data, int len)
{
	char *temp = data;
	int hdr_len = 0;
	char ch = data[len]; /* remember it */

	/* null terminate the string first */
	data[len] = 0;
	while (!hdr_len && (temp = strchr(temp, '\n')) != NULL) {
		temp++;
		if (*temp == '\n') 
			hdr_len = temp - data + 1;
		else if (len > 0 && *temp == '\r' && *(temp + 1) == '\n') 
			hdr_len = temp - data + 2;
	}
	data[len] = ch; /* put it back */

	/* terminate the header if found */
	if (hdr_len) 
		data[hdr_len-1] = 0;

	return hdr_len;
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
char*
http_get_url(char * data, int data_len, char* value, int value_len)
{
	char *ret = data;
	char *temp;
	int i = 0;

	if (strncmp(data, HTTP_GET, sizeof(HTTP_GET)-1)) {
		*value = 0;
		return NULL;
	}
	
	ret += sizeof(HTTP_GET);
	while (*ret && SPACE_OR_TAB(*ret)) 
		ret++;

	temp = ret;
	while (*temp && *temp != ' ' && i < value_len - 1) {
		value[i++] = *temp++;
	}
	value[i] = 0;
	
	return ret;
}
/*---------------------------------------------------------------------------*/
static char *
StatusCodeToString(int scode)
{
	switch (scode)
	{
	case 200:
		return "OK";
		break;

	case 404:
		return "Not Found";
		break;
	}

	return NULL;
}
/*----------------------------------------------------------------------------*/
void CleanServerVariable(struct server_vars *sv)
{
	sv->recv_len = 0;
	sv->request_len = 0;
	sv->total_read = 0;
	sv->total_sent = 0;
	sv->done = 0;
	sv->rspheader_sent = 0;
	sv->keep_alive = 0;
}
/*----------------------------------------------------------------------------*/
void CloseConnection(struct thread_context *ctx, int sockid, struct server_vars *sv)
{
	etcp_epoll_ctl(ctx->etcp_engine, ctx->ep, etcp_EPOLL_CTL_DEL, sockid, NULL);
	etcp_close(ctx->etcp_engine, sockid);
}
/*----------------------------------------------------------------------------*/
static int
SendUntilAvailable(struct thread_context *ctx, int sockid, struct server_vars *sv)
{
	int ret;
	int sent;
	int len;

	if (sv->done || !sv->rspheader_sent)
	{
		return 0;
	}

	sent = 0;
	ret = 1;
	while (ret > 0)
	{
		len = MIN(SNDBUF_SIZE, sv->fsize - sv->total_sent);
		if (len <= 0)
		{
			break;
		}
		ret = etcp_write(ctx->etcp_engine, sockid,
						 fcache[sv->fidx].file + sv->total_sent, len);
		if (ret < 0)
		{
			// buffer space unavailable, wait for next EPOLLOUT event
			if (errno == EAGAIN)
			{
				struct etcp_epoll_event ev;
				ev.events = etcp_EPOLLIN | etcp_EPOLLOUT;
				ev.data.sockid = sockid;
				etcp_epoll_ctl(ctx->etcp_engine, ctx->ep, etcp_EPOLL_CTL_MOD,
							   sockid, &ev);
				break;
			}
			else
			{
				break;
			}
		}
		sent += ret;
		sv->total_sent += ret;
	}

	if (sv->total_sent >= fcache[sv->fidx].size)
	{
		struct etcp_epoll_event ev;
		sv->done = TRUE;
		finished++;

		if (sv->keep_alive)
		{
			/* if keep-alive connection, wait for the incoming request */
			ev.events = etcp_EPOLLIN;
			ev.data.sockid = sockid;
			etcp_epoll_ctl(ctx->etcp_engine, ctx->ep, etcp_EPOLL_CTL_MOD, sockid, &ev);
			CleanServerVariable(sv);
		}
		else
		{
			/* else, close connection */
			CloseConnection(ctx, sockid, sv);
		}
	}

	return sent;
}
/*----------------------------------------------------------------------------*/
static int
HandleReadEvent(struct thread_context *ctx, int sockid, struct server_vars *sv)
{
	struct etcp_epoll_event ev;
	char buf[HTTP_HEADER_LEN];
	char url[URL_LEN];
	char response[HTTP_HEADER_LEN];
	int scode; // status code
	time_t t_now;
	char t_str[128];
	char keepalive_str[128];
	int rd;
	int i;
	int len;
	int sent;

	/* HTTP request handling */
	rd = etcp_read(ctx->etcp_engine, sockid, buf, HTTP_HEADER_LEN);
	if (rd <= 0)
	{
		return rd;
	}
	memcpy(sv->request + sv->recv_len,
		   (char *)buf, MIN(rd, HTTP_HEADER_LEN - sv->recv_len));
	sv->recv_len += rd;
	sv->request_len = find_http_header(sv->request, sv->recv_len);
	if (sv->request_len <= 0)
	{
		fprintf(stderr,"Socket %d: Failed to parse HTTP request header.\n"
					"read bytes: %d, recv_len: %d, "
					"request_len: %d, strlen: %ld, request: \n%s\n",
					sockid, rd, sv->recv_len,
					sv->request_len, strlen(sv->request), sv->request);
		return rd;
	}

	http_get_url(sv->request, sv->request_len, url, URL_LEN);
	sprintf(sv->fname, "%s%s", www_main, url);

	sv->keep_alive = FALSE;
	if (http_header_str_val(sv->request, "Connection: ",
							strlen("Connection: "), keepalive_str, 128))
	{
		if (strstr(keepalive_str, "Keep-Alive"))
		{
			sv->keep_alive = TRUE;
		}
		else if (strstr(keepalive_str, "Close"))
		{
			sv->keep_alive = FALSE;
		}
	}

	/* Find file in cache */
	scode = 404;
	for (i = 0; i < nfiles; i++)
	{
		if (strcmp(sv->fname, fcache[i].fullname) == 0)
		{
			sv->fsize = fcache[i].size;
			sv->fidx = i;
			scode = 200;
			break;
		}
	}

	/* Response header handling */
	time(&t_now);
	strftime(t_str, 128, "%a, %d %b %Y %X GMT", gmtime(&t_now));
	if (sv->keep_alive)
		sprintf(keepalive_str, "Keep-Alive");
	else
		sprintf(keepalive_str, "Close");

	sprintf(response, "HTTP/1.1 %d %s\r\n"
					  "Date: %s\r\n"
					  "Server: Webserver on Middlebox TCP (Ubuntu)\r\n"
					  "Content-Length: %ld\r\n"
					  "Connection: %s\r\n\r\n",
			scode, StatusCodeToString(scode), t_str, sv->fsize, keepalive_str);
	len = strlen(response);
	sent = etcp_write(ctx->etcp_engine, sockid, response, len);
	assert(sent == len);
	sv->rspheader_sent = TRUE;

	ev.events = etcp_EPOLLIN | etcp_EPOLLOUT;
	ev.data.sockid = sockid;
	etcp_epoll_ctl(ctx->etcp_engine, ctx->ep, etcp_EPOLL_CTL_MOD, sockid, &ev);

	SendUntilAvailable(ctx, sockid, sv);

	return rd;
}
/*----------------------------------------------------------------------------*/
int AcceptConnection(struct thread_context *ctx, int listener)
{
	etcp_engine_t etcp_engine = ctx->etcp_engine;
	struct server_vars *sv;
	struct etcp_epoll_event ev;
	int c;

	c = etcp_accept(etcp_engine, listener, NULL, NULL); // return connected socket file descriptor

	if (c >= 0)
	{
		if (c >= MAX_FLOW_NUM)
		{
			fprintf(stderr,"Invalid socket id %d.\n", c);
			return -1;
		}

		sv = &ctx->svars[c];
		CleanServerVariable(sv);
		ev.events = etcp_EPOLLIN;
		ev.data.sockid = c;
		etcp_setsock_nonblock(ctx->etcp_engine, c);
		etcp_epoll_ctl(etcp_engine, ctx->ep, etcp_EPOLL_CTL_ADD, c, &ev);
	}
	else
	{
		if (errno != EAGAIN)
		{
			fprintf(stderr,"etcp_accept() error %s\n",
						strerror(errno));
		}
	}

	return c;
}
/*----------------------------------------------------------------------------*/
struct thread_context *
InitializeServerThread(int core)
{
	struct thread_context *ctx;

	ctx = (struct thread_context *)calloc(1, sizeof(struct thread_context));
	if (!ctx)
	{
		fprintf(stderr,"Failed to create thread context!\n");
		return NULL;
	}

	/* create etcp context: this will spawn an etcp thread */
	ctx->etcp_engine = etcp_create_context(core);
	if (!ctx->etcp_engine)
	{
		fprintf(stderr,"Failed to create etcp context!\n");
		free(ctx);
		return NULL;
	}

	/* create epoll descriptor */
	ctx->ep = etcp_epoll_create(ctx->etcp_engine, MAX_EVENTS);
	if (ctx->ep < 0)
	{
		etcp_destroy_context(ctx->etcp_engine);
		free(ctx);
		fprintf(stderr,"Failed to create epoll descriptor!\n");
		return NULL;
	}

	/* allocate memory for server variables */
	ctx->svars = (struct server_vars *)
		calloc(MAX_FLOW_NUM, sizeof(struct server_vars));
	if (!ctx->svars)
	{
		etcp_close(ctx->etcp_engine, ctx->ep);
		etcp_destroy_context(ctx->etcp_engine);
		free(ctx);
		fprintf(stderr,"Failed to create server_vars struct!\n");
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

	/* create socket and set it as nonblocking */
	listener = etcp_socket(ctx->etcp_engine, AF_INET, SOCK_STREAM, 0);
	if (listener < 0)
	{
		fprintf(stderr,"Failed to create listening socket!\n");
		return -1;
	}
	ret = etcp_setsock_nonblock(ctx->etcp_engine, listener);
	if (ret < 0)
	{
		fprintf(stderr,"Failed to set socket in nonblocking mode.\n");
		return -1;
	}

	/* bind to port 8080 */
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(server_port);
	ret = etcp_bind(ctx->etcp_engine, listener,
					(struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (ret < 0)
	{
		fprintf(stderr,"Failed to bind to the listening socket!\n");
		return -1;
	}

	/* listen (backlog: can be configured) */
	ret = etcp_listen(ctx->etcp_engine, listener, backlog);
	if (ret < 0)
	{
		fprintf(stderr,"etcp_listen() failed!\n");
		return -1;
	}

	/* wait for incoming accept events */
	ev.events = etcp_EPOLLIN;
	ev.data.sockid = listener;
	etcp_epoll_ctl(ctx->etcp_engine, ctx->ep, etcp_EPOLL_CTL_ADD, listener, &ev);

	return listener;
}
/*----------------------------------------------------------------------------*/
void *
RunServerThread(void *arg)
{
	int core = *(int *)arg;
	struct thread_context *ctx;
	etcp_engine_t etcp_engine;
	int listener;
	int ep;
	struct etcp_epoll_event *events;
	int nevents;
	int i, ret;
	int do_accept;
	/* initialization */
	ctx = InitializeServerThread(core);
	if (!ctx)
	{
		fprintf(stderr,"Failed to initialize server thread.\n");
		return NULL;
	}
	etcp_engine = ctx->etcp_engine;
	ep = ctx->ep;

	events = (struct etcp_epoll_event *)
		calloc(MAX_EVENTS, sizeof(struct etcp_epoll_event));
	if (!events)
	{
		fprintf(stderr,"Failed to create event struct!\n");
		exit(-1);
	}

	listener = CreateListeningSocket(ctx);
	if (listener < 0)
	{
		fprintf(stderr,"Failed to create listening socket.\n");
		exit(-1);
	}

	while (!done[core])
	{
		nevents = etcp_epoll_wait(etcp_engine, ep, events, MAX_EVENTS, -1);
		if (nevents < 0)
		{
			if (errno != EINTR)
				perror("etcp_epoll_wait");
			break;
		}

		do_accept = FALSE;
		for (i = 0; i < nevents; i++)
		{
			if (events[i].data.sockid == listener)
			{
				/* if the event is for the listener, accept connection */
				do_accept = TRUE;
			}
			else if (events[i].events & etcp_EPOLLERR)
			{
				int err;
				socklen_t len = sizeof(err);
				if (etcp_getsockopt(etcp_engine, events[i].data.sockid,
									SOL_SOCKET, SO_ERROR, (void *)&err, &len) == 0)
				{
					if (err != ETIMEDOUT)
					{
						fprintf(stderr, "Error on socket %d: %s\n",
								events[i].data.sockid, strerror(err));
					}
				}
				else
				{
					perror("etcp_getsockopt");
				}
				CloseConnection(ctx, events[i].data.sockid,
								&ctx->svars[events[i].data.sockid]);
			}
			else if (events[i].events & etcp_EPOLLIN)
			{
				ret = HandleReadEvent(ctx, events[i].data.sockid,
									  &ctx->svars[events[i].data.sockid]);

				if (ret == 0)
				{
					/* connection closed by remote host */
					CloseConnection(ctx, events[i].data.sockid,
									&ctx->svars[events[i].data.sockid]);
				}
				else if (ret < 0)
				{
					/* if not EAGAIN, it's an error */
					if (errno != EAGAIN)
					{
						CloseConnection(ctx, events[i].data.sockid,
										&ctx->svars[events[i].data.sockid]);
					}
				}
			}
			else if (events[i].events & etcp_EPOLLOUT)
			{
				struct server_vars *sv = &ctx->svars[events[i].data.sockid];
				if (sv->rspheader_sent)
				{
					SendUntilAvailable(ctx, events[i].data.sockid, sv);
				}
			}
		}

		/* if do_accept flag is set, accept connections */
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

	/* destroy etcp context: this will kill the etcp thread */
	etcp_destroy_context(etcp_engine);
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
			{
				pthread_kill(app_thread[i], signum);
			}
		}
	}
}
/*----------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	DIR *dir;
	struct dirent *ent;
	int fd;
	int ret;
	uint64_t total_read;
	struct etcp_conf mcfg;
	int cores[MAX_CPUS];
	int o;
	core_limit = 1;
	dir = NULL;
	server_port = atoi(argv[1]);
	etcp_getconf(&mcfg);
	mcfg.num_cores = 1;
	etcp_setconf(&mcfg);

	while (-1 != (o = getopt(argc, argv, "P:N:f:p:c:b:h")))
	{
		switch (o)
		{
		case 'p':
			/* open the directory to serve */
			www_main = optarg;
			dir = opendir(www_main);
			if (!dir)
			{
				perror("opendir");
				return FALSE;
			}
			break;
		case 'f':
			conf_file = optarg;
			break;
		case 'b':
			backlog = mystrtol(optarg, 10);
			break;
		}
	}

	if (dir == NULL)
	{
		exit(EXIT_FAILURE);
	}

	nfiles = 0;
	while ((ent = readdir(dir)) != NULL)
	{
		if (strcmp(ent->d_name, ".") == 0)
			continue;
		else if (strcmp(ent->d_name, "..") == 0)
			continue;

		snprintf(fcache[nfiles].name, NAME_LIMIT, "%s", ent->d_name);
		snprintf(fcache[nfiles].fullname, FULLNAME_LIMIT, "%s/%s",
				 www_main, ent->d_name);
		fd = open(fcache[nfiles].fullname, O_RDONLY);
		if (fd < 0)
		{
			perror("open");
			continue;
		}
		else
		{
			fcache[nfiles].size = lseek64(fd, 0, SEEK_END);
			lseek64(fd, 0, SEEK_SET);
		}

		fcache[nfiles].file = (char *)malloc(fcache[nfiles].size);
		if (!fcache[nfiles].file)
		{
			perror("malloc");
			continue;
		}
		total_read = 0;
		while (1)
		{
			ret = read(fd, fcache[nfiles].file + total_read,
					   fcache[nfiles].size - total_read);
			if (ret < 0)
			{
				break;
			}
			else if (ret == 0)
			{
				break;
			}
			total_read += ret;
		}
		if (total_read < fcache[nfiles].size)
		{
			free(fcache[nfiles].file);
			continue;
		}
		close(fd);
		nfiles++;

		if (nfiles >= MAX_FILES)
			break;
	}

	finished = 0;

	/* initialize etcp */
	if (conf_file == NULL)
	{
		exit(EXIT_FAILURE);
	}

	/**************************************************************************************/
	ret = etcp_init(conf_file);
	if (ret)
	{
		exit(EXIT_FAILURE);
	}

	etcp_getconf(&mcfg);
	if (backlog > mcfg.max_concurrency)
	{
		return FALSE;
	}

	/* if backlog is not specified, set it to 4K */
	if (backlog == -1)
	{
		backlog = 4096;
	}

	/* register signal handler to etcp */
	etcp_register_signal(SIGINT, SignalHandler);
	cores[0] = 0;
	done[0] = FALSE;
	// make application thread here 
	if (pthread_create(&app_thread[0],
					   NULL, RunServerThread, (void *)&cores[0]))
	{
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}
	pthread_join(app_thread[0], NULL);	
	etcp_destroy();
	closedir(dir);
	return 0;
}
