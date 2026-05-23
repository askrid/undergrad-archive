#include <assert.h>

#include "util.h"
#include "receive_buffer.h"
#include "eventpoll.h"
#include "timer.h"
#include "ip_in.h"


/*---------------------------------------------------------------------------*/
uint16_t
TCPCalcChecksum(uint16_t *buf, uint16_t len, uint32_t saddr, uint32_t daddr)
{
	const uint8_t *data = (const uint8_t *)buf;
	const uint8_t *src = (const uint8_t *)&saddr;
	const uint8_t *dst = (const uint8_t *)&daddr;
	uint32_t sum = 0;
	uint16_t i;

	/* TCP header + payload */
	for (i = 0; i + 1 < len; i += 2)
	{
		sum += ((uint16_t)data[i] << 8) | data[i + 1];
	}

	/* odd byte */
	if (len & 1)
	{
		sum += ((uint16_t)data[len - 1] << 8);
	}

	/* pseudo header: source IP */
	sum += ((uint16_t)src[0] << 8) | src[1];
	sum += ((uint16_t)src[2] << 8) | src[3];

	/* pseudo header: destination IP */
	sum += ((uint16_t)dst[0] << 8) | dst[1];
	sum += ((uint16_t)dst[2] << 8) | dst[3];

	/* zero + protocol */
	sum += IPPROTO_TCP;

	/* TCP length */
	sum += len;

	while (sum >> 16)
	{
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	return (uint16_t)(~sum & 0xFFFF);
}
/*---------------------------------------------------------------------------*/
