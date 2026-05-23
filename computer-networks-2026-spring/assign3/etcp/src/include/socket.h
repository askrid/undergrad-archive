#ifndef SOCKET_H
#define SOCKET_H

#include "etcp_api.h"
#include "etcp_epoll.h"

/*----------------------------------------------------------------------------*/
enum socket_opts
{
	etcp_NONBLOCK		= 0x01,
	etcp_ADDR_BIND		= 0x02, 
};
/*----------------------------------------------------------------------------*/
struct socket_map
{
	int id;
	int socktype;
	uint32_t opts;

	struct sockaddr_in saddr;

	union {
		struct tcp_flow *flow;
		struct tcp_listener *listener;
		struct etcp_epoll *ep;
		struct pipe *pp;
	};

	uint32_t epoll;			/* registered events */
	uint32_t events;		/* available events */
	etcp_epoll_data_t ep_data;

	TAILQ_ENTRY (socket_map) free_smap_link;

};
/*----------------------------------------------------------------------------*/
typedef struct socket_map * socket_map_t;
/*----------------------------------------------------------------------------*/
socket_map_t 
MakeSocket(etcp_engine_t etcp_engine, int socktype, int need_lock);
/*----------------------------------------------------------------------------*/
void 
DestroySocket(etcp_engine_t etcp_engine, int sockid, int need_lock); 
/*----------------------------------------------------------------------------*/
socket_map_t 
ReturnSocket(etcp_engine_t etcp_engine, int sockid);
/*----------------------------------------------------------------------------*/
struct tcp_listener
{
	int sockid;
	socket_map_t socket;

	int backlog;
	flow_queue_t acceptq;
	
	pthread_mutex_t accept_lock;
	pthread_cond_t accept_cond;

	TAILQ_ENTRY(tcp_listener) he_link;	/* hash table entry link */
};
/*----------------------------------------------------------------------------*/

#endif /* SOCKET_H */
