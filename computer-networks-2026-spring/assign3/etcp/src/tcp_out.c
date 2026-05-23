#include <unistd.h>
#include <assert.h>
#include "tcp_out.h"
#include "util.h"
#include "etcp.h"
#include "ip_out.h"
#include "tcp_in.h"
#include "tcp_flow.h"
#include "eventpoll.h"
#include "timer.h"
#include "cc_trace.h"
#define TCP_MAX_WINDOW 65535

/*----------------------------------------------------------------------------*/
static inline uint16_t
CalcOptLen(uint8_t flags)
{
	uint16_t optlen = 0;

	if (flags & TCP_FLAG_SYN)
	{
		optlen += TCP_OPT_MSS_LEN;
	}
	return optlen;
}
/*----------------------------------------------------------------------------*/
static inline void
GenTCPOpt(tcp_flow *cur_flow, uint32_t cur_ts,
		  uint8_t flags, uint8_t *tcpopt, uint16_t optlen)
{
	int i = 0;

	if (flags & TCP_FLAG_SYN)
	{
		uint16_t mss;

		/* MSS option */
		mss = cur_flow->sndvar->mss;
		tcpopt[i++] = TCP_OPT_MSS;
		tcpopt[i++] = TCP_OPT_MSS_LEN;
		tcpopt[i++] = mss >> 8;
		tcpopt[i++] = mss % 256;
	}
	assert(i == optlen);
}

/*----------------------------------------------------------------------------*/
int MakeTCPPacketWotFlow(struct flow_manager *etcp,
						 uint32_t saddr, uint16_t sport, uint32_t daddr, uint16_t dport,
						 uint32_t seq, uint32_t ack_seq, uint8_t flags,
						 uint32_t cur_ts)
{
	struct tcphdr *tcph;
	uint16_t optlen;

	optlen = CalcOptLen(flags);
	tcph = (struct tcphdr *)MakeIPPacketWoutFlow(etcp, IPPROTO_TCP, 0,
												 saddr, daddr, TCP_HEADER_LEN + optlen);
	if (tcph == NULL)
	{
		return ERROR;
	}

	memset(tcph, 0, TCP_HEADER_LEN + optlen);
	tcph->source = sport;
	tcph->dest = dport;
	tcph->doff = (TCP_HEADER_LEN + optlen) >> 2;

	tcph->seq = htonl(seq);
	if (flags & TCP_FLAG_RST)
		tcph->rst = TRUE;

	if (flags & TCP_FLAG_ACK)
	{
		tcph->ack = TRUE;
		tcph->ack_seq = htonl(ack_seq);
	}
	tcph->window = 0;
	tcph->check = htons(TCPCalcChecksum((uint16_t *)tcph,
										TCP_HEADER_LEN + optlen,
										saddr, daddr));

	return 0;
}

