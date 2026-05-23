#include <assert.h>
#include "tcp_flow.h"
#include "etcp_hash.h"
#include "tcp_in.h"
#include "tcp_out.h"
#include "receive_buffer.h"
#include "send_buffer.h"
#include "eventpoll.h"
#include "ip_out.h"
#include "timer.h"
#define TCP_MAX_SEQ 4294967295

/*---------------------------------------------------------------------------*/
char *state_str[] = {"TCP_ST_CLOSED",
					 "TCP_ST_LISTEN",
					 "TCP_ST_SYN_SENT",
					 "TCP_ST_SYN_RCVD",
					 "TCP_ST_ESTABLISHED",
					 "TCP_ST_FIN_WAIT_1",
					 "TCP_ST_FIN_WAIT_2",
					 "TCP_ST_CLOSE_WAIT",
					 "TCP_ST_CLOSING",
					 "TCP_ST_LAST_ACK",
					 "TCP_ST_TIME_WAIT"};
/*---------------------------------------------------------------------------*/
char *close_reason_str[] = {
	"NOT_CLOSED",
	"CLOSE",
	"CLOSED",
	"CONN_FAIL",
	"CONN_LOST",
	"RESET",
	"NO_MEM",
	"DENIED",
	"TIMEDOUT"};
/*---------------------------------------------------------------------------*/
/* for rand_r() functions */
static __thread unsigned int next_seed;
/*---------------------------------------------------------------------------*/
inline char *
GetStateAsString(const tcp_flow *flow)
{
	return state_str[flow->state];
}
/*---------------------------------------------------------------------------*/
inline void
SetInitialSendingSequence()
{
	next_seed = time(NULL);
}
/*---------------------------------------------------------------------------*/
unsigned int
CalcHash(const void *f)
{
	tcp_flow *flow = (tcp_flow *)f;
	unsigned int hash, i;
	char *key = (char *)&flow->saddr; // key for hash table is source address

	for (hash = i = 0; i < 12; ++i)
	{
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash & (NUM_BINS_FLOWS - 1);
}
/*---------------------------------------------------------------------------*/
int IsFlowEqual(const void *f1, const void *f2)
{
	tcp_flow *flow1 = (tcp_flow *)f1;
	tcp_flow *flow2 = (tcp_flow *)f2;

	return (flow1->saddr == flow2->saddr &&
			flow1->sport == flow2->sport &&
			flow1->daddr == flow2->daddr &&
			flow1->dport == flow2->dport);
}
/*---------------------------------------------------------------------------*/
inline void
RegisterReadEvent(flow_manager_t etcp, tcp_flow *flow)
{
	if (flow->socket)
	{
		if (flow->socket->epoll & etcp_EPOLLIN)
		{
			AddEventToEpollQ(etcp->ep,
						  etcp_EVENT_QUEUE, flow->socket, etcp_EPOLLIN);
		}
	}
}
/*---------------------------------------------------------------------------*/
inline void
RegisterWriteEvent(flow_manager_t etcp, tcp_flow *flow)
{
	if (flow->socket)
	{
		if (flow->socket->epoll & etcp_EPOLLOUT)
		{
			AddEventToEpollQ(etcp->ep,
						  etcp_EVENT_QUEUE, flow->socket, etcp_EPOLLOUT);
		}
	}
}
/*---------------------------------------------------------------------------*/
inline void
RegisterCloseEvent(flow_manager_t etcp, tcp_flow *flow)
{
	if (flow->socket)
	{
		if (flow->socket->epoll & etcp_EPOLLRDHUP)
		{
			AddEventToEpollQ(etcp->ep,
						  etcp_EVENT_QUEUE, flow->socket, etcp_EPOLLRDHUP);
		}
		else if (flow->socket->epoll & etcp_EPOLLIN)
		{
			AddEventToEpollQ(etcp->ep,
						  etcp_EVENT_QUEUE, flow->socket, etcp_EPOLLIN);
		}
	}
}
/*---------------------------------------------------------------------------*/
inline void
RegisterErrorEvent(flow_manager_t etcp, tcp_flow *flow)
{
	if (flow->socket)
	{
		if (flow->socket->epoll & etcp_EPOLLERR)
		{
			AddEventToEpollQ(etcp->ep,
						  etcp_EVENT_QUEUE, flow->socket, etcp_EPOLLERR);
		}
	}
}
/*---------------------------------------------------------------------------*/
tcp_flow *
CreateFlow(flow_manager_t etcp, socket_map_t socket, int is_passive_open,
				uint32_t saddr, uint16_t sport, uint32_t daddr, uint16_t dport)
{
	tcp_flow *flow = NULL;
	int ret;
	flow = (tcp_flow *)malloc(sizeof(tcp_flow));
	if (!flow)
	{
		return NULL;
	}
	memset(flow, 0, sizeof(tcp_flow));

	flow->rcvvar = (struct recv_var_tcp *)malloc(sizeof(struct recv_var_tcp));
	if (!flow->rcvvar)
	{
		fprintf(stderr,"Cannnot allocate memory for the socket recieve buffer\n");
		return NULL;
	}
	memset(flow->rcvvar, 0, sizeof(struct recv_var_tcp));

	flow->sndvar = (struct send_var_tcp *)malloc(sizeof(struct send_var_tcp));
	if (!flow->sndvar)
	{
		fprintf(stderr,"Cannnot allocate memory for the socket send buffer\n");
		return NULL;
	}
	memset(flow->sndvar, 0, sizeof(struct send_var_tcp));

	flow->saddr = saddr;
	flow->sport = sport;
	flow->daddr = daddr;
	flow->dport = dport;

	ret = InsertFlowHT(etcp->tcp_flow_table, flow);
	if (ret < 0)
	{
		fprintf(stderr,"flow %d: "
					"Failed to insert the flow into hash table.\n",
					flow->id);
		return NULL;
	}

	flow->on_hash_table = TRUE;
	etcp->flow_cnt++;

	if (socket)
	{
		flow->socket = socket;
		socket->flow = flow;
	}

	flow->sndvar->iss = rand_r(&next_seed) % TCP_MAX_SEQ;
	flow->on_rto_idx = -1;
	flow->sndvar->mss = TCP_DEFAULT_MSS;
	flow->sndvar->rto = TCP_INITIAL_RTO;
	flow->sndvar->in_fast_recovery = 0;
	flow->rcvvar->snd_wl1 = flow->rcvvar->irs - 1;

	if (is_passive_open)
		flow->state = TCP_ST_LISTEN;
	else
		flow->state = TCP_ST_SYN_SENT;
		
	flow->rcvvar->irs = 0;
	flow->snd_nxt = flow->sndvar->iss;
	flow->sndvar->snd_una = flow->sndvar->iss;
	flow->sndvar->snd_wnd = CONFIG.sndbuf_size;
	flow->rcv_nxt = 0;
	flow->rcvvar->rcv_wnd = TCP_INITIAL_WINDOW;

	if (pthread_mutex_init(&flow->rcvvar->read_lock, NULL))
	{
		perror("pthread_mutex_init of read_lock");
		return NULL;
	}

	if (pthread_mutex_init(&flow->sndvar->write_lock, NULL))
	{
		perror("pthread_mutex_init of write_lock");
		pthread_mutex_destroy(&flow->rcvvar->read_lock);
		return NULL;
	}
	return flow;
}
/*---------------------------------------------------------------------------*/
void DestroyFlow(flow_manager_t etcp, tcp_flow *flow)
{
	struct sockaddr_in addr;
	int bound_addr = FALSE;
	uint8_t *sa, *da;
	int ret;
	sa = (uint8_t *)&flow->saddr;
	da = (uint8_t *)&flow->daddr;
	if (flow->is_bound_addr)
	{
		bound_addr = TRUE;
		addr.sin_addr.s_addr = flow->saddr;
		addr.sin_port = flow->sport;
	}

	RemoveFromControlPktList(etcp, flow);
	RemoveFromDataPktList(etcp, flow);
	RemoveFromAckPktList(etcp, flow);

	if (flow->on_rto_idx >= 0)
		RemoveFromRTOList(etcp, flow);

	if (flow->on_timewait_list)
		RemoveFromTimewaitList(etcp, flow);

	if (CONFIG.tcp_timeout > 0)
		RemoveFromTimeoutList(etcp, flow);

	pthread_mutex_destroy(&flow->rcvvar->read_lock);
	pthread_mutex_destroy(&flow->sndvar->write_lock);

	assert(flow->on_hash_table == TRUE);

	/* free ring buffers */
	if (flow->sndvar->sndbuf)
	{
		FreeSendBuffer(flow->sndvar->sndbuf);
		flow->sndvar->sndbuf = NULL;
	}
	if (flow->rcvvar->rcvbuf)
	{
		FreeReceiveBuffer(flow->rcvvar->rcvbuf);
		flow->rcvvar->rcvbuf = NULL;
	}

	/* remove from flow hash table */
	RemoveFlowHT(etcp->tcp_flow_table, flow);
	flow->on_hash_table = FALSE;

	etcp->flow_cnt--;

	free(flow->rcvvar);
	free(flow->sndvar);
	free(flow);

	if (bound_addr)
	{
		if (etcp->ap)
		{
			ret = FreeAddr(etcp->ap, &addr);
		}
		else
		{
			uint8_t is_external;
			int nif = FetchNetworkInterface(addr.sin_addr.s_addr, &is_external);
			if (nif < 0)
			{
				fprintf(stderr,"nif is negative!\n");
				ret = -1;
			}
			else
			{
				int eidx = CONFIG.nif_to_eidx[nif];
				ret = FreeAddr(ap[eidx], &addr);
			}
			UNUSED(is_external);
		}
		if (ret < 0)
		{
			fprintf(stderr,"(NEVER HAPPEN) Failed to free address.\n");
		}
	}

	UNUSED(da);
	UNUSED(sa);
}
