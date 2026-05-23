#include <assert.h>
#include <time.h>
#include <inttypes.h>

#include "util.h"
#include "tcp_in.h"
#include "tcp_out.h"
#include "receive_buffer.h"
#include "eventpoll.h"
#include "timer.h"
#include "ip_in.h"
#include "clock.h"
#include "cc_trace.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static inline int
FilterSYN(flow_manager_t etcp, uint32_t ip, uint16_t port)
{
	struct sockaddr_in *addr;
	struct tcp_listener *listener;
	listener = (struct tcp_listener *)SearchListnerHT(etcp->listeners, &port);
	if (listener == NULL)
		return FALSE;

	addr = &listener->socket->saddr;

	if (addr->sin_port == port)
	{
		if (addr->sin_addr.s_addr != INADDR_ANY)
		{
			if (ip == addr->sin_addr.s_addr)
			{
				return TRUE;
			}
			return FALSE;
		}
		else
		{
			int i;

			for (i = 0; i < CONFIG.eths_num; i++)
			{
				if (ip == CONFIG.eths[i].ip_addr)
				{
					return TRUE;
				}
			}
			return FALSE;
		}
	}

	return FALSE;
}
/*----------------------------------------------------------------------------*/

static inline tcp_flow *
OpenPassive(flow_manager_t etcp, uint32_t cur_ts, const struct iphdr *iph,
			const struct tcphdr *tcph, uint32_t seq, uint16_t window)
{
	tcp_flow *cur_flow = NULL;
	/**************************************************************************** */
	// TODO section
	/* PART_1: Connection Management - Passive Open */
	/*
	 * Called when a SYN arrives on a listening socket.
	 * Create a new flow with CreateFlow() and initialize it
	 * using information from the incoming SYN, update:
	 *   - rcvvar->irs, sndvar->peer_wnd, rcv_nxt
	 * Return the new flow on success, NULL on failure.
	 */

	cur_flow = CreateFlow(etcp, NULL, TRUE,
						  iph->daddr, tcph->dest, iph->saddr, tcph->source);
	if (!cur_flow)
		return NULL;

	cur_flow->rcvvar->irs = seq;
	cur_flow->rcv_nxt = seq;
	cur_flow->sndvar->peer_wnd = window;


	/**************************************************************************** */
	cur_flow->sndvar->cwnd = 1;
	return cur_flow;
}
/*----------------------------------------------------------------------------*/
static inline int
OpenActive(flow_manager_t etcp, tcp_flow *cur_flow, uint32_t cur_ts,
		   struct tcphdr *tcph, uint32_t seq, uint32_t ack_seq, uint16_t window)
{
	/**************************************************************************** */
	// TODO section
	/* PART_1: Connection Management - Active Open */
	/*
	 * Called when a SYN-ACK is received on an active-open flow.
	 * Update flow state to reflect the completed handshake:
	 *   - rcvvar->irs, snd_nxt, sndvar->peer_wnd,
	 *     rcvvar->snd_wl1, rcv_nxt, rcvvar->last_ack_seq
	 *   - arm the retransmission timer
	 * Return TRUE on success.
	 *
	 * Note: SYN consumes one sequence number on both sides.
	 */

	cur_flow->rcvvar->irs = seq;
	cur_flow->rcv_nxt = seq + 1;
	cur_flow->snd_nxt = ack_seq;
	cur_flow->sndvar->peer_wnd = window;
	cur_flow->rcvvar->snd_wl1 = seq;
	cur_flow->rcvvar->last_ack_seq = ack_seq;

	
	/**************************************************************************** */
	cur_flow->sndvar->cwnd = ((cur_flow->sndvar->cwnd == 1) ? (cur_flow->sndvar->mss * TCP_INIT_CWND) : cur_flow->sndvar->mss);
	cur_flow->sndvar->ssthresh = cur_flow->sndvar->mss * 10;
	return TRUE;
}
/*----------------------------------------------------------------------------*/
/* CheckSequenceValid: validates sequence number of the segment                 */
/* Return: TRUE if acceptable, FALSE if not acceptable                        */
/*----------------------------------------------------------------------------*/
static inline int
CheckSequenceValid(flow_manager_t etcp, tcp_flow *cur_flow, uint32_t cur_ts,
				   struct tcphdr *tcph, uint32_t seq, uint32_t ack_seq, int payloadlen)
{

	/* TCP sequence validation */
	if (!TCP_SEQ_BETWEEN(seq + payloadlen, cur_flow->rcv_nxt,
						 cur_flow->rcv_nxt + cur_flow->rcvvar->rcv_wnd))
	{

		/* if RST bit is set, ignore the segment */
		if (tcph->rst)
			return FALSE;

		if (cur_flow->state == TCP_ST_ESTABLISHED)
		{
			/* check if it is to get window advertisement */
			if (seq + 1 == cur_flow->rcv_nxt)
			{
				RegisterACK(etcp, cur_flow, cur_ts, ACK_OPT_AGGREGATE);
				return FALSE;
			}

			if (TCP_SEQ_LEQ(seq, cur_flow->rcv_nxt))
			{
				RegisterACK(etcp, cur_flow, cur_ts, ACK_OPT_AGGREGATE);
			}
			else
			{
				RegisterACK(etcp, cur_flow, cur_ts, ACK_OPT_NOW);
			}
		}
		else
		{
			if (cur_flow->state == TCP_ST_TIME_WAIT)
			{
				RegisterToTimewaitList(etcp, cur_flow, cur_ts);
			}
			RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
		}
		return FALSE;
	}

