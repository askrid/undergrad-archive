#ifndef ETH_OUT_H
#define ETH_OUT_H

#include <stdint.h>

#include "etcp.h"
#include "tcp_flow.h"
#include "ip_util.h"

#define MAX_SEND_PCK_CHUNK 64

uint8_t *
MakeEthernetPacket(struct flow_manager *etcp, uint16_t h_proto, 
		int nif, unsigned char* dst_haddr, uint16_t iplen);

#endif /* ETH_OUT_H */
