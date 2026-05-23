#ifndef etcp_API_H
#define etcp_API_H

#include <stdint.h>
#include <netinet/in.h>
#include <sys/uio.h>

#ifndef UNUSED
#define UNUSED(x)	(void)x
#endif

#ifndef INPORT_ANY
#define INPORT_ANY	(uint16_t)0
#endif

// #ifdef __cplusplus
// extern "C" {
// #endif

enum socket_type
{
	etcp_SOCK_UNUSED, 
	etcp_SOCK_FLOW, 
	etcp_SOCK_PROXY, 
	etcp_SOCK_LISTENER, 
	etcp_SOCK_EPOLL, 
	etcp_SOCK_PIPE, 
};

struct etcp_conf
{
	int num_cores;
	int max_concurrency;

	int max_num_buffers;
	int rcvbuf_size;
	int sndbuf_size;

	int tcp_timewait;
	int tcp_timeout;
};

typedef struct etcp_engine *etcp_engine_t;

int 
etcp_init(const char *config_file);

void 
etcp_destroy();

int 
etcp_getconf(struct etcp_conf *conf);

int 
etcp_setconf(const struct etcp_conf *conf);

// int 
// etcp_core_affinitize(int cpu);

etcp_engine_t 
etcp_create_context(int cpu);

void 
etcp_destroy_context(etcp_engine_t etcp_engine);

typedef void (*etcp_sighandler_t)(int);

etcp_sighandler_t 
etcp_register_signal(int signum, etcp_sighandler_t handler);

int 
etcp_getsockopt(etcp_engine_t etcp_engine, int sockid, int level, 
		int optname, void *optval, socklen_t *optlen);

int 
etcp_setsockopt(etcp_engine_t etcp_engine, int sockid, int level, 
		int optname, const void *optval, socklen_t optlen);

int 
etcp_setsock_nonblock(etcp_engine_t etcp_engine, int sockid);

int 
etcp_socket(etcp_engine_t etcp_engine, int domain, int type, int protocol);

int 
etcp_bind(etcp_engine_t etcp_engine, int sockid, 
		const struct sockaddr *addr, socklen_t addrlen);

int 
etcp_listen(etcp_engine_t etcp_engine, int sockid, int backlog);

int 
etcp_accept(etcp_engine_t etcp_engine, int sockid, struct sockaddr *addr, socklen_t *addrlen);

int 
etcp_init_rss(etcp_engine_t etcp_engine, in_addr_t saddr_base, int num_addr, 
		in_addr_t daddr, in_addr_t dport);

int 
etcp_connect(etcp_engine_t etcp_engine, int sockid, 
		const struct sockaddr *addr, socklen_t addrlen);

int 
etcp_close(etcp_engine_t etcp_engine, int sockid);
	
ssize_t
etcp_read(etcp_engine_t etcp_engine, int sockid, char *buf, size_t len);

ssize_t
etcp_recv(etcp_engine_t etcp_engine, int sockid, char *buf, size_t len, int flags);

ssize_t
etcp_write(etcp_engine_t etcp_engine, int sockid, const char *buf, size_t len);


#endif /* etcp_API_H */
