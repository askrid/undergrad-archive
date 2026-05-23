#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <sys/time.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <assert.h>
#include <sched.h>
#include "ip_util.h"
#include "eth_in.h"
#include "etcp_hash.h"
#include "send_buffer.h"
#include "receive_buffer.h"
#include "socket.h"
#include "eth_out.h"
#include "tcp_in.h"
#include "tcp_out.h"
#include "etcp_api.h"
#include "eventpoll.h"
#include "config.h"
#include "arp.h"
#include "ip_out.h"
#include "timer.h"
#include "cc_trace.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*----------------------------------------------------------------------------*/
extern int cc_number;
/* handlers for threads */
struct etcp_thread_context *g_pctx[MAX_CPUS] = {0};
/*----------------------------------------------------------------------------*/
static pthread_t g_thread[MAX_CPUS] = {0};
/*----------------------------------------------------------------------------*/
static sem_t g_init_sem[MAX_CPUS];
static int running[MAX_CPUS] = {0};
/*----------------------------------------------------------------------------*/
etcp_sighandler_t app_signal_handler;
static int sigint_cnt[MAX_CPUS] = {0};
static struct timespec sigint_ts[MAX_CPUS];
/*----------------------------------------------------------------------------*/
static int etcp_master = -1;
void etcp_free_context(etcp_engine_t etcp_engine);
/*----------------------------------------------------------------------------*/

