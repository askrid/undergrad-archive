#include <stdio.h>

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <linux/if_ether.h>
#include <linux/tcp.h>
#include <netinet/ip.h>

#include "etcp.h"
#include "arp.h"
#include "eth_out.h"

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef ERROR
#define ERROR (-1)
#endif

#define MAX(a, b) ((a)>(b)?(a):(b))
#define MIN(a, b) ((a)<(b)?(a):(b))


/*----------------------------------------------------------------------------*/
uint8_t *
MakeEthernetPacket(struct flow_manager *etcp, uint16_t h_proto, 
		int nif, unsigned char* dst_haddr, uint16_t iplen)
{
	uint8_t *buf;
	struct ethhdr *ethh;
	int i, eidx;

	/* 
 	 * -sanity check- 
	 * return early if no interface is set (if routing entry does not exist)
	 */
	if (nif < 0) {
		return NULL;
	}

	eidx = CONFIG.nif_to_eidx[nif];
	if (eidx < 0) {
		return NULL;
	}
	
	buf = etcp->io_func->get_wptr(etcp->ctx, iplen + ETHERNET_HEADER_LEN);
	// allocate memory for TX path packet
	if (!buf) {
		perror("Failed to get available write buffer\n");
		return NULL;
	}
	
	ethh = (struct ethhdr *)buf;
	for (i = 0; i < 6; i++) {
		ethh->h_source[i] = CONFIG.eths[eidx].haddr[i];
		ethh->h_dest[i] = dst_haddr[i];
	}
	ethh->h_proto = htons(h_proto);

	return (uint8_t *)(ethh + 1);
}
/*----------------------------------------------------------------------------*/