	return TRUE;
}
/*----------------------------------------------------------------------------*/
static inline int
HandleRST(flow_manager_t etcp, tcp_flow *cur_flow, uint32_t ack_seq)
{
	if (cur_flow->state <= TCP_ST_SYN_SENT)
	{
		return FALSE;
	}

	if (cur_flow->state == TCP_ST_SYN_RCVD)
	{
		if (ack_seq == cur_flow->snd_nxt)
		{
			cur_flow->state = TCP_ST_CLOSED;
			cur_flow->close_reason = TCP_RESET;
			DestroyFlow(etcp, cur_flow);
		}
		return TRUE;
	}

	/* if the application is already closed the connection,
	   just destroy it */
	if (cur_flow->state == TCP_ST_FIN_WAIT_1 ||
		cur_flow->state == TCP_ST_FIN_WAIT_2 ||
		cur_flow->state == TCP_ST_LAST_ACK ||
		cur_flow->state == TCP_ST_CLOSING ||
		cur_flow->state == TCP_ST_TIME_WAIT)
	{
		cur_flow->state = TCP_ST_CLOSED;
		cur_flow->close_reason = TCP_ACTIVE_CLOSE;
		DestroyFlow(etcp, cur_flow);
		return TRUE;
	}

	if (!(cur_flow->sndvar->on_closeq || cur_flow->sndvar->on_closeq_int ||
		  cur_flow->sndvar->on_resetq || cur_flow->sndvar->on_resetq_int))
	{
		cur_flow->state = TCP_ST_CLOSE_WAIT;
		cur_flow->close_reason = TCP_RESET;
		RegisterCloseEvent(etcp, cur_flow);
	}

	return TRUE;
}
/*----------------------------------------------------------------------------*/
#ifndef TCP_RTO_MIN
#define TCP_RTO_MIN 100
#endif

#ifndef TCP_RTO_MAX
#define TCP_RTO_MAX 60000
#endif

static inline uint32_t
ClampRTO(uint32_t rto)
{
	if (rto < TCP_RTO_MIN)
		return TCP_RTO_MIN;
	if (rto > TCP_RTO_MAX)
		return TCP_RTO_MAX;
	return rto;
}

/*
 * Uses existing fields:
 *   rcvvar->srtt     : smoothed RTT scaled by 8
 *   rcvvar->mdev     : mean deviation
 *   rcvvar->mdev_max : max deviation seen in current RTT period
 *   rcvvar->rttvar   : smoothed max deviation
 *   rcvvar->rtt_seq  : seq boundary for refreshing rttvar
 *
 * This is a simplified Jacobson/Karels-style estimator.
 */
static inline void
EstimateRTT(tcp_flow *cur_flow, uint32_t mrtt)
{
	struct recv_var_tcp *rcvvar = cur_flow->rcvvar;
	struct send_var_tcp *sndvar = cur_flow->sndvar;
	int32_t m;

	if (mrtt == 0)
		mrtt = 1;

	/* first RTT sample */
	if (rcvvar->srtt == 0)
	{
		rcvvar->srtt = mrtt << 3; /* scaled by 8 */
		rcvvar->mdev = mrtt << 1; /* ~= 2 * mrtt */
		rcvvar->mdev_max = rcvvar->mdev;
		rcvvar->rttvar = rcvvar->mdev_max;
		rcvvar->rtt_seq = cur_flow->snd_nxt;
	}
	else
	{
		/* update srtt: srtt += (mrtt - srtt/8) */
		m = (int32_t)mrtt - (int32_t)(rcvvar->srtt >> 3);
		rcvvar->srtt += m;

		/* update mean deviation */
		if (m < 0)
			m = -m;
		m -= (int32_t)(rcvvar->mdev >> 2);
		rcvvar->mdev += m;

		/* track max deviation within a RTT period */
		if (rcvvar->mdev > rcvvar->mdev_max)
		{
			rcvvar->mdev_max = rcvvar->mdev;
			if (rcvvar->mdev_max > rcvvar->rttvar)
				rcvvar->rttvar = rcvvar->mdev_max;
		}
		/*
		 * Once snd_una passes rtt_seq, start a new RTT period and
		 * slowly adapt rttvar toward the recent max deviation.
		 */
		if (TCP_SEQ_GEQ(sndvar->snd_una, rcvvar->rtt_seq))
		{
			rcvvar->rtt_seq = cur_flow->snd_nxt;
			rcvvar->rttvar -= (rcvvar->rttvar >> 2);
			if (rcvvar->mdev_max > rcvvar->rttvar)
				rcvvar->rttvar = rcvvar->mdev_max;
			rcvvar->mdev_max = TCP_RTO_MIN;
		}
	}

	sndvar->rto = (rcvvar->srtt >> 3) + rcvvar->rttvar;
	sndvar->rto = ClampRTO(sndvar->rto);
}
/*
 * We reuse:
 *   sndvar->ts_rto = send_ts + rto
 * so send_ts can be approximated as:
 *   send_ts = ts_rto - rto
 *
 *   if retransmission happened (nrtx > 0), do not use this ACK for RTT.
 */
static inline void
TryUpdateRTTOnACK(tcp_flow *cur_flow, uint32_t cur_ts)
{
	struct send_var_tcp *sndvar = cur_flow->sndvar;
	uint32_t send_ts;
	uint32_t mrtt;

	/* retransmitted data => ambiguous ACK => do not sample RTT */
	if (sndvar->nrtx > 0)
		return;

	/* guard against underflow / uninitialized state */
	if (sndvar->ts_rto < sndvar->rto)
		return;

	send_ts = sndvar->ts_rto - sndvar->rto;
	if (cur_ts < send_ts)
		return;

	mrtt = cur_ts - send_ts;
	if (mrtt == 0)
		mrtt = 1;

	EstimateRTT(cur_flow, mrtt);
}
/*----------------------------------------------------------------------------*/
/* HandleACK */
/* function parameter explanation
 * cur_ts: current timestamp
 * tcph: tcp header of the received segment
 * seq: sequence number of incoming TCP segment
 * ack_seq: acknowledgment number of incoming TCP segment(what the sender of the segment expects to receive next)
 * window: advertised window of the sender of the segment
 * payloadlen: payload length of the received segment
 */
