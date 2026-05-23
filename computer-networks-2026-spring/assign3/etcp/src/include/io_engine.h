#ifndef io_engine_H
#define io_engine_H
/*----------------------------------------------------------------------------*/
/* for type def'ns */
#include <stdint.h>
/* for ps lib funcs */
#include "ip_util.h"
/*----------------------------------------------------------------------------*/
/**
 * Declaration to soothe down the warnings
 */
struct etcp_thread_context;
/**
 * io_engines - contains template for the various 10Gbps pkt I/O
 *                 - libraries that can be adopted.
 *
 *		   load_module()    : Used to set system-wide I/O module
 *				      initialization.
 *
 *                 init_handle()    : Used to initialize the driver library
 *                                  : Also use the context to create/initialize
 *                                  : a private packet I/O data structures.
 *
 *                 link_devices()   : Used to add link(s) to the etcp stack.
 *				      Returns 0 on success; -1 on failure.
 *
 *		   release_pkt()    : release the packet if etcp does not need
 *				      to process it (e.g. non-IPv4, non-TCP pkts).
 *
 *		   get_wptr()	    : retrieve the next empty pkt buffer for the
 * 				      application for packet writing. Returns
 *				      ptr to pkt buffer.
 *
 *		   send_pkts()	    : transmit batch of packets via interface
 * 				      idx (=nif).
 *				      Returns 0 on success; -1 on failure
 *
 *		   get_rptr()	    : retrieve next pkt for application for
 *				      packet read.
 *				      Returns ptr to pkt buffer.
 *
 *		   recv_pkts()	    : recieve batch of packets from the interface,
 *				      ifidx.
 *				      Returns no. of packets that are read from
 *				      the iface.
 *
 *		   select()	    : for blocking I/O
 *
 *		   destroy_handle() : free up resources allocated during
 * 				      init_handle(). Normally called during
 *				      process termination.
 *
 *                 dev_ioctl()      : contains submodules for select drivers
 *
 */
typedef struct io_engine
{
	void (*load_module)(void);
	void (*init_handle)(struct etcp_thread_context *ctx);
	void (*release_pkt)(struct etcp_thread_context *ctx);
	uint8_t *(*get_wptr)(struct etcp_thread_context *ctx, uint16_t len);
	int32_t (*send_pkts)(struct etcp_thread_context *ctx);
	uint8_t *(*get_rptr)(struct etcp_thread_context *ctx, int index, uint16_t *len);
	int32_t (*recv_pkts)(struct etcp_thread_context *ctx);
	void (*destroy_handle)(struct etcp_thread_context *ctx);
	int32_t (*dev_ioctl)(struct etcp_thread_context *ctx, int nif, int cmd, void *argp);
} io_engine __attribute__((aligned(__WORDSIZE)));
/*----------------------------------------------------------------------------*/
/* set I/O module context */
int ConfigNetworkEnv(char *port_list, char *port_stat_list);

/*----------------------------------------------------------------------------*/
/* ptr to the `running' I/O module context */
extern io_engine *current_iomodule_func;

/* dev_ioctl related macros */
#define PKT_TX_IP_CSUM 0x01
#define PKT_TX_TCP_CSUM 0x02
#define PKT_RX_TCP_LROSEG 0x03
#define PKT_TX_TCPIP_CSUM 0x04
#define PKT_RX_IP_CSUM 0x05
#define PKT_RX_TCP_CSUM 0x06
#define PKT_TX_TCPIP_CSUM_PEEK 0x07
#define DRV_NAME 0x08

/* registered pcap module*/
extern io_engine pcap_module_func;

/* Macro to assign IO module */
#define SetIOEngine(m){\
	if (!strcmp(m, "pcap"))	\
		current_iomodule_func = &pcap_module_func;	\
	else	\
		assert(0);	\
}
/*----------------------------------------------------------------------------*/
#endif /* io_engine_H */
