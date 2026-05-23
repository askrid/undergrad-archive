#include <sys/queue.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include "etcp.h"
#include "etcp_api.h"
#include "tcp_in.h"
#include "tcp_flow.h"
#include "tcp_out.h"
#include "ip_out.h"
#include "eventpoll.h"
#include "etcp_hash.h"
#include "addr.h"
#include "config.h"


#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
/*----------------------------------------------------------------------------*/
inline flow_manager_t
GetFlowManager(etcp_engine_t etcp_engine)
{
	if (!etcp_engine)
	{
		errno = EINVAL;
		return NULL;
	}

	if (etcp_engine->cpu < 0 || etcp_engine->cpu >= num_cpus)
	{
		errno = EINVAL;
		return NULL;
	}

	if (g_etcp[etcp_engine->cpu]->ctx->done || g_etcp[etcp_engine->cpu]->ctx->exit)
	{
		errno = EPERM;
		return NULL;
	}

	return g_etcp[etcp_engine->cpu];
}
/*----------------------------------------------------------------------------*/
static inline int
ReturnSocketError(socket_map_t socket, void *optval, socklen_t *optlen)
{
	tcp_flow *cur_flow;

	if (!socket->flow)
	{
		errno = EBADF;
		return -1;
	}

	cur_flow = socket->flow;
	if (cur_flow->state == TCP_ST_CLOSED)
	{
		if (cur_flow->close_reason == TCP_TIMEDOUT ||
			cur_flow->close_reason == TCP_CONN_FAIL ||
			cur_flow->close_reason == TCP_CONN_LOST)
		{
			*(int *)optval = ETIMEDOUT;
			*optlen = sizeof(int);

			return 0;
		}
	}

	if (cur_flow->state == TCP_ST_CLOSE_WAIT ||
		cur_flow->state == TCP_ST_CLOSED)
	{
		if (cur_flow->close_reason == TCP_RESET)
		{
			*(int *)optval = ECONNRESET;
			*optlen = sizeof(int);

			return 0;
		}
	}

	if (cur_flow->state == TCP_ST_SYN_SENT &&
		errno == EINPROGRESS)
	{
		*(int *)optval = errno;
		*optlen = sizeof(int);
		return -1;
	}

	/*
	 * `base case`: If socket sees no so_error, then
	 * this also means close_reason will always be
	 * TCP_NOT_CLOSED.
	 */
	if (cur_flow->close_reason == TCP_NOT_CLOSED)
	{
		*(int *)optval = 0;
		*optlen = sizeof(int);

		return 0;
	}

	errno = ENOSYS;
	return -1;
}
/*----------------------------------------------------------------------------*/
int etcp_getsockopt(etcp_engine_t etcp_engine, int sockid, int level,
					int optname, void *optval, socklen_t *optlen)
{
	flow_manager_t etcp;
	socket_map_t socket;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (sockid < 0 || sockid >= CONFIG.max_concurrency)
	{
		 fprintf(stderr,"Socket id %d out of range.\n", sockid);
		errno = EBADF;
		return -1;
	}

	socket = &etcp->smap[sockid];
	if (socket->socktype == etcp_SOCK_UNUSED)
	{
		 fprintf(stderr,"Invalid socket id: %d\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (socket->socktype != etcp_SOCK_LISTENER &&
		socket->socktype != etcp_SOCK_FLOW)
	{
		 fprintf(stderr,"Invalid socket id: %d\n", sockid);
		errno = ENOTSOCK;
		return -1;
	}

	if (level == SOL_SOCKET)
	{
		if (optname == SO_ERROR)
		{
			if (socket->socktype == etcp_SOCK_FLOW)
			{
				return ReturnSocketError(socket, optval, optlen);
			}
		}
	}

	errno = ENOSYS;
	return -1;
}
/*----------------------------------------------------------------------------*/
int etcp_setsockopt(etcp_engine_t etcp_engine, int sockid, int level,
					int optname, const void *optval, socklen_t optlen)
{
	flow_manager_t etcp;
	socket_map_t socket;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (sockid < 0 || sockid >= CONFIG.max_concurrency)
	{
		 fprintf(stderr,"Socket id %d out of range.\n", sockid);
		errno = EBADF;
		return -1;
	}

	socket = &etcp->smap[sockid];
	if (socket->socktype == etcp_SOCK_UNUSED)
	{
		 fprintf(stderr,"Invalid socket id: %d\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (socket->socktype != etcp_SOCK_LISTENER &&
		socket->socktype != etcp_SOCK_FLOW)
	{
		 fprintf(stderr,"Invalid socket id: %d\n", sockid);
		errno = ENOTSOCK;
		return -1;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int etcp_setsock_nonblock(etcp_engine_t etcp_engine, int sockid)
{
	flow_manager_t etcp;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (sockid < 0 || sockid >= CONFIG.max_concurrency)
	{
		 fprintf(stderr,"Socket id %d out of range.\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (etcp->smap[sockid].socktype == etcp_SOCK_UNUSED)
	{
		 fprintf(stderr,"Invalid socket id: %d\n", sockid);
		errno = EBADF;
		return -1;
	}

	etcp->smap[sockid].opts |= etcp_NONBLOCK;

	return 0;
}
/*----------------------------------------------------------------------------*/
int etcp_socket(etcp_engine_t etcp_engine, int domain, int type, int protocol)
{
	flow_manager_t etcp;
	socket_map_t socket;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (domain != AF_INET)
	{
		errno = EAFNOSUPPORT;
		return -1;
	}

	if (type == SOCK_STREAM)
	{
		type = (int)etcp_SOCK_FLOW;
	}
	else
	{
		errno = EINVAL;
		return -1;
	}

	socket = MakeSocket(etcp_engine, type, FALSE);
	if (!socket)
	{
		errno = ENFILE;
		return -1;
	}

	return socket->id;
}
/*----------------------------------------------------------------------------*/
int etcp_bind(etcp_engine_t etcp_engine, int sockid,
			  const struct sockaddr *addr, socklen_t addrlen)
{
	flow_manager_t etcp;
	struct sockaddr_in *addr_in;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (sockid < 0 || sockid >= CONFIG.max_concurrency)
	{
		 fprintf(stderr,"Socket id %d out of range.\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (etcp->smap[sockid].socktype == etcp_SOCK_UNUSED)
	{
		 fprintf(stderr,"Invalid socket id: %d\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (etcp->smap[sockid].socktype != etcp_SOCK_FLOW &&
		etcp->smap[sockid].socktype != etcp_SOCK_LISTENER)
	{
		 fprintf(stderr,"Not a flow socket id: %d\n", sockid);
		errno = ENOTSOCK;
		return -1;
	}

	if (!addr)
	{
		 fprintf(stderr,"Socket %d: empty address!\n", sockid);
		errno = EINVAL;
		return -1;
	}

	if (etcp->smap[sockid].opts & etcp_ADDR_BIND)
	{
		 fprintf(stderr,"Socket %d: adress already bind for this socket.\n", sockid);
		errno = EINVAL;
		return -1;
	}

	/* we only allow bind() for AF_INET address */
	if (addr->sa_family != AF_INET || addrlen < sizeof(struct sockaddr_in))
	{
		 fprintf(stderr,"Socket %d: invalid argument!\n", sockid);
		errno = EINVAL;
		return -1;
	}

	/* TODO: validate whether the address is already being used */

	addr_in = (struct sockaddr_in *)addr;
	etcp->smap[sockid].saddr = *addr_in;
	etcp->smap[sockid].opts |= etcp_ADDR_BIND;

	return 0;
}
/*----------------------------------------------------------------------------*/
int etcp_listen(etcp_engine_t etcp_engine, int sockid, int backlog)
{
	flow_manager_t etcp;
	struct tcp_listener *listener;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (sockid < 0 || sockid >= CONFIG.max_concurrency)
	{
		 fprintf(stderr,"Socket id %d out of range.\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (etcp->smap[sockid].socktype == etcp_SOCK_UNUSED)
	{
		 fprintf(stderr,"Invalid socket id: %d\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (etcp->smap[sockid].socktype == etcp_SOCK_FLOW)
	{
		etcp->smap[sockid].socktype = etcp_SOCK_LISTENER;
	}

	if (etcp->smap[sockid].socktype != etcp_SOCK_LISTENER)
	{
		 fprintf(stderr,"Not a listening socket. id: %d\n", sockid);
		errno = ENOTSOCK;
		return -1;
	}

	if (backlog <= 0 || backlog > CONFIG.max_concurrency)
	{
		errno = EINVAL;
		return -1;
	}

	/* check whether we are not already listening on the same port */
	if (SearchListnerHT(etcp->listeners,
						 &etcp->smap[sockid].saddr.sin_port))
	{
		errno = EADDRINUSE;
		return -1;
	}

	listener = (struct tcp_listener *)calloc(1, sizeof(struct tcp_listener));
	if (!listener)
	{
		/* errno set from the malloc() */
		return -1;
	}

	listener->sockid = sockid;
	listener->backlog = backlog;
	listener->socket = &etcp->smap[sockid];

	if (pthread_cond_init(&listener->accept_cond, NULL))
	{
		/* errno set internally */
		perror("pthread_cond_init of ctx->accept_cond\n");
		free(listener);
		return -1;
	}
	if (pthread_mutex_init(&listener->accept_lock, NULL))
	{
		/* errno set internally */
		perror("pthread_mutex_init of ctx->accept_lock\n");
		free(listener);
		return -1;
	}

	listener->acceptq = CreateFlowQueue(backlog);
	if (!listener->acceptq)
	{
		free(listener);
		errno = ENOMEM;
		return -1;
	}

	etcp->smap[sockid].listener = listener;
	InsertListnerHT(etcp->listeners, listener);

	return 0;
}
/*----------------------------------------------------------------------------*/
int etcp_accept(etcp_engine_t etcp_engine, int sockid, struct sockaddr *addr, socklen_t *addrlen)
{
	flow_manager_t etcp;
	struct tcp_listener *listener;
	socket_map_t socket;
	tcp_flow *accepted = NULL;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (sockid < 0 || sockid >= CONFIG.max_concurrency)
	{
		 fprintf(stderr,"Socket id %d out of range.\n", sockid);
		errno = EBADF;
		return -1;
	}

	/* requires listening socket */
	if (etcp->smap[sockid].socktype != etcp_SOCK_LISTENER)
	{
		errno = EINVAL;
		return -1;
	}

	listener = etcp->smap[sockid].listener;

	/* dequeue from the acceptq without lock first */
	/* if nothing there, acquire lock and cond_wait */
	accepted = DequeueFromFlowQ(listener->acceptq);
	if (!accepted)
	{
		if (listener->socket->opts & etcp_NONBLOCK)
		{
			errno = EAGAIN;
			return -1;
		}
		else
		{
			pthread_mutex_lock(&listener->accept_lock);
			while ((accepted = DequeueFromFlowQ(listener->acceptq)) == NULL)
			{
				pthread_cond_wait(&listener->accept_cond, &listener->accept_lock);

				if (etcp->ctx->done || etcp->ctx->exit)
				{
					pthread_mutex_unlock(&listener->accept_lock);
					errno = EINTR;
					return -1;
				}
			}
			pthread_mutex_unlock(&listener->accept_lock);
		}
	}
	if (!accepted->socket)
	{
		socket = MakeSocket(etcp_engine, etcp_SOCK_FLOW, FALSE);
		if (!socket)
		{
			fprintf(stderr,"Failed to create new socket!\n");
			errno = ENFILE;
			return -1;
		}
		socket->flow = accepted;
		accepted->socket = socket;

		/* set socket parameters */
		socket->saddr.sin_family = AF_INET;
		socket->saddr.sin_port = accepted->dport;
		socket->saddr.sin_addr.s_addr = accepted->daddr;
	}

	if (!(listener->socket->epoll & etcp_EPOLLET) &&
		!FlowQueueIsEmpty(listener->acceptq))
		AddEventToEpollQ(etcp->ep,
					  USR_SHADOW_EVENT_QUEUE,
					  listener->socket, etcp_EPOLLIN);

	if (addr && addrlen)
	{
		struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
		addr_in->sin_family = AF_INET;
		addr_in->sin_port = accepted->dport;
		addr_in->sin_addr.s_addr = accepted->daddr;
		*addrlen = sizeof(struct sockaddr_in);
	}

	return accepted->socket->id;
}
/*----------------------------------------------------------------------------*/
int etcp_init_rss(etcp_engine_t etcp_engine, in_addr_t saddr_base, int num_addr,
				  in_addr_t daddr, in_addr_t dport)
{
	flow_manager_t etcp;
	addr_pool_t ap;
	uint8_t is_external;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		errno = EACCES;
		return -1;
	}

	if (etcp->ap)
	{
		RemoveAddrPool(etcp->ap);
		etcp->ap = NULL;
	}

	if (saddr_base == INADDR_ANY)
	{
		int nif_out, eidx;

		/* for the INADDR_ANY, find the output interface for the destination
		   and set the saddr_base as the ip address of the output interface */
		nif_out = FetchNetworkInterface(daddr, &is_external);
		if (nif_out < 0)
		{
			errno = EINVAL;
			perror("Could not determine nif idx!\n");
			return -1;
		}
		eidx = CONFIG.nif_to_eidx[nif_out];
		saddr_base = CONFIG.eths[eidx].ip_addr;
	}

	ap = AllocAddrPoolPerCore(etcp_engine->cpu, num_cpus,
								  saddr_base, num_addr, daddr, dport);
	if (!ap)
	{
		errno = ENOMEM;
		return -1;
	}

	etcp->ap = ap;
	UNUSED(is_external);
	return 0;
}
/*----------------------------------------------------------------------------*/
int etcp_connect(etcp_engine_t etcp_engine, int sockid,
				 const struct sockaddr *addr, socklen_t addrlen)
{
	flow_manager_t etcp;
	socket_map_t socket;
	tcp_flow *cur_flow;
	struct sockaddr_in *addr_in;
	in_addr_t dip;
	in_port_t dport;
	int is_dyn_bound = FALSE;
	int ret, nif;
	int is_passive_open;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (sockid < 0 || sockid >= CONFIG.max_concurrency)
	{
		 fprintf(stderr,"Socket id %d out of range.\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (etcp->smap[sockid].socktype == etcp_SOCK_UNUSED)
	{
		 fprintf(stderr,"Invalid socket id: %d\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (etcp->smap[sockid].socktype != etcp_SOCK_FLOW)
	{
		 fprintf(stderr,"Not an end socket. id: %d\n", sockid);
		errno = ENOTSOCK;
		return -1;
	}

	if (!addr)
	{
		 fprintf(stderr,"Socket %d: empty address!\n", sockid);
		errno = EFAULT;
		return -1;
	}

	/* we only allow bind() for AF_INET address */
	if (addr->sa_family != AF_INET || addrlen < sizeof(struct sockaddr_in))
	{
		 fprintf(stderr,"Socket %d: invalid argument!\n", sockid);
		errno = EAFNOSUPPORT;
		return -1;
	}

	socket = &etcp->smap[sockid];
	if (socket->flow)
	{
		 fprintf(stderr,"Socket %d: flow already exist!\n", sockid);
		if (socket->flow->state >= TCP_ST_ESTABLISHED)
		{
			errno = EISCONN;
		}
		else
		{
			errno = EALREADY;
		}
		return -1;
	}

	addr_in = (struct sockaddr_in *)addr;
	dip = addr_in->sin_addr.s_addr;
	dport = addr_in->sin_port;

	/* address binding */
	if ((socket->opts & etcp_ADDR_BIND) &&
		socket->saddr.sin_port != INPORT_ANY &&
		socket->saddr.sin_addr.s_addr != INADDR_ANY)
	{
		int rss_core;
		rss_core = etcp_engine->cpu;
		if (rss_core != etcp_engine->cpu)
		{
			errno = EINVAL;
			return -1;
		}
	}
	else
	{
		if (etcp->ap)
		{
			ret = GetAddrPerCore(etcp->ap,
									  etcp_engine->cpu, num_queues, addr_in, &socket->saddr);
		}
		else
		{
			uint8_t is_external;
			nif = FetchNetworkInterface(dip, &is_external);
			if (nif < 0)
			{
				errno = EINVAL;
				return -1;
			}
			ret = GetAddr(ap[nif],
							   etcp_engine->cpu, num_queues, addr_in, &socket->saddr);
			UNUSED(is_external);
		}
		if (ret < 0)
		{
			errno = EAGAIN;
			return -1;
		}
		socket->opts |= etcp_ADDR_BIND;
		is_dyn_bound = TRUE;
	}

	is_passive_open = FALSE;
	cur_flow = CreateFlow(etcp, socket,is_passive_open,
								 socket->saddr.sin_addr.s_addr, socket->saddr.sin_port, dip, dport);
	if (!cur_flow)
	{
		fprintf(stderr,"Socket %d: failed to create tcp_flow!\n", sockid);
		errno = ENOMEM;
		return -1;
	}

	if (is_dyn_bound)
		cur_flow->is_bound_addr = TRUE;

	cur_flow->sndvar->cwnd = 1;
	cur_flow->sndvar->ssthresh = cur_flow->sndvar->mss * 10;


	ret = EnqueueToFlowQ(etcp->connectq, cur_flow);
	etcp->wakeup_flag = TRUE;
	if (ret < 0)
	{
		fprintf(stderr,"Socket %d: failed to enqueue to conenct queue!\n", sockid);
		EnqueueToFlowQ(etcp->destroyq, cur_flow);
		errno = EAGAIN;
		return -1;
	}

	/* if nonblocking socket, return EINPROGRESS */
	if (socket->opts & etcp_NONBLOCK)
	{
		errno = EINPROGRESS;
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static inline int
Closeflow(etcp_engine_t etcp_engine, int sockid)
{
	flow_manager_t etcp;
	tcp_flow *cur_flow;
	int ret;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	cur_flow = etcp->smap[sockid].flow;
	if (!cur_flow)
	{
		errno = ENOTCONN;
		return -1;
	}

	if (cur_flow->closed)
	{
		return 0;
	}
	cur_flow->closed = TRUE;
	cur_flow->socket = NULL;

	if (cur_flow->state == TCP_ST_CLOSED)
	{
		EnqueueToFlowQ(etcp->destroyq, cur_flow);
		etcp->wakeup_flag = TRUE;
		return 0;
	}
	else if (cur_flow->state == TCP_ST_SYN_SENT)
	{
		EnqueueToFlowQ(etcp->destroyq, cur_flow);
		etcp->wakeup_flag = TRUE;
		return -1;
	}
	else if (cur_flow->state != TCP_ST_ESTABLISHED &&
			 cur_flow->state != TCP_ST_CLOSE_WAIT)
	{
		errno = EBADF;
		return -1;
	}

	cur_flow->sndvar->on_closeq = TRUE;
	ret = EnqueueToFlowQ(etcp->closeq, cur_flow);
	etcp->wakeup_flag = TRUE;

	if (ret < 0)
	{
		fprintf(stderr,"(NEVER HAPPEN) Failed to enqueue the flow to close.\n");
		errno = EAGAIN;
		return -1;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static inline int
CloseListner(etcp_engine_t etcp_engine, int sockid)
{
	flow_manager_t etcp;
	struct tcp_listener *listener;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	listener = etcp->smap[sockid].listener;
	if (!listener)
	{
		errno = EINVAL;
		return -1;
	}

	if (listener->acceptq)
	{
		DestroyFlowQueue(listener->acceptq);
		listener->acceptq = NULL;
	}

	pthread_mutex_lock(&listener->accept_lock);
	pthread_cond_signal(&listener->accept_cond);
	pthread_mutex_unlock(&listener->accept_lock);

	pthread_cond_destroy(&listener->accept_cond);
	pthread_mutex_destroy(&listener->accept_lock);

	free(listener);
	etcp->smap[sockid].listener = NULL;

	return 0;
}
/*----------------------------------------------------------------------------*/
int etcp_close(etcp_engine_t etcp_engine, int sockid)
{
	flow_manager_t etcp;
	int ret;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (sockid < 0 || sockid >= CONFIG.max_concurrency)
	{
		 fprintf(stderr,"Socket id %d out of range.\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (etcp->smap[sockid].socktype == etcp_SOCK_UNUSED)
	{
		 fprintf(stderr,"Invalid socket id: %d\n", sockid);
		errno = EBADF;
		return -1;
	}

	 fprintf(stderr,"Socket %d: etcp_close called.\n", sockid);

	switch (etcp->smap[sockid].socktype)
	{
	case etcp_SOCK_FLOW:
		ret = Closeflow(etcp_engine, sockid);
		break;

	case etcp_SOCK_LISTENER:
		ret = CloseListner(etcp_engine, sockid);
		break;

	case etcp_SOCK_EPOLL:
		ret = CloseEpoll(etcp_engine, sockid);
		break;

	default:
		errno = EINVAL;
		ret = -1;
		break;
	}

	DestroySocket(etcp_engine, sockid, FALSE);

	return ret;
}
/*----------------------------------------------------------------------------*/
static inline int
CopyDataToUserBuffer(flow_manager_t etcp, tcp_flow *cur_flow, char *buf, int len)
{
	struct recv_var_tcp *rcvvar = cur_flow->rcvvar;
	int copylen;

	copylen = MIN(rcvvar->rcvbuf->merged_len, len);
	if (copylen <= 0)
	{
		errno = EAGAIN;
		return -1;
	}

	/* Copy data to user buffer and remove it from receiving buffer */
	memcpy(buf, rcvvar->rcvbuf->head, copylen);
	RemoveFromReceiveBuffer(rcvvar->rcvbuf, copylen, AT_APP);
	rcvvar->rcv_wnd = rcvvar->rcvbuf->size - rcvvar->rcvbuf->merged_len;

	/* Advertise newly freed receive buffer */
	if (cur_flow->need_wnd_adv)
	{
		if (rcvvar->rcv_wnd > cur_flow->sndvar->mss)
		{
			if (!cur_flow->sndvar->on_ackq)
			{
				cur_flow->sndvar->on_ackq = TRUE;
				EnqueueToFlowQ(etcp->ackq, cur_flow); /* this always success */
				cur_flow->need_wnd_adv = FALSE;
				etcp->wakeup_flag = TRUE;
			}
		}
	}
	return copylen;
}
/*----------------------------------------------------------------------------*/
ssize_t
etcp_read(etcp_engine_t etcp_engine, int sockid, char *buf, size_t len)
{
	flow_manager_t etcp;
	socket_map_t socket;
	tcp_flow *cur_flow;
	struct recv_var_tcp *rcvvar;
	int paylaod_remaining;
	int ret;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (sockid < 0 || sockid >= CONFIG.max_concurrency)
	{
		 fprintf(stderr,"Socket id %d out of range.\n", sockid);
		errno = EBADF;
		return -1;
	}

	socket = &etcp->smap[sockid];
	if (socket->socktype == etcp_SOCK_UNUSED)
	{
		 fprintf(stderr,"Invalid socket id: %d\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (socket->socktype != etcp_SOCK_FLOW)
	{
		 fprintf(stderr,"Not an end socket. id: %d\n", sockid);
		errno = ENOTSOCK;
		return -1;
	}

	/* flow should be in ESTABLISHED, FIN_WAIT_1, FIN_WAIT_2, CLOSE_WAIT */
	cur_flow = socket->flow;
	if (!cur_flow ||
		!(cur_flow->state >= TCP_ST_ESTABLISHED &&
		  cur_flow->state <= TCP_ST_CLOSE_WAIT))
	{
		errno = ENOTCONN;
		return -1;
	}

	rcvvar = cur_flow->rcvvar;

	/* if CLOSE_WAIT, return 0 if there is no payload */
	/* flow got FIN packet from the peer, so state is CLOSE_WAIT */
	if (cur_flow->state == TCP_ST_CLOSE_WAIT)
	{
		if (!rcvvar->rcvbuf)
			return 0;
		/* if FIN comes with rest of payload then need to get out of this block */
		if (rcvvar->rcvbuf->merged_len == 0)
			return 0;
	}

	/* return EAGAIN if no receive buffer */
	if (socket->opts & etcp_NONBLOCK)
	{
		if (!rcvvar->rcvbuf || rcvvar->rcvbuf->merged_len == 0)
		{
			errno = EAGAIN;
			return -1;
		}
	}

	pthread_mutex_lock(&rcvvar->read_lock);

	ret = CopyDataToUserBuffer(etcp, cur_flow, buf, len);
	paylaod_remaining = FALSE;
	/* if there are remaining payload, generate EPOLLIN */
	/* (may due to insufficient user buffer) */
	if (socket->epoll & etcp_EPOLLIN)
	{
		if (!(socket->epoll & etcp_EPOLLET) && rcvvar->rcvbuf->merged_len > 0)
		{
			paylaod_remaining = TRUE;
		}
	}

	/* if waiting for close, notify it if no remaining data */
	if (cur_flow->state == TCP_ST_CLOSE_WAIT &&
		rcvvar->rcvbuf->merged_len == 0 && ret > 0)
	{
		paylaod_remaining = TRUE;
	}

	pthread_mutex_unlock(&rcvvar->read_lock);

	if (paylaod_remaining)
	{
		if (socket->epoll)
		{
			AddEventToEpollQ(etcp->ep,
						  USR_SHADOW_EVENT_QUEUE, socket, etcp_EPOLLIN);
		}
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
static inline int
CopyDataFromUserBuffer(flow_manager_t etcp, tcp_flow *cur_flow, const char *buf, int len)
{
	struct send_var_tcp *sndvar = cur_flow->sndvar;
	int sndlen;
	int ret;

	sndlen = MIN((int)sndvar->snd_wnd, len);
	if (sndlen <= 0)
	{
		errno = EAGAIN;
		return -1;
	}

	/* allocate send buffer if not exist */
	if (!sndvar->sndbuf)
	{
		sndvar->sndbuf = InitSendBuffer(sndvar->iss + 1);
		if (!sndvar->sndbuf)
		{
			cur_flow->close_reason = TCP_NO_MEM;
			/* notification may not required due to -1 return */
			errno = ENOMEM;
			return -1;
		}
	}

	ret = InsertToSendBuffer(sndvar->sndbuf, buf, sndlen);
	assert(ret == sndlen);
	sndvar->snd_wnd = sndvar->sndbuf->size - sndvar->sndbuf->len;
	if (ret <= 0)
	{
		fprintf(stderr,"InsertToSendBuffer failed. reason: %d (sndlen: %u, len: %u\n",
					ret, sndlen, sndvar->sndbuf->len);
		errno = EAGAIN;
		return -1;
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
ssize_t
etcp_write(etcp_engine_t etcp_engine, int sockid, const char *buf, size_t len)
{

	flow_manager_t etcp;
	socket_map_t socket;
	tcp_flow *cur_flow;
	struct send_var_tcp *sndvar;
	int ret;

	etcp = GetFlowManager(etcp_engine);
	if (!etcp)
	{
		return -1;
	}

	if (sockid < 0 || sockid >= CONFIG.max_concurrency)
	{
		 fprintf(stderr,"Socket id %d out of range.\n", sockid);
		errno = EBADF;
		return -1;
	}

	socket = &etcp->smap[sockid];
	if (socket->socktype == etcp_SOCK_UNUSED)
	{
		 fprintf(stderr,"Invalid socket id: %d\n", sockid);
		errno = EBADF;
		return -1;
	}

	if (socket->socktype != etcp_SOCK_FLOW)
	{
		 fprintf(stderr,"Not an end socket. id: %d\n", sockid);
		errno = ENOTSOCK;
		return -1;
	}

	cur_flow = socket->flow;
	if (!cur_flow ||
		!(cur_flow->state == TCP_ST_ESTABLISHED ||
		  cur_flow->state == TCP_ST_CLOSE_WAIT))
	{
		errno = ENOTCONN;
		return -1;
	}

	if (len <= 0)
	{
		if (socket->opts & etcp_NONBLOCK)
		{
			errno = EAGAIN;
			return -1;
		}
		else
		{
			return 0;
		}
	}

	sndvar = cur_flow->sndvar;
	pthread_mutex_lock(&sndvar->write_lock);

	ret = CopyDataFromUserBuffer(etcp, cur_flow, buf, len);

	pthread_mutex_unlock(&sndvar->write_lock);

	if (ret > 0 && !(sndvar->on_sendq || sndvar->on_data_pkt_list))
	{
		sndvar->on_sendq = TRUE;
		EnqueueToFlowQ(etcp->sendq, cur_flow); /* this always success */
		etcp->wakeup_flag = TRUE;
	}

	if (ret == 0 && (socket->opts & etcp_NONBLOCK))
	{
		ret = -1;
		errno = EAGAIN;
	}

	/* if there are remaining sending buffer, generate write event */
	if (sndvar->snd_wnd > 0)
	{
		if ((socket->epoll & etcp_EPOLLOUT) && !(socket->epoll & etcp_EPOLLET))
		{
			AddEventToEpollQ(etcp->ep,
						  USR_SHADOW_EVENT_QUEUE, socket, etcp_EPOLLOUT);
		}
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
