#include <sys/queue.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>

#include "etcp.h"
#include "tcp_flow.h"
#include "eventpoll.h"
#include "tcp_in.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*----------------------------------------------------------------------------*/
char *event_to_string[] = {"NONE", "IN", "PRI", "OUT", "ERR", "HUP", "RDHUP"};
/*----------------------------------------------------------------------------*/
char *
ConvertEventToString(uint32_t event)
{
	switch (event)
	{
	case etcp_EPOLLNONE:
		return event_to_string[0];
		break;
	case etcp_EPOLLIN:
		return event_to_string[1];
		break;
	case etcp_EPOLLPRI:
		return event_to_string[2];
		break;
	case etcp_EPOLLOUT:
		return event_to_string[3];
		break;
	case etcp_EPOLLERR:
		return event_to_string[4];
		break;
	case etcp_EPOLLHUP:
		return event_to_string[5];
		break;
	case etcp_EPOLLRDHUP:
		return event_to_string[6];
		break;
	default:
		assert(0);
	}

	assert(0);
	return NULL;
}
/*----------------------------------------------------------------------------*/
struct event_queue *
CreateEventQ(int size)
{
	struct event_queue *eq;

	eq = (struct event_queue *)calloc(1, sizeof(struct event_queue));
	if (!eq)
		return NULL;

	eq->start = 0;
	eq->end = 0;
	eq->size = size;
	eq->events = (struct etcp_epoll_event_int *)
		calloc(size, sizeof(struct etcp_epoll_event_int));
	if (!eq->events)
	{
		free(eq);
		return NULL;
	}
	eq->num_events = 0;

	return eq;
}
/*----------------------------------------------------------------------------*/
void DestroyEventQ(struct event_queue *eq)
{
	if (eq->events)
		free(eq->events);

	free(eq);
}
/*----------------------------------------------------------------------------*/
int etcp_epoll_create(etcp_engine_t etcp_engine, int size)
{
	flow_manager_t etcp = g_etcp[etcp_engine->cpu];
	struct etcp_epoll *ep;
	socket_map_t epsocket;

	if (size <= 0)
	{
		errno = EINVAL;
		return -1;
	}

	epsocket = MakeSocket(etcp_engine, etcp_SOCK_EPOLL, FALSE);
	if (!epsocket)
	{
		errno = ENFILE;
		return -1;
	}

	ep = (struct etcp_epoll *)calloc(1, sizeof(struct etcp_epoll));
	if (!ep)
	{
		DestroySocket(etcp_engine, epsocket->id, FALSE);
		return -1;
	}

	/* create event queues */
	ep->usr_queue = CreateEventQ(size);
	if (!ep->usr_queue)
	{
		DestroySocket(etcp_engine, epsocket->id, FALSE);
		free(ep);
		return -1;
	}

	ep->usr_shadow_queue = CreateEventQ(size);
	if (!ep->usr_shadow_queue)
	{
		DestroyEventQ(ep->usr_queue);
		DestroySocket(etcp_engine, epsocket->id, FALSE);
		free(ep);
		return -1;
	}

	ep->etcp_queue = CreateEventQ(size);
	if (!ep->etcp_queue)
	{
		DestroyEventQ(ep->usr_shadow_queue);
		DestroyEventQ(ep->usr_queue);
		DestroySocket(etcp_engine, epsocket->id, FALSE);
		free(ep);
		return -1;
	}


	etcp->ep = ep;
	epsocket->ep = ep;

	if (pthread_mutex_init(&ep->epoll_lock, NULL))
	{
		DestroyEventQ(ep->etcp_queue);
		DestroyEventQ(ep->usr_shadow_queue);
		DestroyEventQ(ep->usr_queue);
		DestroySocket(etcp_engine, epsocket->id, FALSE);
		free(ep);
		return -1;
	}
	if (pthread_cond_init(&ep->epoll_cond, NULL))
	{
		DestroyEventQ(ep->etcp_queue);
		DestroyEventQ(ep->usr_shadow_queue);
		DestroyEventQ(ep->usr_queue);
		DestroySocket(etcp_engine, epsocket->id, FALSE);
		free(ep);
		return -1;
	}

	return epsocket->id;
}
/*----------------------------------------------------------------------------*/
int CloseEpoll(etcp_engine_t etcp_engine, int epid)
{
	flow_manager_t etcp;
	struct etcp_epoll *ep;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	ep = etcp->smap[epid].ep;
	if (!ep)
	{
		errno = EINVAL;
		return -1;
	}

	DestroyEventQ(ep->usr_queue);
	DestroyEventQ(ep->usr_shadow_queue);
	DestroyEventQ(ep->etcp_queue);

	pthread_mutex_lock(&ep->epoll_lock);
	etcp->ep = NULL;
	etcp->smap[epid].ep = NULL;
	pthread_cond_signal(&ep->epoll_cond);
	pthread_mutex_unlock(&ep->epoll_lock);

	pthread_cond_destroy(&ep->epoll_cond);
	pthread_mutex_destroy(&ep->epoll_lock);
	free(ep);

	return 0;
}
/*----------------------------------------------------------------------------*/
static int
RegisterPendingEvents(flow_manager_t etcp,
						 struct etcp_epoll *ep, socket_map_t socket)
{
	tcp_flow *flow = socket->flow;

