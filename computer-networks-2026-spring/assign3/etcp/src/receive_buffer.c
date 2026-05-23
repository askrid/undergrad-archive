#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include "receive_buffer.h"
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
static size_t chunk_size;
/*-----------------------------------------------------------------------------*/
void
ConfigReceiveBufferSize(size_t size)
{
	chunk_size = size;
}
/*----------------------------------------------------------------------------*/
void 
DestroyFragmentCtx(struct fragment_ctx *fctx)
{
	struct fragment_ctx *remove;
	if (!fctx)
		return;

	while (fctx)
	{
		remove = fctx;
		fctx = fctx->next;
		free(remove);
	}
}
/*----------------------------------------------------------------------------*/
static struct fragment_ctx *
MakeFragmentCtx()
{
	/* this function should be called only in etcp thread */
	struct fragment_ctx *frag;
	/* first try deqeue the fragment in free fragment queue */
	frag = (struct fragment_ctx *)calloc(1, sizeof(struct fragment_ctx));

	if (!frag)
	{
		fprintf(stderr,"calloc failed\n");
		exit(-1);
	}
	return frag;
}
/*----------------------------------------------------------------------------*/
struct tcp_receive_buffer *
InitReceiveBuffer(uint32_t init_seq)
{
	struct tcp_receive_buffer *buff =
		(struct tcp_receive_buffer *)calloc(1, sizeof(struct tcp_receive_buffer));

	if (buff == NULL)
	{
		perror("rb_init buff");
		return NULL;
	}

	buff->data = malloc(chunk_size);
	if (!buff->data)
	{
		perror("rb_init failed");
		free(buff);
		return NULL;
	}

	buff->size = chunk_size;
	buff->head = buff->data;
	buff->head_seq = init_seq;
	buff->init_seq = init_seq;
	buff->last_len = 0;
	buff->merged_len = 0;
	buff->head_offset = 0;
	buff->tail_offset = 0;
	buff->fctx = NULL;
	return buff;
}
/*----------------------------------------------------------------------------*/
void FreeReceiveBuffer(struct tcp_receive_buffer *buff)
{	assert(buff);
	if (buff->fctx)
	{
		DestroyFragmentCtx(buff->fctx);
		buff->fctx = NULL;
	}

	if (buff->data)
	{
		free(buff->data);
		buff->data = NULL;
	}

	free(buff);
}
/*----------------------------------------------------------------------------*/
#define MAXSEQ ((uint32_t)(0xFFFFFFFF))
/* Segment Merge related APIs */
/*----------------------------------------------------------------------------*/
static inline uint32_t
GetMinSeq(uint32_t a, uint32_t b)
{
	if (a == b)
		return a;
	if (a < b)
		return ((b - a) <= MAXSEQ / 2) ? a : b;
	/* b < a */
	return ((a - b) <= MAXSEQ / 2) ? b : a;
}
/*----------------------------------------------------------------------------*/
static inline uint32_t
GetMaxSeq(uint32_t a, uint32_t b)
{
	if (a == b)
		return a;
	if (a < b)
		return ((b - a) <= MAXSEQ / 2) ? b : a;
	/* b < a */
	return ((a - b) <= MAXSEQ / 2) ? a : b;
}
/*----------------------------------------------------------------------------*/
static inline int
CanMerge(const struct fragment_ctx *a, const struct fragment_ctx *b)
{
	uint32_t a_end = a->seq + a->len + 1;
	uint32_t b_end = b->seq + b->len + 1;

	if (GetMinSeq(a_end, b->seq) == a_end ||
		GetMinSeq(b_end, a->seq) == b_end)
		return 0;

	return 1;
}
/*----------------------------------------------------------------------------*/
static inline void
MergeFragments(struct fragment_ctx *a, struct fragment_ctx *b)
{
	/* merge a into b */
	uint32_t min_seq, max_seq;

	min_seq = GetMinSeq(a->seq, b->seq);
	max_seq = GetMaxSeq(a->seq + a->len, b->seq + b->len);
	b->seq = min_seq;
	b->len = max_seq - min_seq;
}
/*----------------------------------------------------------------------------*/
int InsertToReceiveBuffer(struct tcp_receive_buffer *buff,
		  void *data, uint32_t len, uint32_t cur_seq)
{
	uint32_t putx, end_off; /* To decide where to put in buffer */
	struct fragment_ctx *new_ctx;
	struct fragment_ctx *iter;
	struct fragment_ctx *prev, *pprev;
	uint32_t merged = 0;

	if (len <= 0)
		return 0;

	// if data offset is smaller than head sequence, then drop
	if (GetMinSeq(buff->head_seq, cur_seq) != buff->head_seq)
		return 0;

	putx = cur_seq - buff->head_seq;
	end_off = putx + len;
	if (buff->size < end_off) // segment does not fit
	{
		return -1;
	}

	// if buffer is at tail, move the data to the first of head
	// reset head_offset
	// keeps buffer logically linear
	if (buff->size <= (buff->head_offset + end_off))
	{
		memmove(buff->data, buff->head, buff->last_len);
		buff->tail_offset -= buff->head_offset;
		buff->head_offset = 0;
		buff->head = buff->data;
	}

	// copy data to buffer
	// bytes are placed at the correct offset relative to head_seq
	memcpy(buff->head + putx, data, len);

	// Tracks how much valid data exists in the byte buffer
	if (buff->tail_offset < buff->head_offset + end_off)
		buff->tail_offset = buff->head_offset + end_off;
	buff->last_len = buff->tail_offset - buff->head_offset;

	// create fragmentation context blocks
	new_ctx = MakeFragmentCtx();
	if (!new_ctx)
	{
		perror("allocating new_ctx failed");
		return 0;
	}
	new_ctx->seq = cur_seq;
	new_ctx->len = len;
	new_ctx->next = NULL;

	// traverse the fragment list, and merge the new fragment if possible
	for (iter = buff->fctx, prev = NULL, pprev = NULL;
		 iter != NULL;
		 pprev = prev, prev = iter, iter = iter->next)
	{

		if (CanMerge(new_ctx, iter))
		{
			/* merge the first fragment into the second fragment */
			MergeFragments(new_ctx, iter);

			/* remove the first fragment */
			if (prev == new_ctx)
			{
				if (pprev)
					pprev->next = iter;
				else
					buff->fctx = iter;
				prev = pprev;
			}
			free(new_ctx);
			new_ctx = iter;
			merged = 1;
		}
		else if (merged ||
				 GetMaxSeq(cur_seq + len, iter->seq) == iter->seq)
		{
			/* merged at some point, but no more mergeable
			   then stop it now */
			break;
		}
	}

	if (!merged)
	{
		if (buff->fctx == NULL)
		{
			buff->fctx = new_ctx;
		}
		else if (GetMinSeq(cur_seq, buff->fctx->seq) == cur_seq)
		{
			/* if the new packet's seqnum is before the existing fragments */
			new_ctx->next = buff->fctx;
			buff->fctx = new_ctx;
		}
		else
		{
			/* if the seqnum is in-between the fragments or
			   at the last */
			assert(GetMinSeq(cur_seq, prev->seq + prev->len) ==
				   prev->seq + prev->len);
			prev->next = new_ctx;
			new_ctx->next = iter;
		}
	}

	// Update deliverable data
	// If fragment list starts exactly at head_seq, then we now have contiguous in-order data
	// merged_len tracks how many bytes the application can read
	if (buff->head_seq == buff->fctx->seq)
	{
		buff->cum_len += buff->fctx->len - buff->merged_len;
		buff->merged_len = buff->fctx->len;
	}

	return len;
}
/*----------------------------------------------------------------------------*/
/*
 * RemoveFromReceiveBuffer()
 * remove already-merged, in-order data from the receive buffer
 * and send to application
 */
size_t
RemoveFromReceiveBuffer(struct tcp_receive_buffer *buff, size_t len, int option)
{
	/* this function should be called only in application thread */
	if (buff->merged_len < len)
		len = buff->merged_len;

	if (len == 0)
		return 0;

	buff->head_offset += len;
	buff->head = buff->data + buff->head_offset;
	buff->head_seq += len;

	buff->merged_len -= len;
	buff->last_len -= len;

	// modify fragementation chunks
	if (len == buff->fctx->len)
	{
		struct fragment_ctx *remove = buff->fctx;
		buff->fctx = buff->fctx->next;
		free(remove);
	}
	else if (len < buff->fctx->len)
	{
		buff->fctx->seq += len;
		buff->fctx->len -= len;
	}
	else
	{
		assert(0);
	}

	return len;
}
/*----------------------------------------------------------------------------*/
