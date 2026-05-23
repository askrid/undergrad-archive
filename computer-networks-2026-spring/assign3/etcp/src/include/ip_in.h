#ifndef IP_IN_H
#define IP_IN_H

#include "etcp.h"

int
ProcessIPv4Packet(flow_manager_t etcp, uint32_t cur_ts, 
				  const int ifidx, unsigned char* pkt_data, int len);

#endif /* IP_IN_H */
