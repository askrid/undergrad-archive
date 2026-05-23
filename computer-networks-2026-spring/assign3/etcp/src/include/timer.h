#ifndef TIMER_H
#define TIMER_H

#include "etcp.h"
#include "tcp_flow.h"

#define RTO_HASH 3000

/* ============================================================
 * Internal timer data structures 
 * ============================================================ */


struct rto_hashstore
{
    uint32_t rto_now_idx;   /* pointing the hs_table_s index */
    uint32_t rto_now_ts;

    TAILQ_HEAD(rto_head, tcp_flow) rto_list[RTO_HASH + 1];
};

struct rto_hashstore *
InitRTOStructure();

/* ============================================================
 * Timer list management APIs
 * ============================================================ */

extern  void
RegisterToRTOList(flow_manager_t etcp, tcp_flow *cur_flow);

extern  void
RemoveFromRTOList(flow_manager_t etcp, tcp_flow *cur_flow);

extern  void
RegisterToTimewaitList(flow_manager_t etcp,
                  tcp_flow *cur_flow, uint32_t cur_ts);

extern  void
RemoveFromTimewaitList(flow_manager_t etcp, tcp_flow *cur_flow);

extern  void
RegisterToTimeoutList(flow_manager_t etcp, tcp_flow *cur_flow);

extern  void
RemoveFromTimeoutList(flow_manager_t etcp, tcp_flow *cur_flow);

extern  void
RenewTimeoutList(flow_manager_t etcp, tcp_flow *cur_flow);

extern  void
RenewRetransmissionTimer(flow_manager_t etcp,
                          tcp_flow *cur_flow, uint32_t cur_ts);

/* ============================================================
 * Timer event processing(NOT RELEVANT TO TRANSPORT LAYER)
 * ============================================================ */
void
CheckRetransmissionTimeout(flow_manager_t etcp, uint32_t cur_ts, int thresh);

void
CheckTimewaitExpire(flow_manager_t etcp, uint32_t cur_ts, int thresh);

void
CheckConnectionTimeout(flow_manager_t etcp, uint32_t cur_ts, int thresh);

#endif /* TIMER_H */
