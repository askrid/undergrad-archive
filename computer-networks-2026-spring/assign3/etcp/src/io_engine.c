
/* for I/O module def'ns */
#include "io_engine.h"
/* for num_devices decl */
#include "config.h"
/* std lib funcs */
#include <stdlib.h>
/* std io funcs */
#include <stdio.h>
/* strcmp func etc. */
#include <string.h>
/* for ifreq struct */
#include <net/if.h>
/* for ioctl */
#include <sys/ioctl.h>
/* for inet_* */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* for getopt() */
#include <unistd.h>
/* for getifaddrs */
#include <sys/types.h>
#include <ifaddrs.h>
/* for file opening */
#include <sys/stat.h>
#include <fcntl.h>

#include "etcp.h"

io_engine *current_iomodule_func = &pcap_module_func;
/*----------------------------------------------------------------------------*/
#define ALL_STRING "all"
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
/*----------------------------------------------------------------------------*/

int ConfigNetworkEnv(char *dev_name_list, char *port_stat_list)
{
	int eidx = 0;
	int i, j;
	int set_all_inf = (strncmp(dev_name_list, ALL_STRING, sizeof(ALL_STRING)) == 0);
	CONFIG.eths = (struct eth_table *)
		calloc(16, sizeof(struct eth_table));
	if (!CONFIG.eths)
	{
		fprintf(stderr,"Can't allocate space for CONFIG.eths\n");
		exit(EXIT_FAILURE);
	}

	if (!CONFIG.eths)
	{
		fprintf(stderr,"Can't allocate space for CONFIG.eths\n");
		exit(EXIT_FAILURE);
	}
	struct ifaddrs *ifap;
	struct ifaddrs *iter_if;
	char *seek;
	if (getifaddrs(&ifap) != 0)
	{
		perror("getifaddrs: ");
		exit(EXIT_FAILURE);
	}
	iter_if = ifap;
	do
	{
		if (iter_if->ifa_addr && iter_if->ifa_addr->sa_family == AF_INET &&
			!set_all_inf &&
			(seek = strstr(dev_name_list, iter_if->ifa_name)) != NULL &&
			/* check if the interface was not aliased */
			*(seek + strlen(iter_if->ifa_name)) != ':')
		{
			struct ifreq ifr;
			/* Setting informations */
			eidx = CONFIG.eths_num++;
			strcpy(CONFIG.eths[eidx].dev_name, iter_if->ifa_name);
			strcpy(ifr.ifr_name, iter_if->ifa_name);

			/* Create socket for probing interface info */
			int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
			if (sock == -1)
			{
				perror("socket");
				exit(EXIT_FAILURE);
			}

			CONFIG.eths[eidx].ip_addr = inet_addr(CONFIG.ip_addr);
			
			if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0)
			{
				for (j = 0; j < 6; j++)
				{
					CONFIG.eths[eidx].haddr[j] = ifr.ifr_addr.sa_data[j];
				}
			}

			/* Net MASK */
			if (ioctl(sock, SIOCGIFNETMASK, &ifr) == 0)
			{
				struct in_addr sin = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
				CONFIG.eths[eidx].netmask = *(uint32_t *)&sin;
			}
			close(sock);

			for (j = 0; j < num_devices; j++)
			{

				CONFIG.eths[eidx].ifindex = j;
			}

			/* add to attached devices */
			for (j = 0; j < num_devices_attached; j++)
			{
				if (devices_attached[j] == CONFIG.eths[eidx].ifindex)
				{
					break;
				}
			}
			devices_attached[num_devices_attached] = CONFIG.eths[eidx].ifindex;
			num_devices_attached++;
			fprintf(stderr, "Total number of attached devices: %d\n",
					num_devices_attached);
			fprintf(stderr, "Interface name: %s\n",
					iter_if->ifa_name);
		}
		iter_if = iter_if->ifa_next;
	} while (iter_if != NULL);

	freeifaddrs(ifap);
	CONFIG.nif_to_eidx = (int *)calloc(16, sizeof(int));

	if (!CONFIG.nif_to_eidx)
	{
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < 16; ++i)
	{
		CONFIG.nif_to_eidx[i] = -1;
	}

	for (i = 0; i < CONFIG.eths_num; ++i)
	{

		j = CONFIG.eths[i].ifindex;
		if (j >= 16)
		{
			fprintf(stderr,"ifindex of eths_%d exceed the limit: %d\n", i, j);
			exit(EXIT_FAILURE);
		}
		/* the physic port index of the i-th port listed in the config file is j*/
		CONFIG.nif_to_eidx[j] = i;
		/* finally set the port stats option `on' */
		if (strstr(port_stat_list, CONFIG.eths[i].dev_name) != 0)
			CONFIG.eths[i].stat_print = TRUE;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
