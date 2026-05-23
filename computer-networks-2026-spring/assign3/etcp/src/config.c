#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "etcp.h"
#include "config.h"
#include "tcp_in.h"
#include "arp.h"
#include "io_engine.h"
#include <net/if.h>

#define MAX_ROUTE_ENTRY 64
#define MAX_OPTLINE_LEN 1024
#define ALL_STRING "all"

static const char *route_file = "config/route.conf";
static const char *arp_file = "config/arp.conf";\
int cc_number;
struct flow_manager *g_etcp[MAX_CPUS] = {NULL};
struct network_config CONFIG = {
	/* set default configuration */
	.max_concurrency = 10000,
	.max_num_buffers = 10000,
	.rcvbuf_size = -1,
	.sndbuf_size = -1,
	.tcp_timeout = TCP_TIMEOUT,
	.tcp_timewait = TCP_TIMEWAIT,
	
	.drop_syn_nth = 0,
	.drop_synack_nth = 0,
	.drop_fin_nth = 0,
	.drop_data_rate = 0,
	.drop_ack_rate = 0,
	.drop_data_nth = 0,
	.loss_seed = 1,
};
addr_pool_t ap[ETH_NUM] = {NULL};
static char port_list[MAX_OPTLINE_LEN] = "";
static char port_stat_list[MAX_OPTLINE_LEN] = "";
/* total cpus detected in the etcp stack*/
int num_cpus;
/* this should be equal to num_cpus */
int num_queues;
int num_devices;

