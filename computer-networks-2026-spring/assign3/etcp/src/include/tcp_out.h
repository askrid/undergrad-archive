#ifndef TCP_OUT_H
#define TCP_OUT_H

#include "etcp.h"
#include "tcp_flow.h"

enum ack_opt
{
        ACK_OPT_NOW,
        ACK_OPT_AGGREGATE,
        ACK_OPT_WACK
};

int MakeTCPPacketWotFlow(struct flow_manager *etcp,
                            uint32_t saddr, uint16_t sport,
                            uint32_t daddr, uint16_t dport,
                            uint32_t seq, uint32_t ack_seq,
                            uint8_t flags, uint32_t cur_ts);

int MakeTCPPacket(struct flow_manager *etcp, tcp_flow *cur_flow,
                  uint32_t cur_ts, uint8_t flags,
                  uint8_t *payload, uint16_t payloadlen);

extern int
FlushControlPktList(flow_manager_t etcp,
                    struct pkt_sender *sender,
                    uint32_t cur_ts, int thresh);

extern int
FlushDataPktList(flow_manager_t etcp,
                 struct pkt_sender *sender,
                 uint32_t cur_ts, int thresh);

extern int
FlushAckPktList(flow_manager_t etcp,
                struct pkt_sender *sender,
                uint32_t cur_ts, int thresh);

extern void
RegisterflowToControlPktList(flow_manager_t etcp,
                 tcp_flow *cur_flow, uint32_t cur_ts);

extern void
RegisterflowToSendPktList(flow_manager_t etcp,
              tcp_flow *cur_flow);

extern void
RemoveFromControlPktList(flow_manager_t etcp,
                      tcp_flow *cur_flow);

extern void
RemoveFromDataPktList(flow_manager_t etcp,
                   tcp_flow *cur_flow);

extern void
RemoveFromAckPktList(flow_manager_t etcp,
                  tcp_flow *cur_flow);

extern void
RegisterACK(flow_manager_t etcp,
           tcp_flow *cur_flow,
           uint32_t cur_ts, uint8_t opt);

#endif /* TCP_OUT_H */
