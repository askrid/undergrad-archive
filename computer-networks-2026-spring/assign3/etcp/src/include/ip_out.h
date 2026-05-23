#ifndef IP_OUT_H
#define IP_OUT_H

#include <stdint.h>
#include "tcp_flow.h"

extern  int 
FetchNetworkInterface(uint32_t daddr, uint8_t *is_external);

void
ForwardIPv4Packet(flow_manager_t etcp, int nif_in, char *buf, int len);

uint8_t *
MakeIPPacketWoutFlow(struct flow_manager *etcp, uint8_t protocol, 
		uint16_t ip_id, uint32_t saddr, uint32_t daddr, uint16_t tcplen);

uint8_t *
MakeIPPacket(struct flow_manager *etcp, tcp_flow *flow, uint16_t tcplen);

#endif /* IP_OUT_H */
