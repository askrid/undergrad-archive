#include "etcp.h"
#include "socket.h"
#include <errno.h>
/*---------------------------------------------------------------------------*/
socket_map_t 
MakeSocket(etcp_engine_t etcp_engine, int socktype, int need_lock)
{
	flow_manager_t etcp = g_etcp[etcp_engine->cpu];
	socket_map_t socket = NULL;

	if (need_lock)
		pthread_mutex_lock(&etcp->ctx->smap_lock);

	while (socket == NULL) {
		socket = TAILQ_FIRST(&etcp->free_smap);
		if (!socket) {
			if (need_lock)
				pthread_mutex_unlock(&etcp->ctx->smap_lock);

			fprintf(stderr,"The concurrent sockets are at maximum.\n");
			return NULL;
		}

		TAILQ_REMOVE(&etcp->free_smap, socket, free_smap_link);

		/* if there is not invalidated events, insert the socket to the end */
		/* and find another socket in the free smap list */
		if (socket->events) {
			perror("There are still not invalidate events remaining.\n");
			TAILQ_INSERT_TAIL(&etcp->free_smap, socket, free_smap_link);
			socket = NULL;
		}
	}

	if (need_lock)
		pthread_mutex_unlock(&etcp->ctx->smap_lock);
	
	socket->socktype = socktype;
	socket->opts = 0;
	socket->flow = NULL;
	socket->epoll = 0;
	socket->events = 0;

	/* 
	 * reset a few fields (needed for client socket) 
	 * addr = INADDR_ANY, port = INPORT_ANY
	 */
	memset(&socket->saddr, 0, sizeof(struct sockaddr_in));
	memset(&socket->ep_data, 0, sizeof(etcp_epoll_data_t));

	return socket;
}
/*---------------------------------------------------------------------------*/
void 
DestroySocket(etcp_engine_t etcp_engine, int sockid, int need_lock)
{
	flow_manager_t etcp = g_etcp[etcp_engine->cpu];
	socket_map_t socket = &etcp->smap[sockid];

	if (socket->socktype == etcp_SOCK_UNUSED) {
		return;
	}
	
	socket->socktype = etcp_SOCK_UNUSED;
	socket->epoll = etcp_EPOLLNONE;
	socket->events = 0;

	if (need_lock)
		pthread_mutex_lock(&etcp->ctx->smap_lock);

	/* insert into free flow map */
	etcp->smap[sockid].flow = NULL;
	TAILQ_INSERT_TAIL(&etcp->free_smap, socket, free_smap_link);

	if (need_lock)
		pthread_mutex_unlock(&etcp->ctx->smap_lock);
}
/*---------------------------------------------------------------------------*/
socket_map_t 
ReturnSocket(etcp_engine_t etcp_engine, int sockid)
{
	if (sockid < 0 || sockid >= CONFIG.max_concurrency) {
		errno = EBADF;
		return NULL;
	}

	return &g_etcp[etcp_engine->cpu]->smap[sockid];
}