/*----------------------------------------------------------------------------*/
int MakeTCPPacket(struct flow_manager *etcp, tcp_flow *cur_flow,
				  uint32_t cur_ts, uint8_t flags, uint8_t *payload, uint16_t payloadlen)
{
	struct tcphdr *tcph;
	uint16_t optlen;

	optlen = CalcOptLen(flags);
	if (payloadlen + optlen > cur_flow->sndvar->mss)
	{
		fprintf(stderr, "Payload size exceeds MSS\n");
		return ERROR;
	}

	tcph = (struct tcphdr *)MakeIPPacket(etcp, cur_flow,
										 TCP_HEADER_LEN + optlen + payloadlen);

	if (tcph == NULL)
	{
		return -2;
	}
	memset(tcph, 0, TCP_HEADER_LEN + optlen);

	tcph->source = cur_flow->sport;
	tcph->dest = cur_flow->dport;

	if (flags & TCP_FLAG_WACK)
	{
		tcph->seq = htonl(cur_flow->snd_nxt - 1);
	}

	else if (flags & TCP_FLAG_FIN)
	{
		tcph->fin = TRUE;
		tcph->seq = htonl(cur_flow->sndvar->fss);
		cur_flow->sndvar->is_fin_sent = TRUE;
	}
	else
	{
		tcph->seq = htonl(cur_flow->snd_nxt);
	}
	if (flags & TCP_FLAG_PSH)
	{
		tcph->psh = TRUE;
	}
	if (flags & TCP_FLAG_RST)
	{
		tcph->rst = TRUE;
	}

	// advertising_wnd is the amount of available receive buffer space advertised to the peer.
	uint32_t advertising_wnd;
	/**************************************************************************** */
	// TODO section
	/* PART_1: Connection Management - Make Control Packet */
	/*
	 * If packet to send is SYN packet:
	 *  	turn on the SYN bit
	 * If packet to send is ACK packet:
	 * 		turn on the ACK bit
	 * 		fill in ack numebr of outgoing packet
	 * 		update sndvar's lastack sent moment and flow's last active moment
	 *
	 * caluclate advertising_wnd and fill in window filed of outgoing packet
	 * calculate tcph->doff
	 * update flow's snd_nxt
	 * PLEASE note that SYN and FIN consume an additional one sequence number:
	 * 	 	You must advance snd_nxt
	 * 		Arm the retransmission timer
	 */

	if (flags & TCP_FLAG_SYN)
		tcph->syn = TRUE;

	if (flags & TCP_FLAG_ACK)
	{
		tcph->ack = TRUE;
		tcph->ack_seq = htonl(cur_flow->rcv_nxt);
		cur_flow->sndvar->ts_lastack_sent = cur_ts;
		cur_flow->last_active_ts = cur_ts;
	}

	advertising_wnd = MIN(cur_flow->rcvvar->rcv_wnd, TCP_MAX_WINDOW);
	tcph->window = htons((uint16_t)advertising_wnd);
	tcph->doff = (TCP_HEADER_LEN + optlen) >> 2;

	if ((flags & TCP_FLAG_SYN) || (flags & TCP_FLAG_FIN))
	{
		cur_flow->snd_nxt++;
		cur_flow->sndvar->ts_rto = cur_ts + cur_flow->sndvar->rto;
		RegisterToRTOList(etcp, cur_flow);
	}

	/**************************************************************************** */

	if (payloadlen > 0)
	{
		/**************************************************************************** */
		// TODO section
		/* PART_2: Reliable Data Transfer - Payload Attachment */
		/*
		 * If there is payload to send:
		 *   - Copy it into the packet buffer immediately after the TCP header + options.
		 *   - Arm the retransmission timer: set ts_rto and call RegisterToRTOList().
		 *
		 * Hint: the payload region starts at (uint8_t *)tcph + TCP_HEADER_LEN + optlen.
		 *       ts_rto should be set to cur_ts + sndvar->rto.
		 */

		memcpy((uint8_t *)tcph + TCP_HEADER_LEN + optlen, payload, payloadlen);
		cur_flow->snd_nxt += payloadlen;
		cur_flow->sndvar->ts_rto = cur_ts + cur_flow->sndvar->rto;
		RegisterToRTOList(etcp, cur_flow);
		/**************************************************************************** */

	}

	/* if the advertised window is 0, we need to advertise again later */
	if (advertising_wnd == 0)
	{
		cur_flow->need_wnd_adv = TRUE;
	}
	// Add TCP options if needed
	GenTCPOpt(cur_flow, cur_ts, flags,
			  (uint8_t *)tcph + TCP_HEADER_LEN, optlen);

	// compute TCP checkusm
	tcph->check = htons(TCPCalcChecksum((uint16_t *)tcph,
										TCP_HEADER_LEN + optlen + payloadlen,
										cur_flow->saddr, cur_flow->daddr));

	if (tcph->syn || tcph->fin)
	{
		return payloadlen + 1;
	}
	return payloadlen;
}
/*----------------------------------------------------------------------------*/
static int
ProcessSendBuffer(flow_manager_t etcp, tcp_flow *cur_flow, uint32_t cur_ts)
{
	struct send_var_tcp *sndvar = cur_flow->sndvar;
	uint8_t *data;		   // payload pointer into the send buffer
	uint32_t pkt_len;	   // bytes of payload to put in one packet
	uint32_t len;		   // unsent buffered data starting at snd_nxt
	uint32_t seq = 0;	   // current sequence number to transmit
	int sndlen;			   // actual bytes accepted by MakeTCPPacket()
	int packets = 0;	   // total number of packets sent in this loop
	uint8_t wack_sent = 0; // flag for sending window update ACK (ACK_OPT_WACK) when peer's window is full

	if (!sndvar->sndbuf)
	{
		return 0;
	}

	pthread_mutex_lock(&sndvar->write_lock);

	if (sndvar->sndbuf->len == 0)
	{
		packets = 0;
		goto out;
	}

	while (1)
	{
		/**************************************************************************** */
		// TODO section
		/* PART_2: Reliable Data Transfer - Send Buffer Traversal */
		/*
		 * Identify the next chunk of data to transmit.
		 * Use snd_nxt as the sequence number of the first unsent byte,
		 * then derive a pointer (data) and byte count (len) from sndbuf.
		 *
		 * Break the loop early if any sanity condition fails:
		 *   - seq is behind sndbuf->head_seq  (already acknowledged region)
		 *   - seq is behind snd_una           (should not happen, but guard it)
		 *   - the offset exceeds sndbuf->len  (no buffered data at this position)
		 *   - len == 0                         (nothing left to send)
		 *
		 * Variables to set: seq, data, len declared above.
		 */

		seq = cur_flow->snd_nxt;
		if (TCP_SEQ_LT(seq, sndvar->sndbuf->head_seq))
			break;
		if (TCP_SEQ_LT(seq, sndvar->snd_una))
			break;

		uint32_t off = seq - sndvar->sndbuf->head_seq;
		if (off >= sndvar->sndbuf->len)
			break;

		data = sndvar->sndbuf->head + off;
		len = sndvar->sndbuf->len - off;
		if (len == 0)
			break;
		/**************************************************************************** */

		int remaining_window; // how many more bytes may be sent right now
		/**************************************************************************** */
		// TODO section
		/* PART_2: Flow Control + Congestion Control - Send Window */
		/*
		 * Compute remaining_window declared above
		 * : how many more bytes the sender may transmit right now, taking into account both flow control and congestion control.
		 *
		 * Subtract in-flight bytes (already sent but not yet ACKed)
		 * to get the remaining allowance.
		 *
		 * NOTE: only one line calculating proper remaing_window will be enough in this section
		 */

		remaining_window = (int)MIN(sndvar->cwnd, sndvar->peer_wnd) - (int)(seq - sndvar->snd_una);
		/**************************************************************************** */

		/* if there is no space in the send window */
		if (remaining_window <= 0 ||
			(remaining_window < sndvar->mss && seq - sndvar->snd_una > 0 && len > remaining_window))
		{
			/* if peer window is full, send ACK and let its peer advertises new one */
			if (sndvar->peer_wnd <= sndvar->cwnd)
			{

				if (!wack_sent && TS_TO_MSEC(cur_ts - sndvar->ts_lastack_sent) > 500)
					RegisterACK(etcp, cur_flow, cur_ts, ACK_OPT_WACK);
				else
					wack_sent = 1;
			}
			packets = -3;
			goto out;
		}

		/* payload size limited by remaining window space */
		len = MIN(len, remaining_window);
		/* payload size limited by TCP MSS */
		pkt_len = MIN(len, sndvar->mss - CalcOptLen(TCP_FLAG_ACK));

		if ((sndlen = MakeTCPPacket(etcp, cur_flow, cur_ts,
									TCP_FLAG_ACK, data, pkt_len)) < 0)
		{
			/* there is no available tx buf */
			packets = -3;
			goto out;
		}
		packets++;
	}
out:
	pthread_mutex_unlock(&sndvar->write_lock);
	return packets;
}

