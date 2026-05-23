#include "ip_util.h"
#include "ip_in.h"
#include "eth_in.h"
#include "arp.h"
#include <errno.h>
#include <linux/if_ether.h>
/*----------------------------------------------------------------------------*/
int
ProcessEthPacket(flow_manager_t etcp, const int ifidx, 
		uint32_t cur_ts, unsigned char *pkt_data, int len)
{
	struct ethhdr *ethh = (struct ethhdr *)pkt_data;
	u_short ip_proto = ntohs(ethh->h_proto);
	int ret;
	if (ip_proto == ETH_P_IP) {
		/* process ipv4 packet */
		ret = ProcessIPv4Packet(etcp, cur_ts, ifidx, pkt_data, len);

	} else if (ip_proto == ETH_P_ARP) {
		ProcessARPPacket(etcp, cur_ts, ifidx, pkt_data, len);
		return TRUE;

	} else {
		etcp->io_func->release_pkt(etcp->ctx);
		return TRUE;
	}
	return ret;
}
