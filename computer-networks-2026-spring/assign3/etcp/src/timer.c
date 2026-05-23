#include <assert.h>
#include "timer.h"
#include "tcp_in.h"
#include "tcp_out.h"
#include "cc_trace.h"
#include "stat.h"
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
/*----------------------------------------------------------------------------*/
struct rto_hashstore *
InitRTOStructure()
{
	int i;
	struct rto_hashstore *hs = calloc(1, sizeof(struct rto_hashstore));
	if (!hs)
	{
		fprintf(stderr, "calloc: InitHashStore");
		return 0;
	}

	for (i = 0; i < RTO_HASH; i++)
		TAILQ_INIT(&hs->rto_list[i]);

	TAILQ_INIT(&hs->rto_list[RTO_HASH]);

	return hs;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
inline void
RegisterToRTOList(flow_manager_t etcp, tcp_flow *cur_flow)
{
	if (!etcp->rto_list_cnt)
	{
		etcp->rto_store->rto_now_idx = 0;
		etcp->rto_store->rto_now_ts = cur_flow->sndvar->ts_rto;
	}

	if (cur_flow->on_rto_idx < 0)
	{
		if (cur_flow->on_timewait_list)
		{
			fprintf(stderr, "flow %u: cannot be in both "
							"rto and timewait list.\n",
					cur_flow->id);
			return;
		}

		int diff = (int32_t)(cur_flow->sndvar->ts_rto - etcp->rto_store->rto_now_ts);
		if (diff < RTO_HASH)
		{
			int offset = (diff + etcp->rto_store->rto_now_idx) % RTO_HASH;
			cur_flow->on_rto_idx = offset;
			TAILQ_INSERT_TAIL(&(etcp->rto_store->rto_list[offset]),
							  cur_flow, sndvar->timer_link);
		}
		else
		{
			cur_flow->on_rto_idx = RTO_HASH;
			TAILQ_INSERT_TAIL(&(etcp->rto_store->rto_list[RTO_HASH]),
							  cur_flow, sndvar->timer_link);
		}
		etcp->rto_list_cnt++;
	}
}
/*----------------------------------------------------------------------------*/
inline void
RemoveFromRTOList(flow_manager_t etcp, tcp_flow *cur_flow)
{
	if (cur_flow->on_rto_idx < 0)
	{
		return;
	}

	TAILQ_REMOVE(&etcp->rto_store->rto_list[cur_flow->on_rto_idx],
				 cur_flow, sndvar->timer_link);
	cur_flow->on_rto_idx = -1;

	etcp->rto_list_cnt--;
}
/*----------------------------------------------------------------------------*/
inline void
RegisterToTimewaitList(flow_manager_t etcp, tcp_flow *cur_flow, uint32_t cur_ts)
{
	cur_flow->rcvvar->ts_tw_expire = cur_ts + CONFIG.tcp_timewait;

	if (cur_flow->on_timewait_list)
	{
		// Update list in sorted way by ts_tw_expire
		TAILQ_REMOVE(&etcp->timewait_list, cur_flow, sndvar->timer_link);
		TAILQ_INSERT_TAIL(&etcp->timewait_list, cur_flow, sndvar->timer_link);
	}
	else
	{
		if (cur_flow->on_rto_idx >= 0)
		{
			fprintf(stderr, "flow %u: cannot be in both "
							"timewait and rto list.\n",
					cur_flow->id);
			RemoveFromRTOList(etcp, cur_flow);
		}

		cur_flow->on_timewait_list = TRUE;
		TAILQ_INSERT_TAIL(&etcp->timewait_list, cur_flow, sndvar->timer_link);
		etcp->timewait_list_cnt++;
	}
}
/*----------------------------------------------------------------------------*/
inline void
RemoveFromTimewaitList(flow_manager_t etcp, tcp_flow *cur_flow)
{
	if (!cur_flow->on_timewait_list)
	{
		assert(0);
		return;
	}

	TAILQ_REMOVE(&etcp->timewait_list, cur_flow, sndvar->timer_link);
	cur_flow->on_timewait_list = FALSE;
	etcp->timewait_list_cnt--;
}
/*----------------------------------------------------------------------------*/
inline void
RegisterToTimeoutList(flow_manager_t etcp, tcp_flow *cur_flow)
{
	if (cur_flow->on_timeout_list)
	{
		assert(0);
		return;
	}

	cur_flow->on_timeout_list = TRUE;
	TAILQ_INSERT_TAIL(&etcp->timeout_list, cur_flow, sndvar->timeout_link);
	etcp->timeout_list_cnt++;
}
/*----------------------------------------------------------------------------*/
inline void
RemoveFromTimeoutList(flow_manager_t etcp, tcp_flow *cur_flow)
{
	if (cur_flow->on_timeout_list)
	{
		cur_flow->on_timeout_list = FALSE;
		TAILQ_REMOVE(&etcp->timeout_list, cur_flow, sndvar->timeout_link);
		etcp->timeout_list_cnt--;
	}
}
/*----------------------------------------------------------------------------*/
inline void
RenewTimeoutList(flow_manager_t etcp, tcp_flow *cur_flow)
{
	if (cur_flow->on_timeout_list)
	{
		TAILQ_REMOVE(&etcp->timeout_list, cur_flow, sndvar->timeout_link);
		TAILQ_INSERT_TAIL(&etcp->timeout_list, cur_flow, sndvar->timeout_link);
	}
}
/*----------------------------------------------------------------------------*/
inline void
RenewRetransmissionTimer(flow_manager_t etcp,
						 tcp_flow *cur_flow, uint32_t cur_ts)
{
	/* Update the retransmission timer */
	assert(cur_flow->sndvar->rto > 0);
	cur_flow->sndvar->nrtx = 0;

	/* if in rto list, remove it */
	if (cur_flow->on_rto_idx >= 0)
	{
		RemoveFromRTOList(etcp, cur_flow);
	}

	/* Reset retransmission timeout */
	if (TCP_SEQ_GT(cur_flow->snd_nxt, cur_flow->sndvar->snd_una))
	{
		/* there are packets sent but not acked */
		/* update rto timestamp */
		cur_flow->sndvar->ts_rto = cur_ts + cur_flow->sndvar->rto;
		RegisterToRTOList(etcp, cur_flow);
	}
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
int ProcessRetransmissionTimeOut(flow_manager_t etcp, uint32_t cur_ts, tcp_flow *cur_flow)
{
	assert(cur_flow->sndvar->rto > 0);
	uint32_t old_cwnd = cur_flow->sndvar->cwnd;
	uint32_t old_ssthresh = cur_flow->sndvar->ssthresh;

	/* if the flow is ready to be closed, don't handle RTO */
	if (cur_flow->close_reason != TCP_NOT_CLOSED)
		return 0;

	/* count number of retransmissions */
	if (cur_flow->sndvar->nrtx < TCP_MAX_RTX)
	{
		cur_flow->sndvar->nrtx++;
	}
	else
	{
		/* if it exceeds the threshold, destroy and notify to application */
		if (cur_flow->state < TCP_ST_ESTABLISHED)
		{
			cur_flow->state = TCP_ST_CLOSED;
			cur_flow->close_reason = TCP_CONN_FAIL;
			DestroyFlow(etcp, cur_flow);
		}
		else
		{
			cur_flow->state = TCP_ST_CLOSED;
			cur_flow->close_reason = TCP_CONN_LOST;
			if (cur_flow->socket)
			{
				RegisterErrorEvent(etcp, cur_flow);
			}
			else
			{
				DestroyFlow(etcp, cur_flow);
			}
		}
		return ERROR;
	}
	if (cur_flow->state >= TCP_ST_SYN_SENT && cur_flow->state < TCP_ST_ESTABLISHED)
	{
		/**************************************************************************** */
		// TODO section
		/* PART_1: update RTO after timeout */
		/*
		 * Sicnce there are not enough RTT samples, simply doubling the rto would work
		 */
		cur_flow->sndvar->rto <<= 1;
		if (cur_flow->sndvar->rto > 60000)
			cur_flow->sndvar->rto = 60000;
		/**************************************************************************** */
	}

	if (cur_flow->state >= TCP_ST_ESTABLISHED)
	{
		/**************************************************************************** */
		// TODO section
		/* PART_2: update RTO after timeout */
		/*
		 * When a retransmission timeout occurs, increase the RTO so that repeated
		 * retransmissions become more conservative.
		 *
		 * - In ESTABLISHED and later states, use the RTT estimator together with
		 *   exponential backoff.
		 *
		 * HINT:
		 * 		1. Main objective here is to calculate new retransmission time out value(sndvar->rto)
		 * 		2. backoff must not exceed TCP_MAX_BACKOFF(=7) here
		 * 		3. may use sndvar's nrtx value to determine backoff
		 */
		uint32_t backoff = MIN((uint32_t)cur_flow->sndvar->nrtx, (uint32_t)TCP_MAX_BACKOFF);
		uint32_t base = (cur_flow->rcvvar->srtt >> 3) + cur_flow->rcvvar->rttvar;
		if (base == 0)
			base = cur_flow->sndvar->rto;
		uint32_t new_rto = base << backoff;
		if (new_rto > 60000)
			new_rto = 60000;
		cur_flow->sndvar->rto = new_rto;
		/**************************************************************************** */
	}

	/**************************************************************************** */
	// TODO section
	/* Part2 Bonus: Update Congestion Control variables on retransmission time out */
	if (cur_flow->state >= TCP_ST_ESTABLISHED)
	{
		cur_flow->sndvar->ssthresh = MAX(cur_flow->sndvar->cwnd / 2,
										 2 * cur_flow->sndvar->mss);
		cur_flow->sndvar->cwnd = cur_flow->sndvar->mss;
		CC_LOG(cur_flow, cur_ts, "rto", cur_flow->sndvar->snd_una, NULL);
	}
	(void)old_cwnd;
	(void)old_ssthresh;
	/**************************************************************************** */

	cur_flow->sndvar->in_fast_recovery = 0;
	cur_flow->rcvvar->dup_acks = 0;
	cur_flow->rcvvar->last_ack_seq = cur_flow->sndvar->snd_una;


	/* SYN retry limit check */
	if (cur_flow->state == TCP_ST_SYN_SENT)
	{
		if (cur_flow->sndvar->nrtx > TCP_MAX_SYN_RETRY)
		{
			cur_flow->state = TCP_ST_CLOSED;
			cur_flow->close_reason = TCP_CONN_FAIL;
			if (cur_flow->socket)
			{
				RegisterErrorEvent(etcp, cur_flow);
			}
			else
			{
				DestroyFlow(etcp, cur_flow);
			}
			return ERROR;
		}
	}
	else if (cur_flow->state != TCP_ST_SYN_RCVD &&
			 cur_flow->state != TCP_ST_ESTABLISHED &&
			 cur_flow->state != TCP_ST_CLOSE_WAIT &&
			 cur_flow->state != TCP_ST_LAST_ACK &&
			 cur_flow->state != TCP_ST_FIN_WAIT_1 &&
			 cur_flow->state != TCP_ST_CLOSING)
	{
		return ERROR;
	}

	if (cur_flow->have_reset && cur_flow->state == TCP_ST_SYN_RCVD)
	{
		DestroyFlow(etcp, cur_flow);
		return 0;
	}

	/**************************************************************************** */
	// TODO section
	/* PART 1+2: General RTO Handling */
	/* Retransmit from unacked byte
	 * Update
	 * 	 snd_nxt
	 */
	cur_flow->snd_nxt = cur_flow->sndvar->snd_una;
	/**************************************************************************** */

	/* schedule retransmission on the appropriate send list */
	if (cur_flow->state == TCP_ST_ESTABLISHED ||
		cur_flow->state == TCP_ST_CLOSE_WAIT)
	{
		RegisterflowToSendPktList(etcp, cur_flow);
	}
	else if (cur_flow->state == TCP_ST_FIN_WAIT_1 ||
			 cur_flow->state == TCP_ST_CLOSING ||
			 cur_flow->state == TCP_ST_LAST_ACK)
	{
		/* decide to retransmit data or control packet */
		if (TCP_SEQ_LT(cur_flow->snd_nxt, cur_flow->sndvar->fss))
		{
			/* need to retransmit data */
			if (cur_flow->sndvar->on_control_pkt_list)
			{
				RemoveFromControlPktList(etcp, cur_flow);
			}
			cur_flow->control_pkt_list_waiting = TRUE;
			RegisterflowToSendPktList(etcp, cur_flow);
		}
		else
		{
			/* need to retransmit control packet */
			RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
		}
	}
	else
	{
		RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static inline void
RearrangeRTOStore(flow_manager_t etcp)
{
	tcp_flow *walk, *next;
	struct rto_head *rto_list = &etcp->rto_store->rto_list[RTO_HASH];
	int cnt = 0;

	for (walk = TAILQ_FIRST(rto_list);
		 walk != NULL; walk = next)
	{
		next = TAILQ_NEXT(walk, sndvar->timer_link);

		int diff = (int32_t)(walk->sndvar->ts_rto - etcp->rto_store->rto_now_ts);
		if (diff < RTO_HASH)
		{
			int offset = (diff + etcp->rto_store->rto_now_idx) % RTO_HASH;
			TAILQ_REMOVE(&etcp->rto_store->rto_list[RTO_HASH],
						 walk, sndvar->timer_link);
			walk->on_rto_idx = offset;
			TAILQ_INSERT_TAIL(&(etcp->rto_store->rto_list[offset]),
							  walk, sndvar->timer_link);
		}
		cnt++;
	}
}
/*----------------------------------------------------------------------------*/
void CheckRetransmissionTimeout(flow_manager_t etcp, uint32_t cur_ts, int thresh)
{
	tcp_flow *walk, *next;
	struct rto_head *rto_list;
	int cnt;

	if (!etcp->rto_list_cnt)
	{
		return;
	}

	STAT_COUNT(etcp->runstat.rounds_rtocheck);

	cnt = 0;

	while (1)
	{

		rto_list = &etcp->rto_store->rto_list[etcp->rto_store->rto_now_idx];
		// check if retransmission timeout had occurred
		if ((int32_t)(cur_ts - etcp->rto_store->rto_now_ts) < 0)
		{
			break;
		}

		for (walk = TAILQ_FIRST(rto_list);
			 walk != NULL; walk = next)
		{
			if (++cnt > thresh)
			{
				break;
			}
			next = TAILQ_NEXT(walk, sndvar->timer_link);

			if (walk->on_rto_idx >= 0)
			{
				TAILQ_REMOVE(rto_list, walk, sndvar->timer_link);
				etcp->rto_list_cnt--;
				walk->on_rto_idx = -1;
				ProcessRetransmissionTimeOut(etcp, cur_ts, walk);
			}
			else
			{
				fprintf(stderr, "flow %d: not on rto list.\n", walk->id);
			}
		}

		if (cnt > thresh)
		{
			break;
		}
		else
		{
			etcp->rto_store->rto_now_idx = (etcp->rto_store->rto_now_idx + 1) % RTO_HASH;
			etcp->rto_store->rto_now_ts++;
			if (!(etcp->rto_store->rto_now_idx % 1000))
			{
				RearrangeRTOStore(etcp);
			}
		}
	}
}
/*----------------------------------------------------------------------------*/
void CheckTimewaitExpire(flow_manager_t etcp, uint32_t cur_ts, int thresh)
{
	tcp_flow *walk, *next;
	int cnt;

	STAT_COUNT(etcp->runstat.rounds_twcheck);

	cnt = 0;

	for (walk = TAILQ_FIRST(&etcp->timewait_list);
		 walk != NULL; walk = next)
	{
		if (++cnt > thresh)
			break;
		next = TAILQ_NEXT(walk, sndvar->timer_link);

		if (walk->on_timewait_list)
		{
			if ((int32_t)(cur_ts - walk->rcvvar->ts_tw_expire) >= 0)
			{
				if (!walk->sndvar->on_control_pkt_list)
				{

					TAILQ_REMOVE(&etcp->timewait_list, walk, sndvar->timer_link);
					walk->on_timewait_list = FALSE;
					etcp->timewait_list_cnt--;

					walk->state = TCP_ST_CLOSED;
					walk->close_reason = TCP_ACTIVE_CLOSE;
					DestroyFlow(etcp, walk);
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			fprintf(stderr, "flow %d: not on timewait list.\n", walk->id);
		}
	}
}
/*----------------------------------------------------------------------------*/
void CheckConnectionTimeout(flow_manager_t etcp, uint32_t cur_ts, int thresh)
{
	tcp_flow *walk, *next;
	int cnt;

	STAT_COUNT(etcp->runstat.rounds_tocheck);

	cnt = 0;
	for (walk = TAILQ_FIRST(&etcp->timeout_list);
		 walk != NULL; walk = next)
	{
		if (++cnt > thresh)
			break;
		next = TAILQ_NEXT(walk, sndvar->timeout_link);

		if ((int32_t)(cur_ts - walk->last_active_ts) >=
			CONFIG.tcp_timeout)
		{

			walk->on_timeout_list = FALSE;
			TAILQ_REMOVE(&etcp->timeout_list, walk, sndvar->timeout_link);
			etcp->timeout_list_cnt--;
			walk->state = TCP_ST_CLOSED;
			walk->close_reason = TCP_TIMEDOUT;
			if (walk->socket)
			{
				RegisterErrorEvent(etcp, walk);
			}
			else
			{
				DestroyFlow(etcp, walk);
			}
		}
		else
		{
			break;
		}
	}
}
/*----------------------------------------------------------------------------*/
