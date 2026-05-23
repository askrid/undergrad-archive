#ifndef TCP_FLOW_Q
#define TCP_FLOW_Q

#include <stdint.h>
/*---------------------------------------------------------------------------*/
typedef struct flow_queue* flow_queue_t;
/*---------------------------------------------------------------------------*/
typedef struct internal_flow_queue
{
	struct tcp_flow **array;
	int size;

	int first;
	int last;
	int count;

} internal_flow_queue;
/*---------------------------------------------------------------------------*/
internal_flow_queue * 
CreateInternalFlowQ(int size);
/*---------------------------------------------------------------------------*/
void 
DestroyInternalFlowQ(internal_flow_queue *sq);
/*---------------------------------------------------------------------------*/
int 
EnqueueToInternalFlowQ(internal_flow_queue *sq, struct tcp_flow *flow);
/*---------------------------------------------------------------------------*/
struct tcp_flow *
DequeueFromInternalFlowQ(internal_flow_queue *sq);
/*---------------------------------------------------------------------------*/
flow_queue_t 
CreateFlowQueue(int size);
/*---------------------------------------------------------------------------*/
void 
DestroyFlowQueue(flow_queue_t sq);
/*---------------------------------------------------------------------------*/
int 
EnqueueToFlowQ(flow_queue_t sq, struct tcp_flow *flow);
/*---------------------------------------------------------------------------*/
struct tcp_flow *
DequeueFromFlowQ(flow_queue_t sq);
/*---------------------------------------------------------------------------*/
int 
FlowQueueIsEmpty(flow_queue_t sq);
/*---------------------------------------------------------------------------*/

#endif