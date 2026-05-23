#ifndef EVENTPOLL_H
#define EVENTPOLL_H

#include "etcp_api.h"
#include "etcp_epoll.h"

/*----------------------------------------------------------------------------*/
struct etcp_epoll_event_int
{
	struct etcp_epoll_event ev;
	int sockid;
};
/*----------------------------------------------------------------------------*/
enum event_queue_type
{
	USR_EVENT_QUEUE = 0, 
	USR_SHADOW_EVENT_QUEUE = 1, 
	etcp_EVENT_QUEUE = 2
};
/*----------------------------------------------------------------------------*/
struct event_queue
{
	struct etcp_epoll_event_int *events;
	int start;			// starting index
	int end;			// ending index
	
	int size;			// max size
	int num_events;		// number of events
};
/*----------------------------------------------------------------------------*/
struct etcp_epoll
{
	struct event_queue *usr_queue;
	struct event_queue *usr_shadow_queue;
	struct event_queue *etcp_queue;

	uint8_t waiting;
	
	pthread_cond_t epoll_cond;
	pthread_mutex_t epoll_lock;
};
/*----------------------------------------------------------------------------*/

int 
CloseEpoll(etcp_engine_t etcp_engine, int epid);

#endif /* EVENTPOLL_H */
