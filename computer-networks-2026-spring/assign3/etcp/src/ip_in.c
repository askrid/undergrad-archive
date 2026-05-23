#include <string.h>
#include <netinet/ip.h>

#include "ip_in.h"
#include "tcp_in.h"
#include "etcp_api.h"
#include "ip_util.h"
#define ETH_P_IP_FRAG   0xF800
#define ETH_P_IPV6_FRAG 0xF6DD

/*----------------------------------------------------------------------------*/
inline int 
ProcessIPv4Packet(flow_manager_t etcp, uint32_t cur_ts, 
				  const int ifidx, unsigned char* pkt_data, int len)
{
	/* check and process IPv4 packets */
	struct iphdr* iph = (struct iphdr *)(pkt_data + sizeof(struct ethhdr));
	int ip_len = ntohs(iph->tot_len);

	/* drop the packet shorter than ip header */
	if (ip_len < sizeof(struct iphdr))
		return ERROR;

	if (ip_fast_csum(iph, iph->ihl))
		return ERROR;

	if (iph->version != 0x4 ) {
		etcp->io_func->release_pkt(etcp->ctx);
		return FALSE;
	}
	
	switch (iph->protocol) {
		case IPPROTO_TCP:
			return ProcessTCPPacket(etcp, cur_ts, iph, ip_len);
		default:
			/* currently drop other protocols */
			return FALSE;
	}
	return FALSE;
}
/*----------------------------------------------------------------------------*/
