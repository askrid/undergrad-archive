#ifndef SEND_BUFFER
#define SEND_BUFFER
#include <stdlib.h>
#include <stdint.h>

/*----------------------------------------------------------------------------*/
struct tcp_send_buffer
{
	unsigned char *data;
	unsigned char *head;

	uint32_t head_off;
	uint32_t tail_off;
	uint32_t len;
	uint64_t cum_len;
	uint32_t size;

	uint32_t head_seq;
	uint32_t init_seq;
};
/*----------------------------------------------------------------------------*/
void
ConfigSendBufferSize(int size);
/*----------------------------------------------------------------------------*/
struct tcp_send_buffer *
InitSendBuffer(uint32_t init_seq);
/*----------------------------------------------------------------------------*/
void 
FreeSendBuffer(struct tcp_send_buffer *buf);
/*----------------------------------------------------------------------------*/
size_t 
InsertToSendBuffer(struct tcp_send_buffer *buf, const void *data, size_t len);
/*----------------------------------------------------------------------------*/
size_t 
RemoveFromSendBuffer(struct tcp_send_buffer *buf, size_t len);
/*----------------------------------------------------------------------------*/

#endif /* TCP_SEND_BUFFER_H */