	if (!flow)
		return -1;
	if (flow->state < TCP_ST_ESTABLISHED)
		return -1;

	/* if there are payloads already read before epoll registration */
	/* generate read event */
	if (socket->epoll & etcp_EPOLLIN)
	{
		struct recv_var_tcp *rcvvar = flow->rcvvar;
		if (rcvvar->rcvbuf && rcvvar->rcvbuf->merged_len > 0)
		{
			AddEventToEpollQ(ep, USR_SHADOW_EVENT_QUEUE, socket, etcp_EPOLLIN);
		}
		else if (flow->state == TCP_ST_CLOSE_WAIT)
		{
			AddEventToEpollQ(ep, USR_SHADOW_EVENT_QUEUE, socket, etcp_EPOLLIN);
		}
	}

	/* same thing to the write event */
	if (socket->epoll & etcp_EPOLLOUT)
	{
		struct send_var_tcp *sndvar = flow->sndvar;
		if (!sndvar->sndbuf || (sndvar->sndbuf && sndvar->snd_wnd > 0))
		{
			if (!(socket->events & etcp_EPOLLOUT))
			{
				AddEventToEpollQ(ep, USR_SHADOW_EVENT_QUEUE, socket, etcp_EPOLLOUT);
			}
		}
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int etcp_epoll_ctl(etcp_engine_t etcp_engine, int epid,
				   int op, int sockid, struct etcp_epoll_event *event)
{
	flow_manager_t etcp;
	struct etcp_epoll *ep;
	socket_map_t socket;
	uint32_t events;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (epid < 0 || epid >= CONFIG.max_concurrency)
	{
		 fprintf(stderr,"Epoll id %d out of range.\n", epid);
		errno = EBADF;
		return -1;
	}

	if (sockid < 0 || sockid >= CONFIG.max_concurrency)
	{
		 fprintf(stderr,"Socket id %d out of range.\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (etcp->smap[epid].socktype == etcp_SOCK_UNUSED)
	{
		errno = EBADF;
		return -1;
	}

	if (etcp->smap[epid].socktype != etcp_SOCK_EPOLL)
	{
		errno = EINVAL;
		return -1;
	}

	ep = etcp->smap[epid].ep;
	if (!ep || (!event && op != etcp_EPOLL_CTL_DEL))
	{
		errno = EINVAL;
		return -1;
	}
	socket = &etcp->smap[sockid];

	if (op == etcp_EPOLL_CTL_ADD)
	{
		if (socket->epoll)
		{
			errno = EEXIST;
			return -1;
		}

		/* EPOLLERR and EPOLLHUP are registered as default */
		events = event->events;
		events |= (etcp_EPOLLERR | etcp_EPOLLHUP);
		socket->ep_data = event->data;
		socket->epoll = events;
		if (socket->socktype == etcp_SOCK_FLOW)
		{
			RegisterPendingEvents(etcp, ep, socket);
		}

	}
	else if (op == etcp_EPOLL_CTL_MOD)
	{
		if (!socket->epoll)
		{
			pthread_mutex_unlock(&ep->epoll_lock);
			errno = ENOENT;
			return -1;
		}

		events = event->events;
		events |= (etcp_EPOLLERR | etcp_EPOLLHUP);
		socket->ep_data = event->data;
		socket->epoll = events;

		if (socket->socktype == etcp_SOCK_FLOW)
		{
			RegisterPendingEvents(etcp, ep, socket);
		}

	}
	else if (op == etcp_EPOLL_CTL_DEL)
	{
		if (!socket->epoll)
		{
			errno = ENOENT;
			return -1;
		}

		socket->epoll = etcp_EPOLLNONE;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int etcp_epoll_wait(etcp_engine_t etcp_engine, int epid,
					struct etcp_epoll_event *events, int maxevents, int timeout)
{
	flow_manager_t etcp;
	struct etcp_epoll *ep;
	struct event_queue *eq;
	struct event_queue *eq_shadow;
	socket_map_t event_socket;
	int validity;
	int i, cnt, ret;
	int num_events;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (epid < 0 || epid >= CONFIG.max_concurrency)
	{
		errno = EBADF;
		return -1;
	}

	if (etcp->smap[epid].socktype == etcp_SOCK_UNUSED)
	{
		errno = EBADF;
		return -1;
	}

	if (etcp->smap[epid].socktype != etcp_SOCK_EPOLL)
	{
		errno = EINVAL;
		return -1;
	}

	ep = etcp->smap[epid].ep;
	if (!ep || !events || maxevents <= 0)
	{
		errno = EINVAL;
		return -1;
	}

	if (pthread_mutex_lock(&ep->epoll_lock))
	{
		if (errno == EDEADLK)
			perror("etcp_epoll_wait: epoll_lock blocked\n");
	}
wait:
	eq = ep->usr_queue;
	eq_shadow = ep->usr_shadow_queue;

	/* wait until event occurs */
	while (eq->num_events == 0 && eq_shadow->num_events == 0 && timeout != 0)
	{

		/* signal to etcp thread if it is sleeping */
		if (etcp->wakeup_flag && etcp->is_sleeping)
		{
			pthread_kill(etcp->ctx->thread, SIGUSR1);
		}
		ep->waiting = TRUE;

		if (timeout > 0)
		{
			struct timespec deadline;

			clock_gettime(CLOCK_REALTIME, &deadline);
			if (timeout >= 1000)
			{
				int sec;
				sec = timeout / 1000;
				deadline.tv_sec += sec;
				timeout -= sec * 1000;
			}

			deadline.tv_nsec += timeout * 1000000;

			if (deadline.tv_nsec >= 1000000000)
			{
				deadline.tv_sec++;
				deadline.tv_nsec -= 1000000000;
			}
			// goes to sleep
			ret = pthread_cond_timedwait(&ep->epoll_cond,
										 &ep->epoll_lock, &deadline);
			if (ret && ret != ETIMEDOUT)
			{
				/* errno set by pthread_cond_timedwait() */
				pthread_mutex_unlock(&ep->epoll_lock);
				return -1;
			}
			timeout = 0;
		}
		else if (timeout < 0)
		{
			//  goes to sleep
			ret = pthread_cond_wait(&ep->epoll_cond, &ep->epoll_lock);
			if (ret)
			{
				/* errno set by pthread_cond_wait() */
				pthread_mutex_unlock(&ep->epoll_lock);
				return -1;
			}
		}
		ep->waiting = FALSE;

		if (etcp->ctx->done || etcp->ctx->exit || etcp->ctx->interrupt)
		{
			etcp->ctx->interrupt = FALSE;
			pthread_mutex_unlock(&ep->epoll_lock);
			errno = EINTR;
			return -1;
		}
	}

	/* fetch events from the user event queue */
	cnt = 0;
	num_events = eq->num_events;
	for (i = 0; i < num_events && cnt < maxevents; i++)
	{
		event_socket = &etcp->smap[eq->events[eq->start].sockid];
		validity = TRUE;
		if (event_socket->socktype == etcp_SOCK_UNUSED)
			validity = FALSE;
		if (!(event_socket->epoll & eq->events[eq->start].ev.events))
			validity = FALSE;
		if (!(event_socket->events & eq->events[eq->start].ev.events))
			validity = FALSE;

		if (validity)
		{
			events[cnt++] = eq->events[eq->start].ev;
			assert(eq->events[eq->start].sockid >= 0);
		}
		event_socket->events &= (~eq->events[eq->start].ev.events);

		eq->start++;
		eq->num_events--;
		if (eq->start >= eq->size)
		{
			eq->start = 0;
		}
	}

	/* fetch eventes from user shadow event queue */
	eq = ep->usr_shadow_queue;
	num_events = eq->num_events;
	for (i = 0; i < num_events && cnt < maxevents; i++)
	{
		event_socket = &etcp->smap[eq->events[eq->start].sockid];
		validity = TRUE;
		if (event_socket->socktype == etcp_SOCK_UNUSED)
			validity = FALSE;
		if (!(event_socket->epoll & eq->events[eq->start].ev.events))
			validity = FALSE;
		if (!(event_socket->events & eq->events[eq->start].ev.events))
			validity = FALSE;

		if (validity)
		{
			events[cnt++] = eq->events[eq->start].ev;
			assert(eq->events[eq->start].sockid >= 0);
		}
		event_socket->events &= (~eq->events[eq->start].ev.events);

		eq->start++;
		eq->num_events--;
		if (eq->start >= eq->size)
		{
			eq->start = 0;
		}
	}

	if (cnt == 0 && timeout != 0)
		goto wait;

	pthread_mutex_unlock(&ep->epoll_lock);

	return cnt;
}
/*----------------------------------------------------------------------------*/
inline int
AddEventToEpollQ(struct etcp_epoll *ep,
			  int queue_type, socket_map_t socket, uint32_t event)
{
	struct event_queue *eq;
	int index;

	if (!ep || !socket || !event)
		return -1;

	if (socket->events & event)
	{
		return 0;
	}

	if (queue_type == etcp_EVENT_QUEUE)
	{
		eq = ep->etcp_queue;
	}
	else if (queue_type == USR_EVENT_QUEUE)
	{
		eq = ep->usr_queue;
		pthread_mutex_lock(&ep->epoll_lock);
	}
	else if (queue_type == USR_SHADOW_EVENT_QUEUE)
	{
		eq = ep->usr_shadow_queue;
	}
	else
	{
		fprintf(stderr,"Non-existing event queue type!\n");
		return -1;
	}

	if (eq->num_events >= eq->size)
	{
		fprintf(stderr,"Exceeded epoll event queue! num_events: %d, size: %d\n",
					eq->num_events, eq->size);
		if (queue_type == USR_EVENT_QUEUE)
			pthread_mutex_unlock(&ep->epoll_lock);
		return -1;
	}

	index = eq->end++;

	socket->events |= event;
	eq->events[index].sockid = socket->id;
	eq->events[index].ev.events = event;
	eq->events[index].ev.data = socket->ep_data;

	if (eq->end >= eq->size)
	{
		eq->end = 0;
	}
	eq->num_events++;


	if (queue_type == USR_EVENT_QUEUE)
		pthread_mutex_unlock(&ep->epoll_lock);
	return 0;
}