/*----------------------------------------------------------------------------*/
static inline int
MakeControlPacket(flow_manager_t etcp, tcp_flow *cur_flow, uint32_t cur_ts)
{
	struct send_var_tcp *sndvar = cur_flow->sndvar;
	int ret = 0;
	if (cur_flow->state == TCP_ST_CLOSING)
	{
		if (sndvar->is_fin_sent)
		{
			/* if the sequence is for FIN, send FIN */
			if (cur_flow->snd_nxt == sndvar->fss)
			{
				ret = MakeTCPPacket(etcp, cur_flow, cur_ts,
									TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0);
			}
			else
			{
				ret = MakeTCPPacket(etcp, cur_flow, cur_ts,
									TCP_FLAG_ACK, NULL, 0);
			}
		}
		else
		{
			/* if FIN is not sent, send fin with ack */
			ret = MakeTCPPacket(etcp, cur_flow, cur_ts,
								TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0);
		}
	}
	else if (cur_flow->state == TCP_ST_TIME_WAIT)
	{
		/* Send ACK here */
		ret = MakeTCPPacket(etcp, cur_flow, cur_ts, TCP_FLAG_ACK, NULL, 0);
	}
	else if (cur_flow->state == TCP_ST_CLOSED)
	{
		/* first flush the data and ack */
		if (sndvar->on_data_pkt_list || sndvar->on_ack_pkt_list)
		{
			ret = -1;
		}
		else
		{
			ret = MakeTCPPacket(etcp, cur_flow, cur_ts, TCP_FLAG_RST, NULL, 0);
			if (ret >= 0)
			{
				DestroyFlow(etcp, cur_flow);
			}
		}
	}
	/**************************************************************************** */
	// TODO section
	/* PART_1: Connection Management - Control Packet State Machine */
	/*
	 * Send the appropriate control packet for the current TCP state.
	 * Call MakeTCPPacket() with the correct flag combination per state:
	 *
	 *   SYN_SENT   → SYN
	 *   SYN_RCVD   → SYN|ACK  (set snd_nxt before making the packet)
	 *   ESTABLISHED, CLOSE_WAIT, FIN_WAIT_2, TIME_WAIT → ACK
	 *   LAST_ACK, FIN_WAIT_1  → FIN|ACK
	 *     (but return -1 to defer if on_data_pkt_list or on_ack_pkt_list is set,
	 *      so pending data/ACKs are flushed first)
	 *
	 * Return value from MakeTCPPacket() propagates to the caller.
	 * Return -1 to signal "try again later"
	 */

	else if (cur_flow->state == TCP_ST_SYN_SENT)
	{
		ret = MakeTCPPacket(etcp, cur_flow, cur_ts, TCP_FLAG_SYN, NULL, 0);
	}
	else if (cur_flow->state == TCP_ST_SYN_RCVD)
	{
		cur_flow->snd_nxt = sndvar->iss;
		ret = MakeTCPPacket(etcp, cur_flow, cur_ts,
							TCP_FLAG_SYN | TCP_FLAG_ACK, NULL, 0);
	}
	else if (cur_flow->state == TCP_ST_ESTABLISHED ||
			 cur_flow->state == TCP_ST_CLOSE_WAIT ||
			 cur_flow->state == TCP_ST_FIN_WAIT_2)
	{
		ret = MakeTCPPacket(etcp, cur_flow, cur_ts, TCP_FLAG_ACK, NULL, 0);
	}
	else if (cur_flow->state == TCP_ST_LAST_ACK ||
			 cur_flow->state == TCP_ST_FIN_WAIT_1)
	{
		if (sndvar->on_data_pkt_list || sndvar->on_ack_pkt_list)
			ret = -1;
		else
			ret = MakeTCPPacket(etcp, cur_flow, cur_ts,
								TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0);
	}
	/**************************************************************************** */
	return ret;
}
/*----------------------------------------------------------------------------*/
inline int
FlushControlPktList(flow_manager_t etcp,
					struct pkt_sender *sender, uint32_t cur_ts, int thresh)
{
	tcp_flow *cur_flow;
	tcp_flow *next, *last;
	int cnt = 0;
	int ret;