static inline void
HandleACK(flow_manager_t etcp, tcp_flow *cur_flow, uint32_t cur_ts,
		  struct tcphdr *tcph, uint32_t seq, uint32_t ack_seq,
		  uint16_t window, int payloadlen)
{
	struct send_var_tcp *sndvar = cur_flow->sndvar;
	uint32_t cwindow;	 // current peer advertised window
	uint8_t dup = FALSE; // flag for detecting duplicated ACKs

	cwindow = window;
	/*
	 * In  closing states, FIN occupies one sequence number.
	 * Adjust ACK handling carefully so ACK range checks remain valid.
	 */
	if (cur_flow->state == TCP_ST_FIN_WAIT_1 ||
		cur_flow->state == TCP_ST_FIN_WAIT_2 ||
		cur_flow->state == TCP_ST_CLOSING ||
		cur_flow->state == TCP_ST_CLOSE_WAIT ||
		cur_flow->state == TCP_ST_LAST_ACK)
	{
		if (sndvar->is_fin_sent && ack_seq == sndvar->fss + 1)
		{
			ack_seq--;
		}
	}

	/* Ignore ACKs that acknowledge beyond what exists in the send buffer. */
	if (TCP_SEQ_GT(ack_seq, sndvar->sndbuf->head_seq + sndvar->sndbuf->len))
	{
		return;
	}

	/**************************************************************************** */
	// TODO section
	/* PART_2: Reliable Data Transfer and Flow Control: Flow Control
	 * The receiver advertises its available buffer size (rwnd) in the TCP header.
	 * The sender must respect this value for flow control.
	 *
	 * However, ACK segments may arrive out of order or be duplicated. Therefore,
	 * we only need to update the advertised window if the segment carrying the window
	 * information is "newer" than the last window update.
	 *
	 * snd_wl1 : sequence number of the segment that last updated the window
	 * snd_wl2 : acknowledgment number of the segment that last updated the window
	 *
	 * Only update peer_wnd if:
	 *   1) this segment has a larger sequence number, or
	 *   2) the sequence is the same but the acknowledgment number is larger, or
	 *   3) both are the same but the advertised window becomes larger.
	 * Also update
	 * 		rccvar->snd_wl1, snd_wl2
	 * If the receiver window was previously too small to send more data but is now
	 * large enough ==> notify the application that the socket is writable again.
	 */
	uint32_t prev_peer_wnd = sndvar->peer_wnd;
	if (TCP_SEQ_LT(cur_flow->rcvvar->snd_wl1, seq) ||
		(cur_flow->rcvvar->snd_wl1 == seq &&
		 TCP_SEQ_LT(cur_flow->rcvvar->snd_wl2, ack_seq)) ||
		(cur_flow->rcvvar->snd_wl1 == seq &&
		 cur_flow->rcvvar->snd_wl2 == ack_seq &&
		 cwindow > sndvar->peer_wnd))
	{
		sndvar->peer_wnd = cwindow;
		cur_flow->rcvvar->snd_wl1 = seq;
		cur_flow->rcvvar->snd_wl2 = ack_seq;
		if (prev_peer_wnd == 0 && cwindow > 0)
			RegisterWriteEvent(etcp, cur_flow);
	}

	/**************************************************************************** */

	/**************************************************************************** */
	// TODO section
	/* PART_2: Reliable Data Transfer and Flow Control: Detect Duplicated ACK
	 *	Duplicated ACK Conditions:
	 *  	1.ACK number does not advance
	 *  	2.no payload is carried
	 *  	3.no meaningful new window update
	 *	if detected, update
	 *		rcvvar -> dup_acks
	 *		dup flag
	 *
	 */

	if (ack_seq == sndvar->snd_una && payloadlen == 0 &&
		sndvar->peer_wnd == prev_peer_wnd)
	{
		cur_flow->rcvvar->dup_acks++;
		dup = TRUE;
	}

	/**************************************************************************** */

	/* 3 duplicated ACKs */
	if (dup && cur_flow->rcvvar->dup_acks == 3)
	{
		sndvar->in_fast_recovery = 1; // sender is in fast_recovery phase of congestion control
		/**************************************************************************** */
		// TODO section
		/* Part2 Bonus: Update congestion-control variables in fast retransmit phase */
		sndvar->ssthresh = MAX(sndvar->cwnd / 2, 2 * sndvar->mss);
		sndvar->cwnd = sndvar->ssthresh + 3 * sndvar->mss;
		CC_LOG(cur_flow, cur_ts, "fast_retx", ack_seq, NULL);

		/**************************************************************************** */

		/**************************************************************************** */
		// TODO section
		/* PART_2: Implement Fast retransmission */
		/* If 3 duplicate ACKs are observed, assume one segment is lost and
		 * schedule immediate retransmission without waiting for retranmission timeout.
		 *
		 * Update
		 * 	  flow: snd_nxt
		 */
		cur_flow->snd_nxt = sndvar->snd_una;
		RegisterflowToSendPktList(etcp, cur_flow);
		/**************************************************************************** */
		return;
	}

	/* additional duplicate ACKs during fast recovery */
	if (dup && sndvar->in_fast_recovery)
	{
		/**************************************************************************** */
		// TODO section
		// Part2 Bonus: Part2 Bonus: Update congestion-control variables in fast recovery phase
		sndvar->cwnd += sndvar->mss;
		CC_LOG(cur_flow, cur_ts, "dup_ack_recovery", ack_seq, NULL);
		/**************************************************************************** */
		return;
	}

	/* If ack_seq is previously acked, return */
	if (TCP_SEQ_GEQ(sndvar->sndbuf->head_seq, ack_seq))
	{
		return;
	}

	/* now this section is for hanlding newly advancing ACks*/
	uint32_t rmlen; // removal length of the sending buffer after receiving the ack
	rmlen = ack_seq - sndvar->sndbuf->head_seq;
	if (rmlen > 0)
	{
		TryUpdateRTTOnACK(cur_flow, cur_ts);

		if (cur_flow->state >= TCP_ST_ESTABLISHED)
		{
			/**************************************************************************** */
			// TODO section
			// Part2 Bonus:cogestion window update on newly advancing ACK
			/*
			 *
			 * Implement three phases:
			 * 0. Fast Recovery Exit
			 * 1. Slow Start
			 * 2. Congestion Avoidance
			 * Notes:
			 * 'packets' is the number of MSS-sized packets newly acknowledged.
			 *  Be careful about integer overflow when increasing cwnd.
			 *  Only grow cwnd when the ACK actually advances the window
			 *   (that is why this block runs only when rmlen > 0).
			 */
			if (sndvar->in_fast_recovery)
			{
				sndvar->in_fast_recovery = 0;
				cur_flow->rcvvar->dup_acks = 0;
				/**************************************************************************** */
				sndvar->cwnd = sndvar->ssthresh;
				CC_LOG(cur_flow, cur_ts, "recovery_exit", ack_seq, NULL);
				/**************************************************************************** */
			}
			else if (sndvar->cwnd < sndvar->ssthresh)
			{
				/**************************************************************************** */
				sndvar->cwnd += sndvar->mss;
				CC_LOG(cur_flow, cur_ts, "slow_start", ack_seq, NULL);
				/**************************************************************************** */
			}
			else
			{
				/**************************************************************************** */
				uint32_t inc = sndvar->mss * sndvar->mss / sndvar->cwnd;
				if (inc == 0)
					inc = 1;
				sndvar->cwnd += inc;
				CC_LOG(cur_flow, cur_ts, "cong_avoid", ack_seq, NULL);
				/**************************************************************************** */
			}
		}
		/**************************************************************************** */

		pthread_mutex_lock(&sndvar->write_lock);
		/**************************************************************************** */
		// TODO section
		/* PART_2: Reliable Data Transfer and Flow Control:Update sender-side bookkeeping after ACK */
		/*
		 * remove acknowledged bytes from sndbuf
		 * Update
		 * 	 sndvar: snd_una, snd_wnd
		 *
		 * If the application was blocked before, raise a write event
		 * when new buffer space becomes available.
		 */
		RemoveFromSendBuffer(sndvar->sndbuf, rmlen);
		sndvar->snd_una = ack_seq;
		sndvar->snd_wnd = sndvar->sndbuf->size - sndvar->sndbuf->len;
		if (sndvar->snd_wnd > 0)
			RegisterWriteEvent(etcp, cur_flow);
		/**************************************************************************** */
		pthread_mutex_unlock(&sndvar->write_lock);

		/**************************************************************************** */
		// TODO section
		/* PART 1+2: General ACK Handling*/
		/*
		 * Update
			snd_una
		 * Handle Retransmission timer
		 */
		RenewRetransmissionTimer(etcp, cur_flow, cur_ts);
		/**************************************************************************** */
		if (TCP_SEQ_GT(ack_seq, cur_flow->snd_nxt))
		{
			cur_flow->snd_nxt = ack_seq;

			if (sndvar->sndbuf->len == 0)
			{
				RemoveFromDataPktList(etcp, cur_flow);
			}
		}
		cur_flow->rcvvar->dup_acks = 0;
		cur_flow->rcvvar->last_ack_seq = ack_seq;
	}
}
/*----------------------------------------------------------------------------*/
/* HandlePayload: merges TCP payload using receive ring buffer            */
/* Return: TRUE (1) in normal case, FALSE (0) if immediate ACK is required    */
/* CAUTION: should only be called at ESTABLISHED, FIN_WAIT_1, FIN_WAIT_2      */
/*----------------------------------------------------------------------------*/
static inline int
HandlePayload(flow_manager_t etcp, tcp_flow *cur_flow,
			  uint32_t cur_ts, uint8_t *payload, uint32_t seq, int payloadlen)
{
	struct recv_var_tcp *rcvvar = cur_flow->rcvvar;
	/**************************************************************************** */
	// TODO section
	/* PART_2: Reliable Data Transfer - Sequence Validation */
	/*
	 * Reject payload that is completely old or lies outside the current
	 * receive window. In such cases the receiver must not merge any data.
	 * Hint: use rcv_nxt and rcvvar->rcv_wnd to define the valid range.
	 */
	if (TCP_SEQ_LT(seq + payloadlen, cur_flow->rcv_nxt))
		return FALSE;
	if (TCP_SEQ_GT(seq, cur_flow->rcv_nxt + rcvvar->rcv_wnd))
		return FALSE;

	/**************************************************************************** */

	/* allocate receive buffer if not exist */
	if (!rcvvar->rcvbuf)
	{
		rcvvar->rcvbuf = InitReceiveBuffer(rcvvar->irs + 1);
		if (!rcvvar->rcvbuf)
		{
			fprintf(stderr, "flow %d: Failed to allocate receive buffer.\n",
					cur_flow->id);
			cur_flow->state = TCP_ST_CLOSED;
			cur_flow->close_reason = TCP_NO_MEM;
			RegisterErrorEvent(etcp, cur_flow);

			return FALSE;
		}
	}

	pthread_mutex_lock(&rcvvar->read_lock);
	/**************************************************************************** */
	// TODO section
	/* PART_2: Reliable Data Transfer - Receive Buffer */
	/*
	 * Insert the payload into the receive buffer
	 * Update
	 * 	 flow: rcv_nxt
	 * 	 rcvvar: rcv_wnd
	 * Hint: rcvbuf->head_seq + rcvbuf->merged_len gives the next expected byte.
	 *
	 * Handle special case: if state is FIN_WAIT_1 or FIN_WAIT_2 the local side has
	 * already closed for writing, so immediately discard the received data
	 * from the buffer after inserting it.
	 */
	uint32_t prev_rcv_nxt = cur_flow->rcv_nxt;
	if (InsertToReceiveBuffer(rcvvar->rcvbuf, payload, payloadlen, seq) < 0)
	{
		pthread_mutex_unlock(&rcvvar->read_lock);
		return FALSE;
	}
	cur_flow->rcv_nxt = rcvvar->rcvbuf->head_seq + rcvvar->rcvbuf->merged_len;
	rcvvar->rcv_wnd = rcvvar->rcvbuf->size - rcvvar->rcvbuf->merged_len;

	if (cur_flow->state == TCP_ST_FIN_WAIT_1 ||
		cur_flow->state == TCP_ST_FIN_WAIT_2)
	{
		RemoveFromReceiveBuffer(rcvvar->rcvbuf, rcvvar->rcvbuf->merged_len, AT_APP);
		rcvvar->rcv_wnd = rcvvar->rcvbuf->size - rcvvar->rcvbuf->merged_len;
	}

	/**************************************************************************** */
	pthread_mutex_unlock(&rcvvar->read_lock);

	/**************************************************************************** */
	// TODO section
	/* PART_2: Reliable Data Transfer - Read Event and ACK Decision */
	/*
	 * If the payload does not advance rcv_nxt, return FALSE so the caller
	 * can send an immediate ACK.
	 *
	 * If rcv_nxt advances and the flow is in ESTABLISHED, raise a read event
	 * because new contiguous data is now available to the application.
	 */
	if (cur_flow->rcv_nxt == prev_rcv_nxt)
		return FALSE;

	if (cur_flow->state == TCP_ST_ESTABLISHED)
		RegisterReadEvent(etcp, cur_flow);

	/**************************************************************************** */
	return TRUE;
}
/*----------------------------------------------------------------------------*/
static inline tcp_flow *
CreateHashTableEntry(flow_manager_t etcp, uint32_t cur_ts, const struct iphdr *iph,
					 int ip_len, const struct tcphdr *tcph, uint32_t seq, uint32_t ack_seq,
					 int payloadlen, uint16_t window)
{
	tcp_flow *cur_flow;
	int ret;

	if (tcph->syn && !tcph->ack)
	{
		/* handle the SYN */
		ret = FilterSYN(etcp, iph->daddr, tcph->dest);
		if (!ret)
		{
			fprintf(stderr, "Refusing SYN packet.\n");
			MakeTCPPacketWotFlow(etcp,
								 iph->daddr, tcph->dest, iph->saddr, tcph->source,
								 0, seq + payloadlen + 1, TCP_FLAG_RST | TCP_FLAG_ACK,
								 cur_ts);

			return NULL;
		}

		/* now accept the connection */
		cur_flow = OpenPassive(etcp,
							   cur_ts, iph, tcph, seq, window);
		if (!cur_flow)
		{
			fprintf(stderr, "Not available space in flow pool.\n");
			MakeTCPPacketWotFlow(etcp,
								 iph->daddr, tcph->dest, iph->saddr, tcph->source,
								 0, seq + payloadlen + 1, TCP_FLAG_RST | TCP_FLAG_ACK,
								 cur_ts);

			return NULL;
		}

		return cur_flow;
	}

	else if (tcph->rst)
	{
		fprintf(stderr, "Reset packet comes\n");
		return NULL;
	}

	else
	{
		fprintf(stderr, "Weird packet comes.\n");
		if (tcph->ack)
		{
			MakeTCPPacketWotFlow(etcp,
								 iph->daddr, tcph->dest, iph->saddr, tcph->source,
								 ack_seq, 0, TCP_FLAG_RST, cur_ts);
		}
		else
		{
			MakeTCPPacketWotFlow(etcp,
								 iph->daddr, tcph->dest, iph->saddr, tcph->source,
								 0, seq + payloadlen, TCP_FLAG_RST | TCP_FLAG_ACK,
								 cur_ts);
		}
		return NULL;
	}
}

