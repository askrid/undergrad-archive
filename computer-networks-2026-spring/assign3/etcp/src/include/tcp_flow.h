#ifndef tcp_flow_H
#define tcp_flow_H

#include <netinet/ip.h>
#include <linux/tcp.h>
#include <sys/queue.h>
#include <stdint.h>
#include "etcp.h"

struct recv_var_tcp
{
    uint32_t rcv_wnd;        /* receive window (unscaled) */
    uint32_t irs;            /* initial receiving sequence */
    uint32_t snd_wl1;        /* segment seq number for last window update */
    uint32_t snd_wl2;        /* segment ack number for last window update */

    /* variables for fast retransmission */
    uint8_t dup_acks;        /* number of duplicated acks */
    uint32_t last_ack_seq;   /* highest ackd seq */

    /* timestamps */
    uint32_t ts_tw_expire;       /* timestamp for timewait expire */

    /* RTT estimation variables */
    uint32_t srtt;           /* smoothed round trip time << 3 (scaled) */
    uint32_t mdev;           /* medium deviation */
    uint32_t mdev_max;       /* maximal mdev for the last rtt period */
    uint32_t rttvar;         /* smoothed mdev_max */
    uint32_t rtt_seq;        /* sequence number to update rttvar */

    // socket receive buffer
    struct tcp_receive_buffer *rcvbuf;

    pthread_mutex_t read_lock;


    TAILQ_ENTRY(tcp_flow) he_link;   /* hash table entry link */
};

/* ============================================================
 * send_var_tcp
 * ============================================================ */
struct send_var_tcp
{
    uint16_t mss;            /* maximum segment size */

    int8_t nif_out;          /* cached output network interface */
    unsigned char *d_haddr;  /* cached destination MAC address */

    /* send sequence variables */
    uint32_t snd_una;        /* send unacknowledged */
    uint32_t snd_wnd;        /* send window (unscaled) */
    uint32_t peer_wnd;       /* peer window size */
    uint32_t iss;            /* initial sending sequence */
    uint32_t fss;            /* final sending sequence */

    /* retransmission timeout variables */
    uint8_t nrtx;            /* number of retransmission */
    uint32_t rto;            /* retransmission timeout */
    uint32_t ts_rto;         /* timestamp for retransmission timeout */

    /* congestion control variables */
    uint32_t cwnd;           /* congestion window */
    uint32_t ssthresh;       /* slow start threshold */
    uint8_t in_fast_recovery; 
    
    /* timestamp */
    uint32_t ts_lastack_sent; /* last ack sent time */

    uint8_t is_wack:1,
            ack_cnt:6;
    
    struct tcp_send_buffer *sndbuf;

    uint8_t on_control_pkt_list;
    uint8_t on_data_pkt_list;
    uint8_t on_ack_pkt_list;
    uint8_t on_sendq;
    uint8_t on_ackq;
    uint8_t on_closeq;
    uint8_t on_resetq;

    uint8_t on_closeq_int:1,
            on_resetq_int:1,
            is_fin_sent:1;
            
    TAILQ_ENTRY(tcp_flow) control_link;
    TAILQ_ENTRY(tcp_flow) send_link;
    TAILQ_ENTRY(tcp_flow) ack_link;

    TAILQ_ENTRY(tcp_flow) timer_link;
    TAILQ_ENTRY(tcp_flow) timeout_link;
    
    pthread_mutex_t write_lock;
};

typedef struct tcp_flow
{
    socket_map_t socket;

    uint32_t id:24;

    /* 4-tuple */
    uint32_t saddr;          /* network order */
    uint32_t daddr;          /* network order */
    uint16_t sport;          /* network order */
    uint16_t dport;          /* network order */

    /* FSM */
    uint8_t state;


    uint8_t close_reason;
    uint8_t on_hash_table;
    uint8_t on_timewait_list; 
    uint8_t ht_idx; // hash table index for tcp_flow hash table
    uint8_t closed;
    uint8_t is_bound_addr;
    uint8_t need_wnd_adv; // need window advertisement from peer
    int16_t on_rto_idx; // on RTO list?

    uint16_t on_timeout_list:1,
             control_pkt_list_waiting:1,
             have_reset:1,
             is_external:1;
             
    /* seq cursors */
    uint32_t snd_nxt;        /* send next */
    uint32_t rcv_nxt;        /* receive next */

    struct recv_var_tcp *rcvvar;
    struct send_var_tcp *sndvar;
    uint32_t last_active_ts;
} tcp_flow;


extern char *
GetStateAsString(const tcp_flow *cur_flow);

unsigned int
CalcHash(const void *flow);

int
IsFlowEqual(const void *flow1, const void *flow2);

extern  int
AddEventToEpollQ(struct etcp_epoll *ep,
              int queue_type, socket_map_t socket, uint32_t event);

extern  void
RegisterReadEvent(flow_manager_t etcp, tcp_flow *flow);

extern  void
RegisterWriteEvent(flow_manager_t etcp, tcp_flow *flow);

extern  void
RegisterCloseEvent(flow_manager_t etcp, tcp_flow *flow);

extern  void
RegisterErrorEvent(flow_manager_t etcp, tcp_flow *flow);

/* flow lifecycle */
tcp_flow *
CreateFlow(flow_manager_t etcp, socket_map_t socket, int is_passive_open,
                uint32_t saddr, uint16_t sport,
                uint32_t daddr, uint16_t dport);

void
DestroyFlow(flow_manager_t etcp, tcp_flow *flow);


extern  void
SetInitialSendingSequence();

#endif /* tcp_flow_H */