	thresh = MIN(thresh, sender->control_pkt_list_cnt);

	/* Send TCP control messages */
	cnt = 0;
	cur_flow = TAILQ_FIRST(&sender->control_pkt_list);
	last = TAILQ_LAST(&sender->control_pkt_list, control_head);
	while (cur_flow)
	{
		if (++cnt > thresh)
			break;
		next = TAILQ_NEXT(cur_flow, sndvar->control_link);

		TAILQ_REMOVE(&sender->control_pkt_list, cur_flow, sndvar->control_link);
		sender->control_pkt_list_cnt--;

		if (cur_flow->sndvar->on_control_pkt_list)
		{
			cur_flow->sndvar->on_control_pkt_list = FALSE;
			ret = MakeControlPacket(etcp, cur_flow, cur_ts);
			if (ret == -2)
			{
				TAILQ_INSERT_HEAD(&sender->control_pkt_list,
								  cur_flow, sndvar->control_link);
				cur_flow->sndvar->on_control_pkt_list = TRUE;
				sender->control_pkt_list_cnt++;
				/* since there is no available write buffer, break */
				break;
			}
			else if (ret < 0)
			{
				/* try again after handling other flows */
				TAILQ_INSERT_TAIL(&sender->control_pkt_list,
								  cur_flow, sndvar->control_link);
				cur_flow->sndvar->on_control_pkt_list = TRUE;
				sender->control_pkt_list_cnt++;
			}
		}
		else
		{
			fprintf(stderr, "flow %d: not on control list.\n", cur_flow->id);
		}

		if (cur_flow == last)
			break;
		cur_flow = next;
	}

