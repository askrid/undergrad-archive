#include <string.h>
#include <stdio.h>
#include "send_buffer.h"


#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static size_t chunk_size;
/*----------------------------------------------------------------------------*/
void
ConfigSendBufferSize(int size)
{
	chunk_size = size;
}
/*----------------------------------------------------------------------------*/
struct tcp_send_buffer *
InitSendBuffer(uint32_t init_seq)
{
	struct tcp_send_buffer *buf;
	buf = (struct tcp_send_buffer *)calloc(1,sizeof(struct tcp_send_buffer));

	if (!buf)
	{
		return NULL;
	}

	buf->data = malloc(chunk_size);
	if (!buf->data)
	{
		free(buf);
		return NULL;
	}

	buf->head = buf->data;
	buf->head_off = buf->tail_off = 0;
	buf->len = buf->cum_len = 0;
	buf->size = chunk_size;
	buf->init_seq = buf->head_seq = init_seq;

	return buf;
}
/*----------------------------------------------------------------------------*/
void FreeSendBuffer(struct tcp_send_buffer *buf)
{
	if (!buf)
		return;
	free(buf->data);
	free(buf);
}
/*----------------------------------------------------------------------------*/
size_t
InsertToSendBuffer(struct tcp_send_buffer *buf, const void *data, size_t len)
{
	size_t to_put;

	if (len <= 0)
		return 0;

	/* if no space, return -2 */
	to_put = MIN(len, buf->size - buf->len);
	if (to_put <= 0)
	{
		return -2;
	}

	if (buf->tail_off + to_put < buf->size)
	{
		/* if the data fit into the buffer, copy it */
		memcpy(buf->data + buf->tail_off, data, to_put);
		buf->tail_off += to_put;
	}
	else
	{
		/* if buffer overflows, move the existing payload and merge */
		memmove(buf->data, buf->head, buf->len);
		buf->head = buf->data;
		buf->head_off = 0;
		memcpy(buf->head + buf->len, data, to_put);
		buf->tail_off = buf->len + to_put;
	}
	buf->len += to_put;
	buf->cum_len += to_put;

	return to_put;
}
/*----------------------------------------------------------------------------*/
size_t
RemoveFromSendBuffer(struct tcp_send_buffer *buf, size_t len)
{
	if (len <= 0)
		return 0;

	size_t to_remove = MIN(len, buf->len);
	if (to_remove <= 0)
	{
		fprintf(stderr,"[RemoveFromSendBuffer] nothing to remove (len=%zu buf->len=%u)\n",
					len, buf->len);
		return 0;
	}

	buf->head_off += to_remove;
	buf->head = buf->data + buf->head_off;
	buf->head_seq += to_remove;
	buf->len -= to_remove;

	/* if buffer is empty, reset offsets */
	if (buf->len == 0 && buf->head_off > 0)
	{
		buf->head = buf->data;
		buf->head_off = buf->tail_off = 0;
	}

	return to_remove;
}

/*---------------------------------------------------------------------------*/