int num_devices_attached;
int devices_attached[16];
/*----------------------------------------------------------------------------*/
static inline int
mystrtol(const char *nptr, int base)
{
	int rval;
	char *endptr;

	errno = 0;
	rval = strtol(nptr, &endptr, 10);
	/* check for strtol errors */
	if ((errno == ERANGE && (rval == LONG_MAX ||
							 rval == LONG_MIN)) ||
		(errno != 0 && rval == 0))
	{
		perror("strtol");
		exit(EXIT_FAILURE);
	}
	if (endptr == nptr)
	{
		exit(EXIT_FAILURE);
	}

	return rval;
}
/*----------------------------------------------------------------------------*/
static int
GetIntValue(char *value)
{
	int ret = 0;
	ret = strtol(value, (char **)NULL, 10);
	if (errno == EINVAL || errno == ERANGE)
		return -1;
	return ret;
}
/*----------------------------------------------------------------------------*/
inline uint32_t
MaskFromPrefix(int prefix)
{
	uint32_t mask = 0;
	uint8_t *mask_t = (uint8_t *)&mask;
	int i, j;

	for (i = 0; i <= prefix / 8 && i < 4; i++)
	{
		for (j = 0; j < (prefix - i * 8) && j < 8; j++)
		{
			mask_t[i] |= (1 << (7 - j));
		}
	}

	return mask;
}
/*----------------------------------------------------------------------------*/
static void
EnrollRouteTableEntry(char *optstr)
{
	char *daddr_s;
	char *prefix;
	char *dev;
	int i;
	int ifidx;
	int ridx;
	char *saveptr;

	saveptr = NULL;
	daddr_s = strtok_r(optstr, "/", &saveptr);
	prefix = strtok_r(NULL, " ", &saveptr);
	dev = strtok_r(NULL, "\n", &saveptr);
	assert(daddr_s != NULL);
	assert(prefix != NULL);
	assert(dev != NULL);

	ifidx = -1;
	if (current_iomodule_func == &pcap_module_func)
	{
		for (i = 0; i < num_devices; i++)
		{
			if (strcmp(CONFIG.eths[i].dev_name, dev))
				continue;
			ifidx = CONFIG.eths[i].ifindex;
			break;
		}
	}

	ridx = CONFIG.routes++;
	if (ridx == MAX_ROUTE_ENTRY)
	{
		exit(4);
	}

	CONFIG.rtable[ridx].daddr = inet_addr(daddr_s);
	CONFIG.rtable[ridx].prefix = mystrtol(prefix, 10);
	if (CONFIG.rtable[ridx].prefix > 32 || CONFIG.rtable[ridx].prefix < 0)
	{
		exit(4);
	}

	CONFIG.rtable[ridx].mask = MaskFromPrefix(CONFIG.rtable[ridx].prefix);
	CONFIG.rtable[ridx].masked =
		CONFIG.rtable[ridx].daddr & CONFIG.rtable[ridx].mask;
	CONFIG.rtable[ridx].nif = ifidx;

	if (CONFIG.rtable[ridx].mask == 0)
	{
		CONFIG.gateway = &CONFIG.rtable[ridx];
	}
}
/*----------------------------------------------------------------------------*/
int SetRoutingTableFromFile()
{
#define ROUTES "ROUTES"

	FILE *fc;
	char optstr[MAX_OPTLINE_LEN];
	int i;

	fc = fopen(route_file, "r");
	if (fc == NULL)
	{
		perror("fopen");
		return -1;
	}

	while (1)
	{
		char *iscomment;
		int num;

		if (fgets(optstr, MAX_OPTLINE_LEN, fc) == NULL)
			break;

		// skip comment
		iscomment = strchr(optstr, '#');
		if (iscomment == optstr)
			continue;
		if (iscomment != NULL)
			*iscomment = 0;

		if (!strncmp(optstr, ROUTES, sizeof(ROUTES) - 1))
		{
			num = GetIntValue(optstr + sizeof(ROUTES));
			if (num <= 0)
				break;

			for (i = 0; i < num; i++)
			{
				if (fgets(optstr, MAX_OPTLINE_LEN, fc) == NULL)
					break;

				if (*optstr == '#')
				{
					i -= 1;
					continue;
				}
				if (!CONFIG.gateway)
					EnrollRouteTableEntry(optstr);
				else
				{
					fprintf(stderr, "Default gateway settings in %s should "
									"always come as last entry!\n",
							route_file);
					exit(EXIT_FAILURE);
				}
			}
		}
	}

	fclose(fc);
	return 0;
}
/*----------------------------------------------------------------------------*/
void PrintRoutingTable()
{
	int i;
	uint8_t *da;
	uint8_t *m;
	uint8_t *md;

	/* print out process start information */
	printf("Routes:\n");
	for (i = 0; i < CONFIG.routes; i++)
	{
		da = (uint8_t *)&CONFIG.rtable[i].daddr;
		m = (uint8_t *)&CONFIG.rtable[i].mask;
		md = (uint8_t *)&CONFIG.rtable[i].masked;
		printf("Destination: %u.%u.%u.%u/%d, Mask: %u.%u.%u.%u, "
			   "Masked: %u.%u.%u.%u, Route: ifdx-%d\n",
			   da[0], da[1], da[2], da[3], CONFIG.rtable[i].prefix,
			   m[0], m[1], m[2], m[3], md[0], md[1], md[2], md[3],
			   CONFIG.rtable[i].nif);
	}
	if (CONFIG.routes == 0)
		printf("(blank)\n");

	printf("----------------------------------------------------------"
		   "-----------------------\n");
}
/*----------------------------------------------------------------------------*/
void ParseMACAddress(unsigned char *haddr, char *haddr_str)
{
	int i;
	char *str;
	unsigned int temp;
	char *saveptr = NULL;

	saveptr = NULL;
	str = strtok_r(haddr_str, ":", &saveptr);
	i = 0;
	while (str != NULL)
	{
		if (i >= 6)
		{
			exit(4);
		}
		if (sscanf(str, "%x", &temp) < 1)
		{
			exit(4);
		}
		haddr[i++] = temp;
		str = strtok_r(NULL, ":", &saveptr);
	}
	if (i < 6)
	{
		exit(4);
	}
}
/*----------------------------------------------------------------------------*/
int ParseIPAddress(uint32_t *ip_addr, char *ip_str)
{
	if (ip_str == NULL)
	{
		*ip_addr = 0;
		return -1;
	}

	*ip_addr = inet_addr(ip_str);
	if (*ip_addr == INADDR_NONE)
	{
		*ip_addr = 0;
		return -1;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int SetRoutingTable()
{
	int i, ridx;
	unsigned int c;

	CONFIG.routes = 0;
	CONFIG.rtable = (struct route_table *)
		calloc(MAX_ROUTE_ENTRY, sizeof(struct route_table));
	if (!CONFIG.rtable)
		exit(EXIT_FAILURE);

	/* set default routing table */
	for (i = 0; i < CONFIG.eths_num; i++)
	{

		ridx = CONFIG.routes++;
		CONFIG.rtable[ridx].daddr = CONFIG.eths[i].ip_addr & CONFIG.eths[i].netmask;

		CONFIG.rtable[ridx].prefix = 0;
		c = CONFIG.eths[i].netmask;
		while ((c = (c >> 1)))
		{
			CONFIG.rtable[ridx].prefix++;
		}
		CONFIG.rtable[ridx].prefix++;

		CONFIG.rtable[ridx].mask = CONFIG.eths[i].netmask;
		CONFIG.rtable[ridx].masked = CONFIG.rtable[ridx].daddr;
		CONFIG.rtable[ridx].nif = CONFIG.eths[ridx].ifindex;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
void PrintInterfaceInfo()
{
	int i;

	/* print out process start information */
	printf("Interfaces:\n");
	for (i = 0; i < CONFIG.eths_num; i++)
	{

		uint8_t *da = (uint8_t *)&CONFIG.eths[i].ip_addr;
		uint8_t *nm = (uint8_t *)&CONFIG.eths[i].netmask;

		printf("name: %s, ifindex: %d, "
			   "hwaddr: %02X:%02X:%02X:%02X:%02X:%02X, "
			   "ipaddr: %u.%u.%u.%u, "
			   "netmask: %u.%u.%u.%u\n",
			   CONFIG.eths[i].dev_name,
			   CONFIG.eths[i].ifindex,
			   CONFIG.eths[i].haddr[0],
			   CONFIG.eths[i].haddr[1],
			   CONFIG.eths[i].haddr[2],
			   CONFIG.eths[i].haddr[3],
			   CONFIG.eths[i].haddr[4],
			   CONFIG.eths[i].haddr[5],
			   da[0], da[1], da[2], da[3],
			   nm[0], nm[1], nm[2], nm[3]);
	}
	printf("----------------------------------------------------------"
		   "-----------------------\n");
}
/*----------------------------------------------------------------------------*/
static void
EnrollARPTableEntry(char *optstr)
{
	char *dip_s;	/* destination IP string */
	char *prefix_s; /* IP prefix string */
	char *daddr_s;	/* destination MAC string */

	int prefix;
	uint32_t dip_mask;
	int idx;

	char *saveptr;

	saveptr = NULL;
	dip_s = strtok_r(optstr, "/", &saveptr);
	prefix_s = strtok_r(NULL, " ", &saveptr);
	daddr_s = strtok_r(NULL, "\n", &saveptr);

	assert(dip_s != NULL);
	assert(prefix_s != NULL);
	assert(daddr_s != NULL);

	if (prefix_s == NULL)
		prefix = 32;
	else
		prefix = mystrtol(prefix_s, 10);

	if (prefix > 32 || prefix < 0)
	{
		return;
	}

	idx = CONFIG.arp.entries++;

	CONFIG.arp.entry[idx].prefix = prefix;
	ParseIPAddress(&CONFIG.arp.entry[idx].ip, dip_s);
	ParseMACAddress(CONFIG.arp.entry[idx].haddr, daddr_s);

	dip_mask = MaskFromPrefix(prefix);
	CONFIG.arp.entry[idx].ip_mask = dip_mask;
	CONFIG.arp.entry[idx].ip_masked = CONFIG.arp.entry[idx].ip & dip_mask;
	if (CONFIG.gateway && ((CONFIG.gateway)->daddr &
						   CONFIG.arp.entry[idx].ip_mask) ==
							  CONFIG.arp.entry[idx].ip_masked)
	{
		CONFIG.arp.gateway = &CONFIG.arp.entry[idx];
	}
}
/*----------------------------------------------------------------------------*/
int LoadARPTable()
{
#define ARP_ENTRY "ARP_ENTRY"

	FILE *fc;
	char optstr[MAX_OPTLINE_LEN];
	int numEntry = 0;
	int hasNumEntry = 0;

	printf("Loading ARP table from : %s\n", arp_file);

	InitARPTable();

	fc = fopen(arp_file, "r");
	if (fc == NULL)
	{
		perror("fopen");
		printf("Skip loading static ARP table\n");
		return -1;
	}

	while (1)
	{
		char *p;
		char *temp;

		if (fgets(optstr, MAX_OPTLINE_LEN, fc) == NULL)
			break;

		p = optstr;

		// skip comment
		if ((temp = strchr(p, '#')) != NULL)
			*temp = 0;
		// remove front and tailing spaces
		while (*p && isspace((int)*p))
			p++;
		temp = p + strlen(p) - 1;
		while (temp >= p && isspace((int)*temp))
			*temp = 0;
		if (*p == 0) /* nothing more to process? */
			continue;

		if (!hasNumEntry && strncmp(p, ARP_ENTRY, sizeof(ARP_ENTRY) - 1) == 0)
		{
			numEntry = GetIntValue(p + sizeof(ARP_ENTRY));
			if (numEntry <= 0)
			{
				fprintf(stderr, "Wrong entry in arp.conf: %s\n", p);
				exit(EXIT_FAILURE);
			}

			hasNumEntry = 1;
		}
		else
		{
			if (numEntry <= 0)
			{
				fprintf(stderr,
						"Error in arp.conf: more entries than "
						"are specifed, entry=%s\n",
						p);
				exit(EXIT_FAILURE);
			}
			printf("ARP : %s\n", p);
			EnrollARPTableEntry(p);
			numEntry--;
		}
	}

	fclose(fc);
	return 0;
}
/*----------------------------------------------------------------------------*/
static inline void
SaveInterfaceInfo(char *dev_name_list)
{
	strcpy(port_list, dev_name_list);
}
/*----------------------------------------------------------------------------*/
static inline void
SaveInterfaceStatList(char *dev_name_list)
{
	strcpy(port_stat_list, dev_name_list);
}
/*----------------------------------------------------------------------------*/
static int
ParseConfiguration(char *line)
{
	char optstr[MAX_OPTLINE_LEN];
	char *p, *q;

	char *saveptr;

	strncpy(optstr, line, MAX_OPTLINE_LEN - 1);
	optstr[MAX_OPTLINE_LEN - 1] = '\0';
	saveptr = NULL;

	p = strtok_r(optstr, " \t=", &saveptr);
	if (p == NULL)
	{
		printf("No option name found for the line: %s\n", line);
		return -1;
	}

	q = strtok_r(NULL, " \t=", &saveptr);
	if (q == NULL)
	{
		printf("No option value found for the line: %s\n", line);
		return -1;
	}
	if (strcmp(p, "num_cores") == 0)
	{
		CONFIG.num_cores = mystrtol(q, 10);
		if (CONFIG.num_cores <= 0)
		{
			printf("Number of cores should be larger than 0.\n");
			return -1;
		}
		if (CONFIG.num_cores > num_cpus)
		{
			printf("Number of cores should be smaller than "
				   "# physical CPU cores.\n");
			return -1;
		}
		num_cpus = CONFIG.num_cores;
	}
	else if (strcmp(p, "cc_number") == 0)
	{
		CONFIG.cc_number = mystrtol(q,10);
		cc_number = CONFIG.cc_number;

	}
	else if (strcmp(p, "ip") == 0)
	{
		memcpy(CONFIG.ip_addr, q, INET_ADDRSTRLEN);
	}
	else if (strcmp(p, "max_concurrency") == 0)
	{
		CONFIG.max_concurrency = mystrtol(q, 10);
		if (CONFIG.max_concurrency < 0)
		{
			printf("The maximum concurrency should be larger than 0.\n");
			return -1;
		}
	}
	else if (strcmp(p, "max_num_buffers") == 0)
	{
		CONFIG.max_num_buffers = mystrtol(q, 10);
		if (CONFIG.max_num_buffers < 0)
		{
			printf("The maximum # buffers should be larger than 0.\n");
			return -1;
		}
	}
	else if (strcmp(p, "rcvbuf") == 0)
	{
		CONFIG.rcvbuf_size = mystrtol(q, 10);
		if (CONFIG.rcvbuf_size < 64)
		{
			printf("Receive buffer size should be larger than 64.\n");
			return -1;
		}
	}
	else if (strcmp(p, "sndbuf") == 0)
	{
		CONFIG.sndbuf_size = mystrtol(q, 10);
		if (CONFIG.sndbuf_size < 64)
		{
			printf("Send buffer size should be larger than 64.\n");
			return -1;
		}
	}
	else if (strcmp(p, "tcp_timeout") == 0)
	{
		CONFIG.tcp_timeout = mystrtol(q, 10);
		if (CONFIG.tcp_timeout > 0)
		{
			CONFIG.tcp_timeout = SEC_TO_USEC(CONFIG.tcp_timeout) / TIME_TICK;
		}
	}
	else if (strcmp(p, "tcp_timewait") == 0)
	{
		CONFIG.tcp_timewait = mystrtol(q, 10);
		if (CONFIG.tcp_timewait > 0)
		{
			CONFIG.tcp_timewait = SEC_TO_USEC(CONFIG.tcp_timewait) / TIME_TICK;
		}
	}
	else if (strcmp(p, "port") == 0)
	{
		if (strncmp(q, ALL_STRING, sizeof(ALL_STRING)) == 0)
			SaveInterfaceInfo(q);
		else
			SaveInterfaceInfo(line + strlen(p) + 1);
	}
	else if (strcmp(p, "io") == 0)
	{
		SetIOEngine(q);
	}
	else if (strcmp(p, "drop_syn_nth") == 0)
	{
		CONFIG.drop_syn_nth = mystrtol(q, 10);
	}
	else if (strcmp(p, "drop_synack_nth") == 0)
	{
		CONFIG.drop_synack_nth = mystrtol(q, 10);
	}
	else if (strcmp(p, "drop_fin_nth") == 0)
	{
		CONFIG.drop_fin_nth = mystrtol(q, 10);
	}
	else if (strcmp(p, "drop_data_rate") == 0)
	{
		CONFIG.drop_data_rate = mystrtol(q, 10);
	}
	else if (strcmp(p, "drop_ack_rate") == 0)
	{
		CONFIG.drop_ack_rate = mystrtol(q, 10);
	}
	else if (strcmp(p, "drop_data_nth") == 0)
	{
		CONFIG.drop_data_nth = mystrtol(q, 10);
	}
	else if (strcmp(p, "loss_seed") == 0)
	{
		CONFIG.loss_seed = mystrtol(q, 10);
	}
	else
	{
		printf("Unknown option type: %s\n", line);
		return -1;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int LoadConfiguration(const char *fname)
{
	FILE *fp;
	char optstr[MAX_OPTLINE_LEN];

	printf("----------------------------------------------------------"
		   "-----------------------\n");
	printf("Loading etcp configuration from : %s\n", fname);

	fp = fopen(fname, "r");
	if (fp == NULL)
	{
		perror("fopen");
		printf("Failed to load configuration file: %s\n", fname);
		return -1;
	}
	while (1)
	{
		char *p;
		char *temp;

		if (fgets(optstr, MAX_OPTLINE_LEN, fp) == NULL)
			break;

		p = optstr;

		// skip comment
		if ((temp = strchr(p, '#')) != NULL)
			*temp = 0;
		// remove front and tailing spaces
		while (*p && isspace((int)*p))
			p++;
		temp = p + strlen(p) - 1;
		while (temp >= p && isspace((int)*temp))
			*temp = 0;
		if (*p == 0) /* nothing more to process? */
			continue;

		if (ParseConfiguration(p) < 0)
		{
			fclose(fp);
			return -1;
		}
	}

	fclose(fp);

	/* if rcvbuf is set but sndbuf is not, sndbuf = rcvbuf */
	if (CONFIG.sndbuf_size == -1 && CONFIG.rcvbuf_size != -1)
		CONFIG.sndbuf_size = CONFIG.rcvbuf_size;
	/* if sndbuf is set but rcvbuf is not, rcvbuf = sndbuf */
	if (CONFIG.rcvbuf_size == -1 && CONFIG.sndbuf_size != -1)
		CONFIG.rcvbuf_size = CONFIG.sndbuf_size;
	/* if sndbuf & rcvbuf are not set, rcvbuf = sndbuf = 8192 */
	if (CONFIG.rcvbuf_size == -1 && CONFIG.sndbuf_size == -1)
		CONFIG.sndbuf_size = CONFIG.rcvbuf_size = 8192;

	return ConfigNetworkEnv(port_list, port_stat_list);
}
/*----------------------------------------------------------------------------*/
void PrintConfiguration()
{

	printf("Configurations:\n");
	printf("Receive buffer size: %d\n", CONFIG.rcvbuf_size);
	printf("Send buffer size: %d\n", CONFIG.sndbuf_size);

	if (CONFIG.tcp_timeout > 0)
	{
		printf("TCP timeout seconds: %d\n",
			   USEC_TO_SEC(CONFIG.tcp_timeout * TIME_TICK));
	}
	else
	{
		printf("TCP timeout check disabled.\n");
	}
	printf("TCP timewait seconds: %d\n",
		   USEC_TO_SEC(CONFIG.tcp_timewait * TIME_TICK));
	printf("\n");
	printf("----------------------------------------------------------"
		   "-----------------------\n");
}
/*----------------------------------------------------------------------------*/