	return cnt;
}
/*----------------------------------------------------------------------------*/
inline int
FlushDataPktList(flow_manager_t etcp,
				 struct pkt_sender *sender, uint32_t cur_ts, int thresh)
{
	tcp_flow *cur_flow;
	tcp_flow *next, *last;
	int cnt = 0;
	int ret;

	/* Send data */
	cnt = 0;
	cur_flow = TAILQ_FIRST(&sender->data_pkt_list);
	last = TAILQ_LAST(&sender->data_pkt_list, send_head);
	while (cur_flow)
	{
		if (++cnt > thresh)
			break;

		next = TAILQ_NEXT(cur_flow, sndvar->send_link);

		TAILQ_REMOVE(&sender->data_pkt_list, cur_flow, sndvar->send_link);
		if (cur_flow->sndvar->on_data_pkt_list)
		{
			ret = 0;
			/* Send data here */
			/* Only can send data when ESTABLISHED or CLOSE_WAIT */
			if (cur_flow->state == TCP_ST_ESTABLISHED)
			{
				if (cur_flow->sndvar->on_control_pkt_list)
				{
					/* delay sending data after until on_control_pkt_list becomes off */
					ret = -1;
				}
				else
				{
					ret = ProcessSendBuffer(etcp, cur_flow, cur_ts);
				}
			}
			else if (cur_flow->state == TCP_ST_CLOSE_WAIT ||
					 cur_flow->state == TCP_ST_FIN_WAIT_1 ||
					 cur_flow->state == TCP_ST_LAST_ACK)
			{
				ret = ProcessSendBuffer(etcp, cur_flow, cur_ts);
			}
			else
			{
				fprintf(stderr, "flow %d: on_data_pkt_list at state %s\n",
						cur_flow->id, GetStateAsString(cur_flow));
			}

			if (ret < 0)
			{
				TAILQ_INSERT_TAIL(&sender->data_pkt_list, cur_flow, sndvar->send_link);
				/* since there is no available write buffer, break */
				break;
			}
			else
			{
				cur_flow->sndvar->on_data_pkt_list = FALSE;
				sender->data_pkt_list_cnt--;
				/* the ret value is the number of packets sent. */
				/* decrease ack_cnt for the piggybacked acks */

				if (cur_flow->control_pkt_list_waiting)
				{
					if (!cur_flow->sndvar->on_ack_pkt_list)
					{
						cur_flow->control_pkt_list_waiting = FALSE;
						RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
					}
				}
			}
		}
		if (cur_flow == last)
			break;
		cur_flow = next;
	}

	return cnt;
}
/*----------------------------------------------------------------------------*/
inline int
FlushAckPktList(flow_manager_t etcp,
				struct pkt_sender *sender, uint32_t cur_ts, int thresh)
{
	tcp_flow *cur_flow;
	tcp_flow *next, *last;
	int to_ack;
	int cnt = 0;
	int ret;