static void SignalHandler(int signal)
{
	int i = 0;

	if (signal == SIGINT)
	{
		int core;
		struct timespec cur_ts;

		core = sched_getcpu();
		clock_gettime(CLOCK_REALTIME, &cur_ts);

		if (CONFIG.multi_process)
		{
			for (i = 0; i < num_cpus; i++)
				if (running[i] == TRUE)
					g_pctx[i]->exit = TRUE;
		}
		else
		{
			if (sigint_cnt[core] > 0 && cur_ts.tv_sec > sigint_ts[core].tv_sec)
			{
				for (i = 0; i < num_cpus; i++)
				{
					if (running[i])
					{
						g_pctx[i]->exit = TRUE;
					}
				}
			}
			else
			{
				for (i = 0; i < num_cpus; i++)
				{
					if (running[i])
						g_pctx[i]->interrupt = TRUE;
				}
				if (!app_signal_handler)
				{
					for (i = 0; i < num_cpus; i++)
					{
						if (running[i])
						{
							g_pctx[i]->exit = TRUE;
						}
					}
				}
			}
			sigint_cnt[core]++;
			clock_gettime(CLOCK_REALTIME, &sigint_ts[core]);
		}
	}

	if (signal != SIGUSR1)
	{
		if (app_signal_handler)
		{
			app_signal_handler(signal);
		}
	}
}
/*----------------------------------------------------------------------------*/
static inline void
FlushEventQueue(flow_manager_t etcp, uint32_t cur_ts)
{
	struct etcp_epoll *ep = etcp->ep;
	struct event_queue *usrq = ep->usr_queue;
	struct event_queue *etcpq = ep->etcp_queue;

	pthread_mutex_lock(&ep->epoll_lock);

	while (etcpq->num_events > 0 && usrq->num_events < usrq->size)
	{
		/* copy the event from etcp_queue to usr_queue */
		usrq->events[usrq->end++] = etcpq->events[etcpq->start++];

		if (usrq->end >= usrq->size)
			usrq->end = 0;
		usrq->num_events++;

		if (etcpq->start >= etcpq->size)
			etcpq->start = 0;
		etcpq->num_events--;
	}

	/* if there are pending events, wake up user */
	if (usrq->num_events > 0 || ep->usr_shadow_queue->num_events > 0)
	{
		STAT_COUNT(etcp->runstat.rounds_epoll);
		etcp->ts_last_event = cur_ts;
		pthread_cond_signal(&ep->epoll_cond);
	}
	pthread_mutex_unlock(&ep->epoll_lock);
}
/*----------------------------------------------------------------------------*/
static inline void
ProcessAppRequests(flow_manager_t etcp, uint32_t cur_ts)
{
	tcp_flow *flow;
	int cnt, max_cnt;
	int handled, delayed;
	int control, send, ack;

	/* connect handling */
	while ((flow = DequeueFromFlowQ(etcp->connectq)))
	{
		RegisterflowToControlPktList(etcp, flow, cur_ts);
	}

	/* send queue handling */
	while ((flow = DequeueFromFlowQ(etcp->sendq)))
	{
		flow->sndvar->on_sendq = FALSE;
		RegisterflowToSendPktList(etcp, flow);
	}

	/* ack queue handling */
	while ((flow = DequeueFromFlowQ(etcp->ackq)))
	{
		flow->sndvar->on_ackq = FALSE;
		RegisterACK(etcp, flow, cur_ts, ACK_OPT_AGGREGATE);
	}

	/* close handling */
	handled = delayed = 0;
	control = send = ack = 0;
	while ((flow = DequeueFromFlowQ(etcp->closeq)))
	{
		struct send_var_tcp *sndvar = flow->sndvar;
		sndvar->on_closeq = FALSE;

		if (sndvar->sndbuf)
		{
			sndvar->fss = sndvar->sndbuf->head_seq + sndvar->sndbuf->len;
		}
		else
		{
			sndvar->fss = flow->snd_nxt;
		}

		if (CONFIG.tcp_timeout > 0)
			RemoveFromTimeoutList(etcp, flow);

		if (flow->have_reset) // if Received RST Packet
		{
			handled++;
			if (flow->state != TCP_ST_CLOSED)
			{
				flow->close_reason = TCP_RESET;
				flow->state = TCP_ST_CLOSED;
				DestroyFlow(etcp, flow);
			}
		}
		else if (sndvar->on_control_pkt_list)
		{
			sndvar->on_closeq_int = TRUE;
			EnqueueToInternalFlowQ(etcp->closeq_int, flow);
			delayed++;
			if (sndvar->on_control_pkt_list)
				control++;
			if (sndvar->on_data_pkt_list)
				send++;
			if (sndvar->on_ack_pkt_list)
				ack++;
		}
		else if (sndvar->on_data_pkt_list || sndvar->on_ack_pkt_list)
		{
			handled++;
			if (flow->state == TCP_ST_ESTABLISHED)
			{
				flow->state = TCP_ST_FIN_WAIT_1;
			}
			else if (flow->state == TCP_ST_CLOSE_WAIT)
			{
				flow->state = TCP_ST_LAST_ACK;
			}
			flow->control_pkt_list_waiting = TRUE;
		}

		else if (flow->state != TCP_ST_CLOSED)
		{
			handled++;
			if (flow->state == TCP_ST_ESTABLISHED)
			{
				flow->state = TCP_ST_FIN_WAIT_1;
			}
			else if (flow->state == TCP_ST_CLOSE_WAIT)
			{
				flow->state = TCP_ST_LAST_ACK;
			}
			RegisterflowToControlPktList(etcp, flow, cur_ts);
		}
	}

	cnt = 0;
	max_cnt = etcp->closeq_int->count;
	while (cnt++ < max_cnt)
	{
		flow = DequeueFromInternalFlowQ(etcp->closeq_int);

		if (flow->sndvar->on_control_pkt_list)
		{
			EnqueueToInternalFlowQ(etcp->closeq_int, flow);
		}
		else if (flow->state != TCP_ST_CLOSED)
		{
			handled++;
			flow->sndvar->on_closeq_int = FALSE;
			if (flow->state == TCP_ST_ESTABLISHED)
			{
				flow->state = TCP_ST_FIN_WAIT_1;
			}
			else if (flow->state == TCP_ST_CLOSE_WAIT)
			{
				flow->state = TCP_ST_LAST_ACK;
			}
			RegisterflowToControlPktList(etcp, flow, cur_ts);
		}
		else
		{
			flow->sndvar->on_closeq_int = FALSE;
			fprintf(stderr, "Already closed connection!\n");
		}
	}

	/* reset handling */
	while ((flow = DequeueFromFlowQ(etcp->resetq)))
	{
		flow->sndvar->on_resetq = FALSE;

		if (CONFIG.tcp_timeout > 0)
			RemoveFromTimeoutList(etcp, flow);

		if (flow->have_reset)
		{
			if (flow->state != TCP_ST_CLOSED)
			{
				flow->close_reason = TCP_RESET;
				flow->state = TCP_ST_CLOSED;
				DestroyFlow(etcp, flow);
			}
			else
			{
				fprintf(stderr, "flow already closed.\n");
			}
		}
		else if (flow->sndvar->on_control_pkt_list ||
				 flow->sndvar->on_data_pkt_list || flow->sndvar->on_ack_pkt_list)
		{
			/* wait until all the queues are flushed */
			flow->sndvar->on_resetq_int = TRUE;
			EnqueueToInternalFlowQ(etcp->resetq_int, flow);
		}
		else
		{
			if (flow->state != TCP_ST_CLOSED)
			{
				flow->close_reason = TCP_ACTIVE_CLOSE;
				flow->state = TCP_ST_CLOSED;
				RegisterflowToControlPktList(etcp, flow, cur_ts);
			}
			else
			{
				fprintf(stderr, "flow already closed.\n");
			}
		}
	}

	cnt = 0;
	max_cnt = etcp->resetq_int->count;
	while (cnt++ < max_cnt)
	{
		flow = DequeueFromInternalFlowQ(etcp->resetq_int);

		if (flow->sndvar->on_control_pkt_list ||
			flow->sndvar->on_data_pkt_list || flow->sndvar->on_ack_pkt_list)
		{
			/* wait until all the queues are flushed */
			EnqueueToInternalFlowQ(etcp->resetq_int, flow);
		}
		else
		{
			flow->sndvar->on_resetq_int = FALSE;

			if (flow->state != TCP_ST_CLOSED)
			{
				flow->close_reason = TCP_ACTIVE_CLOSE;
				flow->state = TCP_ST_CLOSED;
				RegisterflowToControlPktList(etcp, flow, cur_ts);
			}
		}
	}

	/* destroy flows in destroyq */
	while ((flow = DequeueFromFlowQ(etcp->destroyq)))
	{
		DestroyFlow(etcp, flow);
	}

	etcp->wakeup_flag = FALSE;
}
/*----------------------------------------------------------------------------*/
static inline void
DispatchSendQueues(flow_manager_t etcp, uint32_t cur_ts)
{
	int thresh = CONFIG.max_concurrency;

	if (etcp->n_sender[0]->control_pkt_list_cnt)
	{
		FlushControlPktList(etcp, etcp->n_sender[0], cur_ts, thresh);
	}
	if (etcp->n_sender[0]->ack_pkt_list_cnt)
	{
		FlushAckPktList(etcp, etcp->n_sender[0], cur_ts, thresh);
	}
	if (etcp->n_sender[0]->data_pkt_list_cnt)
	{
		FlushDataPktList(etcp, etcp->n_sender[0], cur_ts, thresh);
	}
}
/*----------------------------------------------------------------------------*/
static void
WakeBlockedCalls(flow_manager_t etcp)
{
	int i;
	struct tcp_listener *listener = NULL;

	/* interrupt if the etcp_epoll_wait() is waiting */
	if (etcp->ep)
	{
		pthread_mutex_lock(&etcp->ep->epoll_lock);
		if (etcp->ep->waiting)
		{
			pthread_cond_signal(&etcp->ep->epoll_cond);
		}
		pthread_mutex_unlock(&etcp->ep->epoll_lock);
	}

	/* interrupt if the accept() is waiting */
	/* this may be a looong loop but this is called only on exit */
	for (i = 0; i < MAX_PORT; i++)
	{
		listener = SearchListnerHT(etcp->listeners, &i);
		if (listener != NULL)
		{
			pthread_mutex_lock(&listener->accept_lock);
			if (!(listener->socket->opts & etcp_NONBLOCK))
			{
				pthread_cond_signal(&listener->accept_cond);
			}
			pthread_mutex_unlock(&listener->accept_lock);
		}
	}
}
/*----------------------------------------------------------------------------*/
static void
CoreLoop(struct etcp_thread_context *ctx)
{
	flow_manager_t etcp = ctx->flow_manager;
	int i;
	int recv_cnt;
	struct timeval cur_ts = {0};
	uint32_t ts, ts_prev;
	int thresh;
	gettimeofday(&cur_ts, NULL);
	fprintf(stderr, "CPU %d: etcp thread running.\n", ctx->cpu);

	ts = ts_prev = 0;
	while ((!ctx->done || etcp->flow_cnt) && !ctx->exit)
	{
		recv_cnt = 0;

		gettimeofday(&cur_ts, NULL);
		ts = TIMEVAL_TO_TS(&cur_ts);
		etcp->cur_ts = ts;

		// receive batch of incoming packets
		static uint16_t len;
		static uint8_t *pktbuf;
		recv_cnt = etcp->io_func->recv_pkts(ctx);

		for (i = 0; i < recv_cnt; i++)
		{
			pktbuf = etcp->io_func->get_rptr(etcp->ctx, i, &len);
			if (pktbuf != NULL)
			{
				ProcessEthPacket(etcp, 0, ts, pktbuf, len);
			}
		}

		/* interaction with application */
		if (etcp->flow_cnt > 0)
		{
			/* check retransmission timeout and timewait expire */
			thresh = CONFIG.max_concurrency;
			if (thresh == -1)
				thresh = CONFIG.max_concurrency;

			CheckRetransmissionTimeout(etcp, ts, thresh);
			CheckTimewaitExpire(etcp, ts, CONFIG.max_concurrency);

			if (CONFIG.tcp_timeout > 0 && ts != ts_prev)
			{
				CheckConnectionTimeout(etcp, ts, thresh);
			}
		}

		etcp->io_func->release_pkt(ctx);

		/* if epoll is in use, flush all the queued events */
		if (etcp->ep)
		{
			FlushEventQueue(etcp, ts);
		}
		if (etcp->flow_cnt > 0)
		{
			ProcessAppRequests(etcp, ts);
		}

		DispatchSendQueues(etcp, ts);

		/* send packets from write buffer */
		/* send until tx is available */
		etcp->io_func->send_pkts(ctx);

		if (ts != ts_prev)
		{
			ts_prev = ts;
			if (ctx->cpu == etcp_master)
			{
				ARPTimer(etcp, ts);
			}
		}

		if (ctx->interrupt)
		{
			WakeBlockedCalls(etcp);
		}
	}
	WakeBlockedCalls(etcp);
}
/*----------------------------------------------------------------------------*/
struct pkt_sender *
CreateSender(int ifidx)
{
	struct pkt_sender *sender;

	sender = (struct pkt_sender *)calloc(1, sizeof(struct pkt_sender));
	if (!sender)
	{
		return NULL;
	}

	sender->ifidx = ifidx;

	TAILQ_INIT(&sender->control_pkt_list);
	TAILQ_INIT(&sender->data_pkt_list);
	TAILQ_INIT(&sender->ack_pkt_list);

	sender->control_pkt_list_cnt = 0;
	sender->data_pkt_list_cnt = 0;
	sender->ack_pkt_list_cnt = 0;

	return sender;
}
/*----------------------------------------------------------------------------*/
static flow_manager_t
InitFlowManger(struct etcp_thread_context *ctx)
{
	flow_manager_t etcp;
	int i;

	etcp = (flow_manager_t)calloc(1, sizeof(struct flow_manager));
	if (!etcp)
	{
		perror("malloc");
		fprintf(stderr, "Failed to allocate flow_manager.\n");
		return NULL;
	}
	g_etcp[ctx->cpu] = etcp;

	etcp->tcp_flow_table = CreateHT(CalcHash, IsFlowEqual, NUM_BINS_FLOWS);
	if (!etcp->tcp_flow_table)
	{
		fprintf(stderr, "Falied to allocate tcp flow table.\n");
		return NULL;
	}

	etcp->listeners = CreateHT(GetHashListener, GetEqualListner, NUM_BINS_LISTENERS);
	if (!etcp->listeners)
	{
		fprintf(stderr, "Failed to allocate listener table.\n");
		return NULL;
	}

	etcp->ctx = ctx;

	ConfigSendBufferSize(CONFIG.sndbuf_size);
	ConfigReceiveBufferSize(CONFIG.rcvbuf_size);
	SetInitialSendingSequence();

	etcp->smap = (socket_map_t)calloc(CONFIG.max_concurrency, sizeof(struct socket_map));
	if (!etcp->smap)
	{
		perror("calloc");
		fprintf(stderr, "Failed to allocate memory for flow map.\n");
		return NULL;
	}
	TAILQ_INIT(&etcp->free_smap);
	for (i = 0; i < CONFIG.max_concurrency; i++)
	{
		etcp->smap[i].id = i;
		etcp->smap[i].socktype = etcp_SOCK_UNUSED;
		memset(&etcp->smap[i].saddr, 0, sizeof(struct sockaddr_in));
		etcp->smap[i].flow = NULL;
		TAILQ_INSERT_TAIL(&etcp->free_smap, &etcp->smap[i], free_smap_link);
	}

	etcp->ep = NULL;
	etcp->connectq = CreateFlowQueue(BACKLOG_SIZE);
	if (!etcp->connectq)
	{
		fprintf(stderr, "Failed to create connect queue.\n");
		return NULL;
	}
	etcp->sendq = CreateFlowQueue(CONFIG.max_concurrency);
	if (!etcp->sendq)
	{
		fprintf(stderr, "Failed to create send queue.\n");
		return NULL;
	}
	etcp->ackq = CreateFlowQueue(CONFIG.max_concurrency);
	if (!etcp->ackq)
	{
		fprintf(stderr, "Failed to create ack queue.\n");
		return NULL;
	}
	etcp->closeq = CreateFlowQueue(CONFIG.max_concurrency);
	if (!etcp->closeq)
	{
		fprintf(stderr, "Failed to create close queue.\n");
		return NULL;
	}
	etcp->closeq_int = CreateInternalFlowQ(CONFIG.max_concurrency);
	if (!etcp->closeq_int)
	{
		fprintf(stderr, "Failed to create close queue.\n");
		return NULL;
	}
	etcp->resetq = CreateFlowQueue(CONFIG.max_concurrency);
	if (!etcp->resetq)
	{
		fprintf(stderr, "Failed to create reset queue.\n");
		return NULL;
	}
	etcp->resetq_int = CreateInternalFlowQ(CONFIG.max_concurrency);
	if (!etcp->resetq_int)
	{
		fprintf(stderr, "Failed to create reset queue.\n");
		return NULL;
	}
	etcp->destroyq = CreateFlowQueue(CONFIG.max_concurrency);
	if (!etcp->destroyq)
	{
		fprintf(stderr, "Failed to create destroy queue.\n");
		return NULL;
	}

	etcp->g_sender = CreateSender(-1);
	if (!etcp->g_sender)
	{
		fprintf(stderr, "Failed to create global sender structure.\n");
		return NULL;
	}
	for (i = 0; i < CONFIG.eths_num; i++)
	{
		etcp->n_sender[i] = CreateSender(i);
		if (!etcp->n_sender[i])
		{
			fprintf(stderr, "Failed to create per-nic sender structure.\n");
			return NULL;
		}
	}

	etcp->rto_store = InitRTOStructure();
	TAILQ_INIT(&etcp->timewait_list);
	TAILQ_INIT(&etcp->timeout_list);
	return etcp;
}
/*----------------------------------------------------------------------------*/
static void *
RunETCPThread(void *arg)
{
	etcp_engine_t etcp_engine = (etcp_engine_t)arg;
	int cpu = etcp_engine->cpu;
	struct flow_manager *etcp;
	struct etcp_thread_context *ctx;
	ctx = calloc(1, sizeof(*ctx));
	if (!ctx)
	{
		perror("calloc");
		fprintf(stderr, "Failed to calloc etcp context.\n");
		exit(-1);
	}
	ctx->thread = pthread_self();
	ctx->cpu = cpu;
	etcp = ctx->flow_manager = InitFlowManger(ctx);
	if (!etcp)
	{
		fprintf(stderr, "Failed to initialize etcp manager.\n");
		exit(-1);
	}

	/* assign etcp context's underlying I/O module */
	etcp->io_func = current_iomodule_func;

	/* I/O initializing */
	etcp->io_func->init_handle(ctx);

	if (pthread_mutex_init(&ctx->smap_lock, NULL))
	{
		perror("pthread_mutex_init of ctx->smap_lock\n");
		exit(-1);
	}

	if (pthread_mutex_init(&ctx->socket_pool_lock, NULL))
	{
		perror("pthread_mutex_init of ctx->socket_pool_lock\n");
		exit(-1);
	}

	g_pctx[cpu] = ctx;
	mlockall(MCL_CURRENT);
	fprintf(stderr, "CPU %d: initialization finished.\n", cpu);

	sem_post(&g_init_sem[ctx->cpu]);

	/* start the main loop */
	CoreLoop(ctx);

	struct etcp_engine m;
	m.cpu = cpu;
	etcp_free_context(&m);
	/* destroy hash tables */
	DestroyHT(g_etcp[cpu]->tcp_flow_table);
	DestroyHT(g_etcp[cpu]->listeners);
	fprintf(stderr, "etcp thread %d finished.\n", cpu);

	return 0;
}
/*----------------------------------------------------------------------------*/
etcp_engine_t
etcp_create_context(int cpu)
{
	etcp_engine_t etcp_engine;
	int ret;

	if (cpu >= CONFIG.num_cores)
	{
		fprintf(stderr, "Failed initialize new etcp context. "
						"Requested cpu id %d exceed the number of cores %d configured to use.\n",
				cpu, CONFIG.num_cores);
		return NULL;
	}

	ret = sem_init(&g_init_sem[cpu], 0, 0);
	// semaphore is shared across all threads of process
	if (ret)
	{
		fprintf(stderr, "Failed initialize init_sem.\n");
		return NULL;
	}

	etcp_engine = (etcp_engine_t)calloc(1, sizeof(struct etcp_engine));
	if (!etcp_engine)
	{
		fprintf(stderr, "Failed to allocate memory for etcp_engine.\n");
		return NULL;
	}
	etcp_engine->cpu = cpu;

	if (pthread_create(&g_thread[cpu],
					   NULL, RunETCPThread, (void *)etcp_engine) != 0)
	{
		fprintf(stderr, "pthread_create of etcp thread failed!\n");
		return NULL;
	}

	sem_wait(&g_init_sem[cpu]);
	sem_destroy(&g_init_sem[cpu]);

	running[cpu] = TRUE;

	if (etcp_master < 0)
	{
		etcp_master = cpu;
	}

	return etcp_engine;
}
/*----------------------------------------------------------------------------*/
void etcp_destroy_context(etcp_engine_t etcp_engine)
{
	struct etcp_thread_context *ctx = g_pctx[etcp_engine->cpu];
	if (ctx != NULL)
		ctx->done = 1;
	free(etcp_engine);
}
/*----------------------------------------------------------------------------*/
void etcp_free_context(etcp_engine_t etcp_engine)
{
	struct etcp_thread_context *ctx = g_pctx[etcp_engine->cpu];
	struct flow_manager *etcp = ctx->flow_manager;
	int i;

	if (g_pctx[etcp_engine->cpu] == NULL)
		return;

	/* close all flow sockets that are still open */
	if (!ctx->exit)
	{
		for (i = 0; i < CONFIG.max_concurrency; i++)
		{
			if (etcp->smap[i].socktype == etcp_SOCK_FLOW)
			{
				fprintf(stderr, "Closing remaining socket %d (%s)\n",
						i, GetStateAsString(etcp->smap[i].flow));
				etcp_close(etcp_engine, i);
			}
		}
	}

	ctx->done = 1;
	printf("etcp thread %d joined.\n", etcp_engine->cpu);
	running[etcp_engine->cpu] = FALSE;

	if (etcp_master == etcp_engine->cpu)
	{
		for (i = 0; i < num_cpus; i++)
		{
			if (i != etcp_engine->cpu && running[i])
			{
				etcp_master = i;
				break;
			}
		}
	}

	if (etcp->connectq)
	{
		DestroyFlowQueue(etcp->connectq);
		etcp->connectq = NULL;
	}
	if (etcp->sendq)
	{
		DestroyFlowQueue(etcp->sendq);
		etcp->sendq = NULL;
	}
	if (etcp->ackq)
	{
		DestroyFlowQueue(etcp->ackq);
		etcp->ackq = NULL;
	}
	if (etcp->closeq)
	{
		DestroyFlowQueue(etcp->closeq);
		etcp->closeq = NULL;
	}
	if (etcp->closeq_int)
	{
		DestroyInternalFlowQ(etcp->closeq_int);
		etcp->closeq_int = NULL;
	}
	if (etcp->resetq)
	{
		DestroyFlowQueue(etcp->resetq);
		etcp->resetq = NULL;
	}
	if (etcp->resetq_int)
	{
		DestroyInternalFlowQ(etcp->resetq_int);
		etcp->resetq_int = NULL;
	}
	if (etcp->destroyq)
	{
		DestroyFlowQueue(etcp->destroyq);
		etcp->destroyq = NULL;
	}

	free(etcp->g_sender);
	for (i = 0; i < CONFIG.eths_num; i++)
	{
		free(etcp->n_sender[i]);
	}

	if (etcp->ap)
	{
		RemoveAddrPool(etcp->ap);
		etcp->ap = NULL;
	}

	etcp->io_func->destroy_handle(ctx);
	free(ctx);
	g_pctx[etcp_engine->cpu] = NULL;
}
/*----------------------------------------------------------------------------*/
etcp_sighandler_t
etcp_register_signal(int signum, etcp_sighandler_t handler)
{
	etcp_sighandler_t prev;

	if (signum == SIGINT)
	{
		prev = app_signal_handler;
		app_signal_handler = handler;
	}
	else
	{
		if ((prev = signal(signum, handler)) == SIG_ERR)
		{
			perror("signal");
			return SIG_ERR;
		}
	}

	return prev;
}
/*----------------------------------------------------------------------------*/
int etcp_getconf(struct etcp_conf *conf)
{
	if (!conf)
		return -1;

	conf->num_cores = CONFIG.num_cores;
	conf->max_concurrency = CONFIG.max_concurrency;

	conf->max_num_buffers = CONFIG.max_num_buffers;
	conf->rcvbuf_size = CONFIG.rcvbuf_size;
	conf->sndbuf_size = CONFIG.sndbuf_size;

	conf->tcp_timewait = CONFIG.tcp_timewait;
	conf->tcp_timeout = CONFIG.tcp_timeout;

	return 0;
}
/*----------------------------------------------------------------------------*/
int etcp_setconf(const struct etcp_conf *conf)
{
	if (!conf)
		return -1;

	if (conf->num_cores > 0)
		CONFIG.num_cores = conf->num_cores;
	if (conf->max_concurrency > 0)
		CONFIG.max_concurrency = conf->max_concurrency;
	if (conf->max_num_buffers > 0)
		CONFIG.max_num_buffers = conf->max_num_buffers;
	if (conf->rcvbuf_size > 0)
		CONFIG.rcvbuf_size = conf->rcvbuf_size;
	if (conf->sndbuf_size > 0)
		CONFIG.sndbuf_size = conf->sndbuf_size;

	if (conf->tcp_timewait > 0)
		CONFIG.tcp_timewait = conf->tcp_timewait;
	if (conf->tcp_timeout > 0)
		CONFIG.tcp_timeout = conf->tcp_timeout;

	return 0;
}

