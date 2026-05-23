#ifndef RECEIVE_BUFFER
#define RECEIVE_BUFFER

#include <stdint.h>
#include <sys/types.h>

/*----------------------------------------------------------------------------*/
enum rb_caller
{
	AT_APP,
	AT_etcp
};
/*----------------------------------------------------------------------------*/
struct fragment_ctx
{
	uint32_t seq;
	uint32_t len : 31;
	struct fragment_ctx *next;
};
/*----------------------------------------------------------------------------*/
struct tcp_receive_buffer
{
	u_char *data; /* buffered data */
	u_char *head; /* pointer to the head */

	uint32_t head_offset; /* offset for the head (head - data) */
	uint32_t tail_offset; /* offset fot the last byte (null byte) */

	int merged_len;	  /* contiguously merged length */
	uint64_t cum_len; /* cummulatively merged length */
	int last_len;	  /* currently saved data length */
	int size;		  /* total ring buffer size */

	/* TCP payload features */
	uint32_t head_seq;
	uint32_t init_seq;

	struct fragment_ctx *fctx;
};
/*----------------------------------------------------------------------------*/
void ConfigReceiveBufferSize(size_t size);
/*----------------------------------------------------------------------------*/
struct tcp_receive_buffer *InitReceiveBuffer(uint32_t init_seq);
void FreeReceiveBuffer(struct tcp_receive_buffer *buff);
/*----------------------------------------------------------------------------*/
/* data manupulation functions */
int InsertToReceiveBuffer(struct tcp_receive_buffer *buff,
		  void *data, uint32_t len, uint32_t seq);
size_t RemoveFromReceiveBuffer(struct tcp_receive_buffer *buff, size_t len, int option);
/*----------------------------------------------------------------------------*/

#endif