	/* Send aggregated acks */
	cnt = 0;
	cur_flow = TAILQ_FIRST(&sender->ack_pkt_list);
	last = TAILQ_LAST(&sender->ack_pkt_list, ack_head);
	while (cur_flow)
	{
		if (++cnt > thresh)
			break;
		next = TAILQ_NEXT(cur_flow, sndvar->ack_link);

		if (cur_flow->sndvar->on_ack_pkt_list)
		{
			/* this list is only to ack the data packets */
			/* if the ack is not data ack, then it will not process here */
			to_ack = FALSE;
			if (cur_flow->state == TCP_ST_ESTABLISHED ||
				cur_flow->state == TCP_ST_CLOSE_WAIT ||
				cur_flow->state == TCP_ST_FIN_WAIT_1 ||
				cur_flow->state == TCP_ST_FIN_WAIT_2 ||
				cur_flow->state == TCP_ST_TIME_WAIT)
			{
				/* TIMEWAIT is possible since the ack is queued
				   at FIN_WAIT_2 */
				if (cur_flow->rcvvar->rcvbuf)
				{
					if (TCP_SEQ_LEQ(cur_flow->rcv_nxt,
									cur_flow->rcvvar->rcvbuf->head_seq +
										cur_flow->rcvvar->rcvbuf->merged_len))
					{
						to_ack = TRUE;
					}
				}
			}
			if (to_ack)
			{
				/* send the queued ack packets */
				while (cur_flow->sndvar->ack_cnt > 0)
				{
					ret = MakeTCPPacket(etcp, cur_flow,
										cur_ts, TCP_FLAG_ACK, NULL, 0);
					if (ret < 0)
					{
						/* since there is no available write buffer, break */
						break;
					}
					cur_flow->sndvar->ack_cnt--;
				}

				/* if is_wack is set, send packet to get window advertisement */
				if (cur_flow->sndvar->is_wack)
				{
					cur_flow->sndvar->is_wack = FALSE;
					ret = MakeTCPPacket(etcp, cur_flow,
										cur_ts, TCP_FLAG_ACK | TCP_FLAG_WACK, NULL, 0);
					if (ret < 0)
					{
						/* since there is no available write buffer, break */
						cur_flow->sndvar->is_wack = TRUE;
					}
				}

				if (!(cur_flow->sndvar->ack_cnt || cur_flow->sndvar->is_wack))
				{
					cur_flow->sndvar->on_ack_pkt_list = FALSE;
					TAILQ_REMOVE(&sender->ack_pkt_list, cur_flow, sndvar->ack_link);
					sender->ack_pkt_list_cnt--;
				}
			}
			else
			{
				cur_flow->sndvar->on_ack_pkt_list = FALSE;
				cur_flow->sndvar->ack_cnt = 0;
				cur_flow->sndvar->is_wack = 0;
				TAILQ_REMOVE(&sender->ack_pkt_list, cur_flow, sndvar->ack_link);
				sender->ack_pkt_list_cnt--;
			}

			if (cur_flow->control_pkt_list_waiting)
			{
				if (!cur_flow->sndvar->on_data_pkt_list)
				{
					cur_flow->control_pkt_list_waiting = FALSE;
					RegisterflowToControlPktList(etcp, cur_flow, cur_ts);
				}
			}
		}
		else
		{
			fprintf(stderr, "flow %d: not on ack list.\n", cur_flow->id);
			TAILQ_REMOVE(&sender->ack_pkt_list, cur_flow, sndvar->ack_link);
			sender->ack_pkt_list_cnt--;
		}

		if (cur_flow == last)
			break;
		cur_flow = next;
	}