/*----------------------------------------------------------------------------*/
int etcp_init(const char *config_file)
{
	int i;
	int ret;

	/* getting cpu and NIC */
	num_cpus = 1;
	assert(num_cpus >= 1);

	if (num_cpus > MAX_CPUS)
	{
		fprintf(stderr, "You cannot run etcp with more than %d cores due "
						"to your static etcp configuration. Please disable "
						"the last %d cores in your system.\n",
				MAX_CPUS, num_cpus - MAX_CPUS);
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < num_cpus; i++)
	{
		g_etcp[i] = NULL;
		running[i] = FALSE;
		sigint_cnt[i] = 0;
	}

	ret = LoadConfiguration(config_file);
	if (ret)
	{
		return -1;
	}
	PrintConfiguration();

	for (i = 0; i < CONFIG.eths_num; i++)
	{
		// in case of pcap, ip_address of veth_host is passed as argument
		ap[i] = AllocAddrPool(CONFIG.eths[i].ip_addr, 1);
		if (!ap[i])
		{
			return -1;
		}
	}

	PrintInterfaceInfo();

	ret = SetRoutingTable();
	if (ret)
	{
		return -1;
	}
	LoadARPTable();
	if (signal(SIGUSR1, SignalHandler) == SIG_ERR)
	{
		perror("signal, SIGUSR1");
		return -1;
	}
	if (signal(SIGINT, SignalHandler) == SIG_ERR)
	{
		perror("signal, SIGINT");
		return -1;
	}
	app_signal_handler = NULL;

	/* load system-wide io module specs */
	current_iomodule_func->load_module();

	return 0;
}
/*----------------------------------------------------------------------------*/
void etcp_destroy()
{
	int i;
	/* wait until all threads are closed */
	for (i = 0; i < num_cpus; i++)
	{
		if (running[i])
		{

			pthread_join(g_thread[i], NULL);
		}
	}

	for (i = 0; i < CONFIG.eths_num; i++)
		RemoveAddrPool(ap[i]);

	printf("All etcp threads are joined.\n");
}
/*----------------------------------------------------------------------------*/
