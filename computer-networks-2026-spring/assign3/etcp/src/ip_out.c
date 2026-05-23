#include <assert.h>
#include <linux/if_ether.h>
#include "ip_out.h"
#include "ip_in.h"
#include "eth_out.h"
#include "arp.h"

/*----------------------------------------------------------------------------*/
inline int
FetchNetworkInterface(uint32_t daddr, uint8_t *is_external)
{
	int nif = -1;
	int i;
	int prefix = 0;

	*is_external = 0;
	/* Longest prefix matching */
	for (i = 0; i < CONFIG.routes; i++)
	{
		if ((daddr & CONFIG.rtable[i].mask) == CONFIG.rtable[i].masked)
		{
			if (CONFIG.rtable[i].prefix > prefix)
			{
				nif = CONFIG.rtable[i].nif;
				prefix = CONFIG.rtable[i].prefix;
			}
			else if (CONFIG.gateway)
			{
				*is_external = 1;
				nif = (CONFIG.gateway)->nif;
			}
			break;
		}
	}

	if (nif < 0)
	{
		uint8_t *da = (uint8_t *)&daddr;
		fprintf(stderr,"[WARNING] No route to %u.%u.%u.%u\n",
					da[0], da[1], da[2], da[3]);
		assert(0);
	}

	return nif;
}
/*----------------------------------------------------------------------------*/
uint8_t *
MakeIPPacketWoutFlow(struct flow_manager *etcp, uint8_t protocol,
				   uint16_t ip_id, uint32_t saddr, uint32_t daddr, uint16_t payloadlen)
{
	struct iphdr *iph;
	int nif;
	unsigned char *haddr, is_external;

	nif = FetchNetworkInterface(daddr, &is_external);
	if (nif < 0)
		return NULL;

	haddr = GetDestinationHWaddr(daddr, is_external);
	if (!haddr)
	{
		RequestARP(etcp, (is_external) ? ((CONFIG.gateway)->daddr) : daddr,
				   nif, etcp->cur_ts);
		return NULL;
	}

	iph = (struct iphdr *)MakeEthernetPacket(etcp,
										 ETH_P_IP, nif, haddr, payloadlen + IP_HEADER_LEN);
	if (!iph)
	{
		return NULL;
	}

	iph->ihl = IP_HEADER_LEN >> 2;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = htons(IP_HEADER_LEN + payloadlen);
	iph->id = htons(ip_id);
	iph->frag_off = htons(IP_DF); // no fragmentation
	iph->ttl = 64;
	iph->protocol = protocol;
	iph->saddr = saddr;
	iph->daddr = daddr;
	iph->check = 0;
	iph->check = ip_fast_csum(iph, iph->ihl);

	return (uint8_t *)(iph + 1);
}
/*----------------------------------------------------------------------------*/
uint8_t *
MakeIPPacket(struct flow_manager *etcp, tcp_flow *flow, uint16_t tcplen)
{
	struct iphdr *iph;
	int nif;
	unsigned char *haddr, is_external = 0;

	nif = FetchNetworkInterface(flow->daddr, &is_external);
	flow->sndvar->nif_out = nif;
	flow->is_external = is_external;

	haddr = GetDestinationHWaddr(flow->daddr, flow->is_external);
	iph = (struct iphdr *)MakeEthernetPacket(etcp, ETH_P_IP,
										 flow->sndvar->nif_out, haddr, tcplen + IP_HEADER_LEN);
	if (!iph)
	{
		return NULL;
	}

	iph->ihl = IP_HEADER_LEN >> 2;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = htons(IP_HEADER_LEN + tcplen);
	iph->frag_off = htons(0x4000); // no fragmentation
	iph->ttl = 64;
	iph->protocol = IPPROTO_TCP;
	iph->saddr = flow->saddr;
	iph->daddr = flow->daddr;
	iph->check = 0;
	iph->check = ip_fast_csum(iph, iph->ihl);

	return (uint8_t *)(iph + 1);
}