	return cnt;
}
/*----------------------------------------------------------------------------*/
inline struct pkt_sender *
GetSender(flow_manager_t etcp, tcp_flow *cur_flow)
{
	if (cur_flow->sndvar->nif_out < 0)
	{
		return etcp->g_sender;
	}

	int eidx = CONFIG.nif_to_eidx[cur_flow->sndvar->nif_out];
	return etcp->n_sender[eidx];
}
/*----------------------------------------------------------------------------*/
inline void
RegisterflowToControlPktList(flow_manager_t etcp, tcp_flow *cur_flow, uint32_t cur_ts)
{
	if (!cur_flow->sndvar->on_control_pkt_list)
	{
		struct pkt_sender *sender = GetSender(etcp, cur_flow);
		assert(sender != NULL);

		cur_flow->sndvar->on_control_pkt_list = TRUE;
		TAILQ_INSERT_TAIL(&sender->control_pkt_list, cur_flow, sndvar->control_link);
		sender->control_pkt_list_cnt++;
	}
}
/*----------------------------------------------------------------------------*/
inline void
RegisterflowToSendPktList(flow_manager_t etcp, tcp_flow *cur_flow)
{
	struct pkt_sender *sender = GetSender(etcp, cur_flow);
	assert(sender != NULL);

	if (!cur_flow->sndvar->sndbuf)
	{
		fprintf(stderr, "[%d] flow %d: No send buffer available.\n",
				etcp->ctx->cpu,
				cur_flow->id);
		assert(0);
		return;
	}

	if (!cur_flow->sndvar->on_data_pkt_list)
	{
		cur_flow->sndvar->on_data_pkt_list = TRUE;
		TAILQ_INSERT_TAIL(&sender->data_pkt_list, cur_flow, sndvar->send_link);
		sender->data_pkt_list_cnt++;
	}
}
/*----------------------------------------------------------------------------*/
inline void
RegisterflowToAckPktList(flow_manager_t etcp, tcp_flow *cur_flow)
{
	struct pkt_sender *sender = GetSender(etcp, cur_flow);
	assert(sender != NULL);

	if (!cur_flow->sndvar->on_ack_pkt_list)
	{
		cur_flow->sndvar->on_ack_pkt_list = TRUE;
		TAILQ_INSERT_TAIL(&sender->ack_pkt_list, cur_flow, sndvar->ack_link);
		sender->ack_pkt_list_cnt++;
	}
}
/*----------------------------------------------------------------------------*/
inline void
RemoveFromControlPktList(flow_manager_t etcp, tcp_flow *cur_flow)
{
	struct pkt_sender *sender = GetSender(etcp, cur_flow);
	assert(sender != NULL);

	if (cur_flow->sndvar->on_control_pkt_list)
	{
		cur_flow->sndvar->on_control_pkt_list = FALSE;
		TAILQ_REMOVE(&sender->control_pkt_list, cur_flow, sndvar->control_link);
		sender->control_pkt_list_cnt--;
	}
}
/*----------------------------------------------------------------------------*/
inline void
RemoveFromDataPktList(flow_manager_t etcp, tcp_flow *cur_flow)
{
	struct pkt_sender *sender = GetSender(etcp, cur_flow);
	assert(sender != NULL);

	if (cur_flow->sndvar->on_data_pkt_list)
	{
		cur_flow->sndvar->on_data_pkt_list = FALSE;
		TAILQ_REMOVE(&sender->data_pkt_list, cur_flow, sndvar->send_link);
		sender->data_pkt_list_cnt--;
	}
}
/*----------------------------------------------------------------------------*/
inline void
RemoveFromAckPktList(flow_manager_t etcp, tcp_flow *cur_flow)
{
	struct pkt_sender *sender = GetSender(etcp, cur_flow);
	assert(sender != NULL);

	if (cur_flow->sndvar->on_ack_pkt_list)
	{
		cur_flow->sndvar->on_ack_pkt_list = FALSE;
		TAILQ_REMOVE(&sender->ack_pkt_list, cur_flow, sndvar->ack_link);
		sender->ack_pkt_list_cnt--;
	}
}
/*----------------------------------------------------------------------------*/
inline void
RegisterACK(flow_manager_t etcp,
			tcp_flow *cur_flow, uint32_t cur_ts, uint8_t opt)
{
	if (!(cur_flow->state == TCP_ST_ESTABLISHED ||
		  cur_flow->state == TCP_ST_CLOSE_WAIT ||
		  cur_flow->state == TCP_ST_FIN_WAIT_1 ||
		  cur_flow->state == TCP_ST_FIN_WAIT_2))
	{
		fprintf(stderr, "flow %u: Enqueueing ack at state %s\n",
				cur_flow->id, GetStateAsString(cur_flow));
	}

	if (opt == ACK_OPT_NOW)
	{
		if (cur_flow->sndvar->ack_cnt < cur_flow->sndvar->ack_cnt + 1)
		{
			cur_flow->sndvar->ack_cnt++;
		}
	}
	else if (opt == ACK_OPT_AGGREGATE)
	{
		if (cur_flow->sndvar->ack_cnt == 0)
		{
			cur_flow->sndvar->ack_cnt = 1;
		}
	}
	else if (opt == ACK_OPT_WACK)
	{
		cur_flow->sndvar->is_wack = TRUE;
	}
	RegisterflowToAckPktList(etcp, cur_flow);
}
