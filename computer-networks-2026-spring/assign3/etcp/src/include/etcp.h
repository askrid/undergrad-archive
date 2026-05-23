#ifndef etcp_H
#define etcp_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <pthread.h>
#include "memory_manager.h"
#include "receive_buffer.h"
#include "send_buffer.h"
#include "flow_queue.h"
#include "socket.h"
#include "etcp_api.h"
#include "eventpoll.h"
#include "addr.h"
#include "ip_util.h"
#include "stat.h"
#include "io_engine.h"

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef ERROR
#define ERROR (-1)
#endif

#define ETHERNET_HEADER_LEN 14 // sizeof(struct ethhdr)
#define IP_HEADER_LEN 20	   // sizeof(struct iphdr)
#define TCP_HEADER_LEN 20	   // sizeof(struct tcphdr)

/* configurations */
#define BACKLOG_SIZE (10 * 1024)
#define ETH_NUM 16

#ifndef MAX_CPUS
#define MAX_CPUS 16
#endif
/* add macro if it is not defined in /usr/include/sys/queue.h */
#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)        \
	for ((var) = TAILQ_FIRST((head));                     \
		 (var) && ((tvar) = TAILQ_NEXT((var), field), 1); \
		 (var) = (tvar))
#endif

/*----------------------------------------------------------------------------*/
struct eth_table
{
	char dev_name[128];
	int ifindex;
	int stat_print;
	unsigned char haddr[6];
	uint32_t netmask;
	uint32_t ip_addr;
};
/*----------------------------------------------------------------------------*/
struct route_table
{
	uint32_t daddr;
	uint32_t mask;
	uint32_t masked;
	int prefix;
	int nif;
};
/*----------------------------------------------------------------------------*/
struct arp_entry
{
	uint32_t ip;
	int8_t prefix;
	uint32_t ip_mask;
	uint32_t ip_masked;
	unsigned char haddr[6];
};
/*----------------------------------------------------------------------------*/
struct arp_table
{
	struct arp_entry *entry;
	struct arp_entry *gateway;
	int entries;
};
/*----------------------------------------------------------------------------*/
struct network_config
{
	/* network interface config */
	char ip_addr[INET_ADDRSTRLEN];
	struct eth_table *eths;
	int *nif_to_eidx; // mapping physic port indexes to that of the configured port-list
	int eths_num;

	/* route config */
	struct route_table *rtable; // routing table
	struct route_table *gateway;
	int routes; // # of entries

	/* arp config */
	struct arp_table arp;

	int num_cores;
	int max_concurrency;

	int max_num_buffers;
	int rcvbuf_size;
	int sndbuf_size;

	int tcp_timewait;
	int tcp_timeout;

	/* fault injection for testing */
	int drop_syn_nth;
	int drop_synack_nth;
	int drop_fin_nth;
	int drop_data_rate;
	int drop_ack_rate;
	int drop_data_nth;
	int loss_seed;

	/* adding multi-process support */
	uint8_t multi_process;
	uint8_t multi_process_is_master;

	int cc_number;
};
/*----------------------------------------------------------------------------*/
struct etcp_engine
{
	int cpu;
};
/*----------------------------------------------------------------------------*/
struct pkt_sender
{
	int ifidx;

	/* TCP layer send queues */
	TAILQ_HEAD(control_head, tcp_flow)
	control_pkt_list;
	TAILQ_HEAD(send_head, tcp_flow)
	data_pkt_list;
	TAILQ_HEAD(ack_head, tcp_flow)
	ack_pkt_list;

	int control_pkt_list_cnt;
	int data_pkt_list_cnt;
	int ack_pkt_list_cnt;
};
/*----------------------------------------------------------------------------*/
struct flow_manager
{
	struct hashtable *tcp_flow_table;

	socket_map_t smap;
	TAILQ_HEAD(, socket_map)
	free_smap;

	addr_pool_t ap; /* address pool */

	uint32_t flow_cnt; /* number of concurrent flows */

	struct etcp_thread_context *ctx;

	/* variables related to event */
	struct etcp_epoll *ep;
	uint32_t ts_last_event;

	struct hashtable *listeners;

	flow_queue_t connectq; /* flows need to connect */
	flow_queue_t sendq;	   /* flows need to send data */
	flow_queue_t ackq;	   /* flows need to send ack */

	flow_queue_t closeq;			 /* flows need to close */
	internal_flow_queue *closeq_int; /* internally maintained closeq */
	flow_queue_t resetq;			 /* flows need to reset */
	internal_flow_queue *resetq_int; /* internally maintained resetq */

	flow_queue_t destroyq; /* flows need to be destroyed */

	struct pkt_sender *g_sender;
	struct pkt_sender *n_sender[ETH_NUM];

	/* lists related to timeout */
	struct rto_hashstore *rto_store;
	TAILQ_HEAD(timewait_head, tcp_flow)
	timewait_list;
	TAILQ_HEAD(timeout_head, tcp_flow)
	timeout_list;

	int rto_list_cnt;
	int timewait_list_cnt;
	int timeout_list_cnt;

	uint32_t cur_ts;

	int wakeup_flag;
	int is_sleeping;
	struct io_engine *io_func;
};
/*----------------------------------------------------------------------------*/
typedef struct flow_manager *flow_manager_t;
/*----------------------------------------------------------------------------*/
flow_manager_t
GetFlowManager(etcp_engine_t etcp_engine);
/*----------------------------------------------------------------------------*/
struct etcp_thread_context
{
	int cpu;
	pthread_t thread;
	uint8_t done : 1,
		exit : 1,
		interrupt : 1;

	struct flow_manager *flow_manager;

	void *io_private_context;
	pthread_mutex_t smap_lock;
	pthread_mutex_t socket_pool_lock;
};
/*----------------------------------------------------------------------------*/
typedef struct etcp_thread_context *etcp_thread_context_t;
/*----------------------------------------------------------------------------*/
extern struct flow_manager *g_etcp[MAX_CPUS];
extern struct network_config CONFIG;
extern addr_pool_t ap[ETH_NUM];
/*----------------------------------------------------------------------------*/

#endif /* etcp_H */