/*----------------------------------------------------------------------------*/
static inline void
LISTEN_Handler(flow_manager_t etcp, uint32_t cur_ts,
			   tcp_flow *cur_flow, struct tcphdr *tcph)
{
	/**************************************************************************** */
	// TODO section
	/* PART_1: Connection Management - LISTEN state */
	/*
	 * Only a SYN is valid here. Reject anything else.
	 * On SYN: advance rcv_nxt (SYN consumes one seq number),
	 * transition to TCP_ST_SYN_RCVD, and schedule a SYN-ACK.
	 *
	 * Note: only increment rcv_nxt when currently in TCP_ST_LISTEN
	 * (a retransmitted SYN may arrive while already in SYN_RCVD).
	 */

	if (!tcph->syn || tcph->ack || tcph->rst)
		return;

	if (cur_flow->state == TCP_ST_LISTEN)
		cur_flow->rcv_nxt++;

	cur_flow->state = TCP_ST_SYN_RCVD;
	RegisterflowToControlPktList(etcp, cur_flow, cur_ts);


	/**************************************************************************** */
}
/*----------------------------------------------------------------------------*/
static inline void
SYN_SENT_Handler(flow_manager_t etcp, uint32_t cur_ts,
				 tcp_flow *cur_flow, const struct iphdr *iph, struct tcphdr *tcph,
				 uint32_t seq, uint32_t ack_seq, int payloadlen, uint16_t window)
{
	/*
	 * ACK validation: if ACK bit is set, reject ack_seq outside (iss, snd_nxt].
	 *   Send RST for bad acks; otherwise accept by advancing snd_una.
	 */
	if (tcph->ack)
	{ /* filter the unacceptable acks */
		if (TCP_SEQ_LEQ(ack_seq, cur_flow->sndvar->iss) ||
			TCP_SEQ_GT(ack_seq, cur_flow->snd_nxt))
		{
			if (!tcph->rst)
			{
				// send RST packete right away
				MakeTCPPacketWotFlow(etcp,
									 iph->daddr, tcph->dest, iph->saddr, tcph->source,
									 ack_seq, 0, TCP_FLAG_RST, cur_ts);
			}
			return;
		}
		/* accept the ack */
		cur_flow->sndvar->snd_una++;
	}
	// RST handling: if RST + valid ACK, connection refused → close.
	if (tcph->rst)
	{
		if (tcph->ack)
		{
			cur_flow->state = TCP_ST_CLOSE_WAIT;
			cur_flow->close_reason = TCP_RESET;
			if (cur_flow->socket)
			{
				RegisterErrorEvent(etcp, cur_flow);
			}
			else
			{
				DestroyFlow(etcp, cur_flow);
			}
		}
		return;
	}
	/**************************************************************************** */
	// TODO section
	/* PART_1: Connection Management - SYN_SENT state */
	/*
	 * Waiting for SYN-ACK after sending SYN (active open).
	 *   - if SYN + ACK (normal SYN-ACK): call OpenActive(you must implement it first)
	 * 	 	- update
	 * 			- sndvar: nrtx
	 * 			- flow: rcv_nxt, state
	 *  	- handle RTO list and Timeout list
	 *   	- fire write event and schedule outgoing control packet
	 *
	 *   - if SYN only (simultaneous open)
	 * 		- update
	 * 			 - flow: state, snd_nxt
	 * 			 - schedule outgoing control packet
	 *
	 */

	if (tcph->syn && tcph->ack)
	{
		if (!OpenActive(etcp, cur_flow, cur_ts, tcph, seq, ack_seq, window))
			return;

		cur_flow->sndvar->nrtx = 0;
		RemoveFromRTOList(etcp, cur_flow);
		cur_flow->state = TCP_ST_ESTABLISHED;
		RegisterWriteEvent(etcp, cur_flow);
		RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
	}
	else if (tcph->syn)
	{
		cur_flow->state = TCP_ST_SYN_RCVD;
		cur_flow->snd_nxt = cur_flow->sndvar->iss;
		RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
	}
	/**************************************************************************** */
}
/*----------------------------------------------------------------------------*/
static inline void
SYN_RCVD_Handler(flow_manager_t etcp, uint32_t cur_ts,
				 tcp_flow *cur_flow, struct tcphdr *tcph, uint32_t ack_seq)
{
	struct send_var_tcp *sndvar = cur_flow->sndvar;
	int ret;
	if (tcph->ack)
	{
		struct tcp_listener *listener;
		uint32_t prior_cwnd;
		if (ack_seq != sndvar->iss + 1)
		{
			return;
		}
		// initialize congestion control variables
		prior_cwnd = sndvar->cwnd;
		sndvar->cwnd = ((prior_cwnd == 1) ? (sndvar->mss * TCP_INIT_CWND) : sndvar->mss);
		sndvar->ssthresh = cur_flow->sndvar->mss * 10;
		sndvar->nrtx = 0;
		/**************************************************************************** */
		// TODO section
		/* PART_1: Connection Management - SYN_RCVD state */
		/*
		 * Waiting for the final ACK of the three-way handshake (passive open).
		 *
		 * ACK received:
		 *  Update
		 * 		- sndvar: snd_una
		 *  	- flow: rcv_nxt, snd_nxt, and state
		 *  Handle RTO list.
		 */
		sndvar->snd_una = ack_seq;
		cur_flow->snd_nxt = ack_seq;
		cur_flow->state = TCP_ST_ESTABLISHED;
		RemoveFromRTOList(etcp, cur_flow);


		/**************************************************************************** */

		/*Enqueue the flow to the listener's accept queue
		 *   (use SearchListnerHT to find the listener).
		 *   On enqueue failure, mark TCP_NOT_ACCEPTED and schedule RST.
		 *   Register timeout and signal EPOLLIN on the listener socket.
		 *
		 * No ACK (retransmitted SYN received): retransmit SYN-ACK.
		 */
		listener = (struct tcp_listener *)SearchListnerHT(etcp->listeners, &tcph->dest);
		ret = EnqueueToFlowQ(listener->acceptq, cur_flow);
		if (ret < 0)
		{
			cur_flow->close_reason = TCP_NOT_ACCEPTED;
			cur_flow->state = TCP_ST_CLOSED;
			RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
		}
		if (CONFIG.tcp_timeout > 0)
			RegisterToTimeoutList(etcp, cur_flow);

		if (listener->socket && (listener->socket->epoll & etcp_EPOLLIN))
		{
			AddEventToEpollQ(etcp->ep,
							 etcp_EVENT_QUEUE, listener->socket, etcp_EPOLLIN);
		}
	}
	else
	{
		/* retransmit SYN/ACK */
		cur_flow->snd_nxt = sndvar->iss;
		RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
	}
}
/*----------------------------------------------------------------------------*/
static inline void
ESTABLISHED_Handler(flow_manager_t etcp, uint32_t cur_ts,
					tcp_flow *cur_flow, struct tcphdr *tcph, uint32_t seq, uint32_t ack_seq,
					uint8_t *payload, int payloadlen, uint16_t window)
{
	/*
	 * 1. SYN (retransmitted): re-send ACK and return.
	 */
	if (tcph->syn)
	{
		cur_flow->snd_nxt = ack_seq;
		RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
		return;
	}

	/*
	 * 2. Payload present: call HandlePayload.
	 *      On success (in-order data): schedule ACK_OPT_AGGREGATE.
	 *      On failure (out-of-order / invalid): schedule ACK_OPT_NOW.
	 * 3. ACK set and sndbuf exists: call HandleACK to process sender state.
	 */
	if (payloadlen > 0)
	{
		if (HandlePayload(etcp, cur_flow,
						  cur_ts, payload, seq, payloadlen))
		{
			RegisterACK(etcp, cur_flow, cur_ts, ACK_OPT_AGGREGATE);
		}
		else
		{
			RegisterACK(etcp, cur_flow, cur_ts, ACK_OPT_NOW);
		}
	}
	if (tcph->ack)
	{
		if (cur_flow->sndvar->sndbuf)
		{
			HandleACK(etcp, cur_flow, cur_ts,
					  tcph, seq, ack_seq, window, payloadlen);
		}
	}

	/**************************************************************************** */
	// TODO section
	/* PART_1: Connection Management - ESTABLISHED state */
	/*
	 *
	 * 4. FIN set:
	 *      If in-order FIN comes:
	 * 			- Update
	 * 				- flow: stae, rcv_nxt
	 *        	- schedule outgoing control packet, fire read event(EOF to the application).
	 *
	 *      If out-of-order: schedule immediate ACK only.
	 * 	  HINT: FIN consumes an additional one sequence number!
	 */

	if (tcph->fin)
	{
		if (seq + payloadlen == cur_flow->rcv_nxt)
		{
			cur_flow->rcv_nxt++;
			cur_flow->state = TCP_ST_CLOSE_WAIT;
			RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
			RegisterReadEvent(etcp, cur_flow);
		}
		else
		{
			RegisterACK(etcp, cur_flow, cur_ts, ACK_OPT_NOW);
		}
	}

	/**************************************************************************** */
}
/*----------------------------------------------------------------------------*/
static inline void
CLOSE_WAIT_Handler(flow_manager_t etcp, uint32_t cur_ts,
				   tcp_flow *cur_flow, struct tcphdr *tcph, uint32_t seq, uint32_t ack_seq,
				   int payloadlen, uint16_t window)
{
	if (TCP_SEQ_LT(seq, cur_flow->rcv_nxt))
	{
		RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
		return;
	}

	if (cur_flow->sndvar->sndbuf)
	{
		HandleACK(etcp, cur_flow, cur_ts,
				  tcph, seq, ack_seq, window, payloadlen);
	}
}
/*----------------------------------------------------------------------------*/
static inline void
LAST_ACK_Handler(flow_manager_t etcp, uint32_t cur_ts, const struct iphdr *iph,
				 int ip_len, tcp_flow *cur_flow, struct tcphdr *tcph,
				 uint32_t seq, uint32_t ack_seq, int payloadlen, uint16_t window)
{
	if (TCP_SEQ_LT(seq, cur_flow->rcv_nxt))
	{
		return;
	}

	if (tcph->ack)
	{
		if (cur_flow->sndvar->sndbuf)
		{
			HandleACK(etcp, cur_flow, cur_ts,
					  tcph, seq, ack_seq, window, payloadlen);
		}

		if (!cur_flow->sndvar->is_fin_sent)
		{
			/* the case that FIN is not sent yet */
			/* this is not ack for FIN, ignore */
			return;
		}
		if (ack_seq == cur_flow->sndvar->fss + 1)
		{
			cur_flow->sndvar->snd_una = ack_seq;
			cur_flow->state = TCP_ST_CLOSED;
			cur_flow->close_reason = TCP_PASSIVE_CLOSE;
			RemoveFromRTOList(etcp, cur_flow);
			DestroyFlow(etcp, cur_flow);
		}
		else
		{
			RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
		}
	}
	else
	{
		RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
	}
}
/*----------------------------------------------------------------------------*/
static inline void
FIN_WAIT_1_Handler(flow_manager_t etcp, uint32_t cur_ts,
				   tcp_flow *cur_flow, struct tcphdr *tcph, uint32_t seq, uint32_t ack_seq,
				   uint8_t *payload, int payloadlen, uint16_t window)
{
	/**************************************************************************** */
	// TODO section
	/* PART_1: Connection Management - FIN_WAIT_1 state */
	/*
	 * We sent a FIN and are waiting for it to be ACKed (active close).
	 * Discard stale packets (seq < rcv_nxt)
	 * 	 - schedule outgoing control packet
	 *
	 * ACK received:
	 *   - Handle it if sndbuf exists
	 *   - If this packet acks our FIN:
	 *       Update
	 * 			- flow: snd_una, snd_nxt, state,
	 * 			-
	 *       transition to TCP_ST_FIN_WAIT_2.
	 *   - If no ACK bit set: return immediately.
	 */
	if (TCP_SEQ_LT(seq, cur_flow->rcv_nxt))
	{
		RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
		return;
	}

	if (!tcph->ack)
		return;

	if (cur_flow->sndvar->sndbuf)
	{
		HandleACK(etcp, cur_flow, cur_ts,
				  tcph, seq, ack_seq, window, payloadlen);
	}

	if (cur_flow->sndvar->is_fin_sent &&
		ack_seq == cur_flow->sndvar->fss + 1)
	{
		cur_flow->sndvar->snd_una = ack_seq;
		cur_flow->snd_nxt = ack_seq;
		cur_flow->state = TCP_ST_FIN_WAIT_2;
		RemoveFromRTOList(etcp, cur_flow);
	}





	/**************************************************************************** */
	if (payloadlen > 0)
	{
		if (HandlePayload(etcp, cur_flow,
						  cur_ts, payload, seq, payloadlen))
		{
			/* if return is TRUE, send ACK */
			RegisterACK(etcp, cur_flow, cur_ts, ACK_OPT_AGGREGATE);
		}
		else
		{
			RegisterACK(etcp, cur_flow, cur_ts, ACK_OPT_NOW);
		}
	}
	if (tcph->fin)
	{
		if (seq + payloadlen == cur_flow->rcv_nxt)
		{
			cur_flow->rcv_nxt++;
			if (cur_flow->state == TCP_ST_FIN_WAIT_1)
			{
				cur_flow->state = TCP_ST_CLOSING;
			}

			else if (cur_flow->state == TCP_ST_FIN_WAIT_2)
			{
				cur_flow->state = TCP_ST_TIME_WAIT;
				RegisterToTimewaitList(etcp, cur_flow, cur_ts);
			}
			RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
		}
	}
}
/*----------------------------------------------------------------------------*/
static inline void
FIN_WAIT_2_Handler(flow_manager_t etcp, uint32_t cur_ts,
				   tcp_flow *cur_flow, struct tcphdr *tcph, uint32_t seq, uint32_t ack_seq,
				   uint8_t *payload, int payloadlen, uint16_t window)
{
	if (tcph->ack)
	{
		if (cur_flow->sndvar->sndbuf)
		{
			HandleACK(etcp, cur_flow, cur_ts,
					  tcph, seq, ack_seq, window, payloadlen);
		}
	}
	else
	{
		fprintf(stderr, "flow %d: does not contain an ack!\n",
				cur_flow->id);
		return;
	}

	if (payloadlen > 0)
	{
		if (HandlePayload(etcp, cur_flow,
						  cur_ts, payload, seq, payloadlen))
		{
			/* if return is TRUE, send ACK */
			RegisterACK(etcp, cur_flow, cur_ts, ACK_OPT_AGGREGATE);
		}
		else
		{
			RegisterACK(etcp, cur_flow, cur_ts, ACK_OPT_NOW);
		}
	}

	/**************************************************************************** */
	// TODO section
	/* PART_1: Connection Management - FIN_WAIT_2 state */
	/*
	 * Our FIN has been ACKed; now waiting for the peer's FIN.
	 *
	 *
	 * FIN received (in-order, seq + payloadlen == rcv_nxt):
	 *   - Transition to TCP_ST_TIME_WAIT.
	 *   - Advance rcv_nxt.
	 *   - Start the TIME_WAIT timer and schedule a final ACK.
	 */
	if (tcph->fin && seq + payloadlen == cur_flow->rcv_nxt)
	{
		cur_flow->rcv_nxt++;
		cur_flow->state = TCP_ST_TIME_WAIT;
		RegisterToTimewaitList(etcp, cur_flow, cur_ts);
		RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
	}


	/**************************************************************************** */
}
/*----------------------------------------------------------------------------*/
static inline void
CLOSING_Handler(flow_manager_t etcp, uint32_t cur_ts,
				tcp_flow *cur_flow, struct tcphdr *tcph, uint32_t seq, uint32_t ack_seq,
				int payloadlen, uint16_t window)
{

	if (tcph->ack)
	{
		if (cur_flow->sndvar->sndbuf)
		{
			HandleACK(etcp, cur_flow, cur_ts,
					  tcph, seq, ack_seq, window, payloadlen);
		}

		if (!cur_flow->sndvar->is_fin_sent)
		{
			fprintf(stderr, "flow %d (TCP_ST_CLOSING): "
							"No FIN sent yet.\n",
					cur_flow->id);
			return;
		}

		// check if ACK of FIN
		if (ack_seq != cur_flow->sndvar->fss + 1)
		{

			/* if the packet is not the ACK of FIN, ignore */
			return;
		}

		cur_flow->sndvar->snd_una = ack_seq;
		cur_flow->snd_nxt = ack_seq;
		RenewRetransmissionTimer(etcp, cur_flow, cur_ts);

		cur_flow->state = TCP_ST_TIME_WAIT;

		RegisterToTimewaitList(etcp, cur_flow, cur_ts);
	}
	else
	{
		fprintf(stderr, "flow %d (TCP_ST_CLOSING): Not ACK\n",
				cur_flow->id);
		return;
	}
}

