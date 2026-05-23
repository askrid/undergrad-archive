#ifndef etcp_EPOLL_H
#define etcp_EPOLL_H

#include "etcp_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
enum etcp_epoll_op
{
	etcp_EPOLL_CTL_ADD = 1, 
	etcp_EPOLL_CTL_DEL = 2, 
	etcp_EPOLL_CTL_MOD = 3, 
};
/*----------------------------------------------------------------------------*/
enum etcp_event_type
{
	etcp_EPOLLNONE	= 0x000, 
	etcp_EPOLLIN	= 0x001, 
	etcp_EPOLLPRI	= 0x002,
	etcp_EPOLLOUT	= 0x004,
	etcp_EPOLLRDNORM	= 0x040, 
	etcp_EPOLLRDBAND	= 0x080, 
	etcp_EPOLLWRNORM	= 0x100, 
	etcp_EPOLLWRBAND	= 0x200, 
	etcp_EPOLLMSG		= 0x400, 
	etcp_EPOLLERR		= 0x008,
	etcp_EPOLLHUP		= 0x010,
	etcp_EPOLLRDHUP 	= 0x2000,
	etcp_EPOLLONESHOT	= (1 << 30), 
	etcp_EPOLLET		= (1 << 31)
};
/*----------------------------------------------------------------------------*/
typedef union etcp_epoll_data
{
	void *ptr;
	int sockid;
	uint32_t u32;
	uint64_t u64;
} etcp_epoll_data_t;
/*----------------------------------------------------------------------------*/
struct etcp_epoll_event
{
	uint32_t events;
	etcp_epoll_data_t data;
};
/*----------------------------------------------------------------------------*/
int 
etcp_epoll_create(etcp_engine_t etcp_engine, int size);
/*----------------------------------------------------------------------------*/
int
etcp_epoll_ctl(etcp_engine_t etcp_engine, int epid, 
		int op, int sockid, struct etcp_epoll_event *event);
/*----------------------------------------------------------------------------*/
int 
etcp_epoll_wait(etcp_engine_t etcp_engine, int epid, 
		struct etcp_epoll_event *events, int maxevents, int timeout);
/*----------------------------------------------------------------------------*/
char * 
ConvertEventToString(uint32_t event);
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
};
#endif

#endif /* etcp_EPOLL_H */
