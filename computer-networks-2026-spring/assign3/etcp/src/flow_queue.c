#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "flow_queue.h"
#include "tcp_flow.h"
#ifndef _INDEX_TYPE_
#define _INDEX_TYPE_
typedef uint32_t index_type;
typedef int32_t signed_index_type;
#endif
/*---------------------------------------------------------------------------*/
struct flow_queue
{
	index_type _capacity;
	volatile index_type _head;
	volatile index_type _tail;

	struct tcp_flow * volatile * _q;
};
/*----------------------------------------------------------------------------*/
internal_flow_queue * 
CreateInternalFlowQ(int size)
{
	internal_flow_queue *sq;

	sq = (internal_flow_queue *)calloc(1, sizeof(internal_flow_queue));
	if (!sq) {
		return NULL;
	}

	sq->array = (tcp_flow **)calloc(size, sizeof(tcp_flow *));
	if (!sq->array) {
		free(sq);
		return NULL;
	}

	sq->size = size;
	sq->first = sq->last = 0;
	sq->count = 0;

	return sq;
}
/*----------------------------------------------------------------------------*/
void 
DestroyInternalFlowQ(internal_flow_queue *sq)
{
	if (!sq)
		return;
	
	if (sq->array) {
		free(sq->array);
		sq->array = NULL;
	}

	free(sq);
}
/*----------------------------------------------------------------------------*/
int 
EnqueueToInternalFlowQ(internal_flow_queue *sq, struct tcp_flow *flow)
{
	if (sq->count >= sq->size) {
		/* queue is full */
		return -1;
	}

	sq->array[sq->last++] = flow;
	sq->count++;
	if (sq->last >= sq->size) {
		sq->last = 0;
	}
	assert (sq->count <= sq->size);

	return 0;
}
/*----------------------------------------------------------------------------*/
struct tcp_flow *
DequeueFromInternalFlowQ(internal_flow_queue *sq)
{
	struct tcp_flow *flow = NULL;

	if (sq->count <= 0) {
		return NULL;
	}

	flow = sq->array[sq->first++];
	assert(flow != NULL);
	if (sq->first >= sq->size) {
		sq->first = 0;
	}
	sq->count--;
	assert(sq->count >= 0);

	return flow;
}
/*---------------------------------------------------------------------------*/
static inline index_type 
NextIndex(flow_queue_t sq, index_type i)
{
	return (i != sq->_capacity ? i + 1: 0);
}
/*---------------------------------------------------------------------------*/
int 
FlowQueueIsEmpty(flow_queue_t sq)
{
	return (sq->_head == sq->_tail);
}
/*---------------------------------------------------------------------------*/
static inline void 
flowMemoryBarrier(tcp_flow * volatile flow, volatile index_type index)
{
	__asm__ volatile("" : : "m" (flow), "m" (index));
}
/*---------------------------------------------------------------------------*/
flow_queue_t 
CreateFlowQueue(int capacity)
{
	flow_queue_t sq;

	sq = (flow_queue_t)calloc(1, sizeof(struct flow_queue));
	if (!sq)
		return NULL;

	sq->_q = (tcp_flow **)calloc(capacity + 1, sizeof(tcp_flow *));
	if (!sq->_q) {
		free(sq);
		return NULL;
	}

	sq->_capacity = capacity;
	sq->_head = sq->_tail = 0;

	return sq;
}
/*---------------------------------------------------------------------------*/
void 
DestroyFlowQueue(flow_queue_t sq)
{
	if (!sq)
		return;

	if (sq->_q) {
		free((void *)sq->_q);
		sq->_q = NULL;
	}

	free(sq);
}
/*---------------------------------------------------------------------------*/
int 
EnqueueToFlowQ(flow_queue_t sq, tcp_flow *flow)
{
	index_type h = sq->_head;
	index_type t = sq->_tail;
	index_type nt = NextIndex(sq, t);

	if (nt != h) {
		sq->_q[t] = flow;
		flowMemoryBarrier(sq->_q[t], sq->_tail);
		sq->_tail = nt;
		return 0;
	}

	fprintf(stderr,"Exceed capacity of flow queue!\n");
	return -1;
}
/*---------------------------------------------------------------------------*/
tcp_flow *
DequeueFromFlowQ(flow_queue_t sq)
{
	index_type h = sq->_head;
	index_type t = sq->_tail;

	if (h != t) {
		tcp_flow *flow = sq->_q[h];
		flowMemoryBarrier(sq->_q[h], sq->_head);
		sq->_head = NextIndex(sq, h);
		assert(flow);
		return flow;
	}

	return NULL;
}
/*---------------------------------------------------------------------------*/