/*----------------------------------------------------------------------------*/
int ProcessTCPPacket(flow_manager_t etcp,
					 uint32_t cur_ts, const struct iphdr *iph, int ip_len)
{
	struct tcphdr *tcph = (struct tcphdr *)((u_char *)iph + (iph->ihl << 2));
	uint8_t *payload = (uint8_t *)tcph + (tcph->doff << 2);
	int payloadlen = ip_len - (payload - (u_char *)iph);
	tcp_flow s_flow;
	tcp_flow *cur_flow = NULL;
	uint32_t seq = ntohl(tcph->seq);
	uint32_t ack_seq = ntohl(tcph->ack_seq);
	uint16_t window = ntohs(tcph->window);

	/* Check ip packet invalidation */
	if (ip_len < ((iph->ihl + tcph->doff) << 2))
		return ERROR;

	// fill in 4-tuple(src_addr,dst_addr,src_port,dst_port) for the flow
	s_flow.saddr = iph->daddr;
	s_flow.sport = tcph->dest;
	s_flow.daddr = iph->saddr;
	s_flow.dport = tcph->source;

	// find if corresponding flow exists in tcp_flow hash table
	if (!(cur_flow = SearchFlowHT(etcp->tcp_flow_table, &s_flow)))
	{
		cur_flow = CreateHashTableEntry(etcp, cur_ts, iph, ip_len, tcph,
										seq, ack_seq, payloadlen, window);
		if (!cur_flow)
			return TRUE;
	}

	/* Validate sequence. if not valid, ignore the packet */
	if (cur_flow->state > TCP_ST_SYN_RCVD)
	{
		int ret = CheckSequenceValid(etcp, cur_flow,
									 cur_ts, tcph, seq, ack_seq, payloadlen);
		if (!ret)
		{
			return TRUE;
		}
	}

	/* Update receive window size */
	if (tcph->syn)
	{
		cur_flow->sndvar->peer_wnd = window;
	}
	cur_flow->last_active_ts = cur_ts;
	RenewTimeoutList(etcp, cur_flow);

	/* Process RST: process here only if state > TCP_ST_SYN_SENT */
	if (tcph->rst)
	{
		cur_flow->have_reset = TRUE;
		if (cur_flow->state > TCP_ST_SYN_SENT)
		{
			if (HandleRST(etcp, cur_flow, ack_seq))
			{
				return TRUE;
			}
		}
	}

	switch (cur_flow->state)
	{
	// based on current state , process packets
	case TCP_ST_LISTEN:
		LISTEN_Handler(etcp, cur_ts, cur_flow, tcph);
		break;

	case TCP_ST_SYN_SENT:
		SYN_SENT_Handler(etcp, cur_ts, cur_flow, iph, tcph,
						 seq, ack_seq, payloadlen, window);
		break;

	case TCP_ST_SYN_RCVD:
		if (tcph->syn && seq == cur_flow->rcvvar->irs) // SYN retransmitted
		{
			LISTEN_Handler(etcp, cur_ts, cur_flow, tcph);
		}
		else
		{
			SYN_RCVD_Handler(etcp, cur_ts, cur_flow, tcph, ack_seq);
			if (payloadlen > 0 && cur_flow->state == TCP_ST_ESTABLISHED)
			{
				ESTABLISHED_Handler(etcp, cur_ts, cur_flow, tcph,
									seq, ack_seq, payload,
									payloadlen, window);
			}
		}
		break;

	case TCP_ST_ESTABLISHED:
		ESTABLISHED_Handler(etcp, cur_ts, cur_flow, tcph,
							seq, ack_seq, payload, payloadlen, window);
		break;

	case TCP_ST_CLOSE_WAIT:
		CLOSE_WAIT_Handler(etcp, cur_ts, cur_flow, tcph, seq, ack_seq,
						   payloadlen, window);
		break;

	case TCP_ST_LAST_ACK:
		LAST_ACK_Handler(etcp, cur_ts, iph, ip_len, cur_flow, tcph,
						 seq, ack_seq, payloadlen, window);
		break;

	case TCP_ST_FIN_WAIT_1:
		FIN_WAIT_1_Handler(etcp, cur_ts, cur_flow, tcph, seq, ack_seq,
						   payload, payloadlen, window);
		break;

	case TCP_ST_FIN_WAIT_2:
		FIN_WAIT_2_Handler(etcp, cur_ts, cur_flow, tcph, seq, ack_seq,
						   payload, payloadlen, window);
		break;

	case TCP_ST_CLOSING:
		CLOSING_Handler(etcp, cur_ts, cur_flow, tcph, seq, ack_seq,
						payloadlen, window);
		break;

	case TCP_ST_TIME_WAIT:
		/* the only thing that can arrive in this state is a retransmission
		   of the remote FIN. Acknowledge it, and restart the 2 MSL timeout */
		if (cur_flow->on_timewait_list)
		{
			RemoveFromTimewaitList(etcp, cur_flow);
			RegisterToTimewaitList(etcp, cur_flow, cur_ts);
		}
		RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
		break;

	case TCP_ST_CLOSED:
		break;
	}

	return TRUE;
}
