#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include "io_engine.h"
#include "etcp.h"
#include "config.h"

#define CHUNK_SIZE 8192
#define BUF_SIZE (128 * 1024 * 1024)
#define SNAPLEN 65535
#define TIMEOUT_MS 1
#define MAX_TX_BURST 128

/*----------------------------------------------------------------------------*/
struct fault_injection_state
{
    int syn_cnt;
    int synack_cnt;
    int fin_cnt;
    int ack_cnt;
    int data_cnt;
    unsigned int rand_seed;
};
/*----------------------------------------------------------------------------*/
struct tx_pkt
{
    uint8_t *buf;
    int len;
};
/*----------------------------------------------------------------------------*/
struct tx_buf
{
    struct tx_pkt pkts[MAX_TX_BURST];
    int cnt;
};
/*----------------------------------------------------------------------------*/
struct pcap_private_context
{
    pcap_t *handle;
    struct tx_buf w_buf;
    struct pcap_pkthdr hdr_copy[CHUNK_SIZE];
    struct pcap_pkthdr *hdrs[CHUNK_SIZE]; // this contains metatdata for each packet captured
    // metadata includes time it was captured and total length of this packet
    const u_char *pkts[CHUNK_SIZE];
    int num_pkts_in_chunk;

    int tx_sock;
    struct sockaddr_ll tx_addr;
    /* fault injection state */
    struct fault_injection_state *fi;

} __attribute__((aligned(__WORDSIZE)));
/*----------------------------------------------------------------------------*/
enum fi_pkt_type
{
    FI_PKT_NONE = 0,
    FI_PKT_SYN,
    FI_PKT_SYNACK,
    FI_PKT_FIN,
    FI_PKT_PURE_ACK,
    FI_PKT_DATA,
};
/*----------------------------------------------------------------------------*/
enum fi_action
{
    FI_ACT_SEND = 0,
    FI_ACT_DROP,
    FI_ACT_DUPLICATE, /* later */
    FI_ACT_HOLD,      /* later */
    FI_ACT_SEND_HELD, /* later */
};
/*----------------------------------------------------------------------------*/
static inline enum fi_pkt_type
ClassifyOutgoingPacket(const uint8_t *buf, uint16_t len)
{
    const struct ethhdr *eth;
    const struct iphdr *iph;
    const struct tcphdr *tcph;
    uint16_t ip_hlen, tcp_hlen, ip_tot_len, payloadlen;

    if (len < ETHERNET_HEADER_LEN + IP_HEADER_LEN + TCP_HEADER_LEN)
        return FI_PKT_NONE;

    eth = (const struct ethhdr *)buf;
    if (ntohs(eth->h_proto) != ETH_P_IP)
        return FI_PKT_NONE;

    iph = (const struct iphdr *)(buf + ETHERNET_HEADER_LEN);
    if (iph->protocol != IPPROTO_TCP)
        return FI_PKT_NONE;

    ip_hlen = iph->ihl << 2;
    if (len < ETHERNET_HEADER_LEN + ip_hlen + TCP_HEADER_LEN)
        return FI_PKT_NONE;

    tcph = (const struct tcphdr *)((const uint8_t *)iph + ip_hlen);
    tcp_hlen = tcph->doff << 2;
    ip_tot_len = ntohs(iph->tot_len);

    if (ip_tot_len < ip_hlen + tcp_hlen)
        return FI_PKT_NONE;

    payloadlen = ip_tot_len - ip_hlen - tcp_hlen;

    if (tcph->syn && tcph->ack)
        return FI_PKT_SYNACK;
    if (tcph->syn)
        return FI_PKT_SYN;
    if (tcph->fin)
        return FI_PKT_FIN;
    if (payloadlen > 0)
        return FI_PKT_DATA;
    if (tcph->ack)
        return FI_PKT_PURE_ACK;

    return FI_PKT_NONE;
}
/*----------------------------------------------------------------------------*/
static void
PrintCurrentTCPState(uint8_t *buf)
{
    const struct iphdr *iph;
    const struct tcphdr *tcph;
    iph = (const struct iphdr *)(buf + ETHERNET_HEADER_LEN);
    int ip_hlen = iph->ihl << 2;
    tcph = (const struct tcphdr *)((const uint8_t *)iph + ip_hlen);
    printf("Drop Situation: %s %s %s %s, seq=%u, ack=%u\n ",
           tcph->syn ? "SYN " : "",
           tcph->fin ? "FIN " : "",
           tcph->rst ? "RST " : "",
           tcph->ack ? "ACK " : "",
           ntohl(tcph->seq),
           ntohl(tcph->ack_seq));
}
/*----------------------------------------------------------------------------*/
static inline enum fi_action
DecideFiAction(struct fault_injection_state *fi, enum fi_pkt_type type)
{
    switch (type)
    {
    case FI_PKT_SYN:
        fi->syn_cnt++;
        if (CONFIG.drop_syn_nth > 0 &&
            fi->syn_cnt <= CONFIG.drop_syn_nth)
        {
            printf("[FI] dropping SYN #%d\n", fi->syn_cnt);
            return FI_ACT_DROP;
        }
        break;

    case FI_PKT_SYNACK:
        fi->synack_cnt++;
        if (CONFIG.drop_synack_nth > 0 &&
            fi->synack_cnt <= CONFIG.drop_synack_nth)
        {
            printf("[FI] dropping SYN-ACK #%d\n", fi->synack_cnt);
            return FI_ACT_DROP;
        }
        break;

    case FI_PKT_FIN:
        fi->fin_cnt++;
        if (CONFIG.drop_fin_nth > 0 &&
            fi->fin_cnt <= CONFIG.drop_fin_nth)
        {
            printf("[FI] dropping FIN #%d\n", fi->fin_cnt);
            return FI_ACT_DROP;
        }
        break;

    case FI_PKT_PURE_ACK:
        fi->ack_cnt++;
        if (CONFIG.drop_ack_rate > 0 &&
            (rand_r(&fi->rand_seed) % 100) < CONFIG.drop_ack_rate)
        {
            printf("[FI] dropping pure ACK (count=%d)\n", fi->ack_cnt);
            return FI_ACT_DROP;
        }
        break;

    case FI_PKT_DATA:
        fi->data_cnt++;
        if (CONFIG.drop_data_nth > 0 &&
            fi->data_cnt == CONFIG.drop_data_nth)
        {
            printf("[FI] dropping DATA nth=%d\n", fi->data_cnt);
            return FI_ACT_DROP;
        }
        
        if (CONFIG.drop_data_rate > 0 &&
            (rand_r(&fi->rand_seed) % 100) < CONFIG.drop_data_rate)
        {
            printf("[FI] dropping DATA (count=%d)\n", fi->data_cnt);
            return FI_ACT_DROP;
        }
        break;

    default:
        break;
    }

    return FI_ACT_SEND;
}
/*----------------------------------------------------------------------------*/
static int init_tx_sock(struct etcp_thread_context *ctxt)
{
    struct pcap_private_context *ppc =
        (struct pcap_private_context *)ctxt->io_private_context;
    int s;
    if ((s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) // send/receive raw socket including L2 header
    {
        perror("tx_sock initialziation failed");
        return -1;
    }
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, CONFIG.eths[0].dev_name);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("tx socekt ioctl failed");
        close(s);
        return -1;
    }
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_IP);

    if (bind(s, (struct sockaddr *)&sll, sizeof(sll)) < 0)
    {
        perror("tx_sock bind error");
        close(s);
        return -1;
    } // attaches socket to specific interface and specifc EtherType

    ppc->tx_addr = sll;
    ppc->tx_sock = s;

    /* fault injection state */
    ppc->fi = (struct fault_injection_state *)malloc(sizeof(struct fault_injection_state));
    ppc->fi->syn_cnt = 0;
    ppc->fi->synack_cnt = 0;
    ppc->fi->fin_cnt = 0;
    ppc->fi->ack_cnt = 0;
    ppc->fi->data_cnt = 0;
    ppc->fi->rand_seed = (unsigned int)CONFIG.loss_seed;

    return 0;
}
/*----------------------------------------------------------------------------*/
void pcap_init_handle(struct etcp_thread_context *ctxt)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_private_context *ppc;

    ctxt->io_private_context = calloc(1, sizeof(struct pcap_private_context));
    if (!ctxt->io_private_context)
    {
        fprintf(stderr, "Failed to allocate pcap_private_context\n");
        exit(EXIT_FAILURE);
    }
    ppc = (struct pcap_private_context *)ctxt->io_private_context;
    pcap_t *temp = pcap_create(CONFIG.eths[0].dev_name, errbuf);
    if (!temp)
    {
        fprintf(stderr, "pcap_create failed: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }
    if (init_tx_sock(ctxt) < 0)
    {
        fprintf(stderr, "error initialzing tx_sock ");
        exit(EXIT_FAILURE);
    }
    if (pcap_set_snaplen(temp, SNAPLEN) != 0)
        fprintf(stderr, "pcap_set_snaplen failed\n");
    if (pcap_set_promisc(temp, 0) != 0)
        fprintf(stderr, "pcap_set_promisc failed\n");
    if (pcap_set_timeout(temp, TIMEOUT_MS) != 0)
        fprintf(stderr, "pcap_set_timeout failed\n");
    if (pcap_set_buffer_size(temp, BUF_SIZE) != 0)
        fprintf(stderr, "pcap_set_buffer_size failed\n");
    if (pcap_setnonblock(temp, 0, errbuf) < 0)
        fprintf(stderr, "pcap_setnonblock failed: %s\n", errbuf);

    if (pcap_activate(temp) < 0)
    {
        fprintf(stderr, "pcap_activate failed: %s\n", pcap_geterr(temp));
        exit(EXIT_FAILURE);
    }
    ppc->handle = temp;

    struct bpf_program fp;
    struct ifreq ifr;
    struct in_addr my_ip_addr;
    char filter[128];
    char ip_str[INET_ADDRSTRLEN];
    int fd;

    my_ip_addr.s_addr = CONFIG.eths[0].ip_addr;
    if (inet_ntop(AF_INET, &my_ip_addr, ip_str, INET_ADDRSTRLEN) == NULL)
    {
        fprintf(stderr, "inet_ntop failed\n");
        exit(EXIT_FAILURE);
    }

    unsigned char *m;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, CONFIG.eths[0].dev_name);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ioctl(fd, SIOCGIFHWADDR, &ifr);
    close(fd);

    m = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    sprintf(filter,
            "(dst host %s or arp) and not ether src %02x:%02x:%02x:%02x:%02x:%02x",
            ip_str, m[0], m[1], m[2], m[3], m[4], m[5]);
    if (pcap_compile(ppc->handle, &fp, filter, 0, PCAP_NETMASK_UNKNOWN) == -1)
    {
        fprintf(stderr, "pcap_compile failed\n");
        exit(EXIT_FAILURE);
    }
    if (pcap_setfilter(ppc->handle, &fp) == -1)
    {
        fprintf(stderr, "pcap_setfilter failed\n");
        exit(EXIT_FAILURE);
    }
}
/*----------------------------------------------------------------------------*/
void pcap_release_pkt(struct etcp_thread_context *ctxt)
{
    struct pcap_private_context *ppc =
        (struct pcap_private_context *)ctxt->io_private_context;

    ppc->num_pkts_in_chunk = 0;
    return;
}
/*----------------------------------------------------------------------------*/
static int raw_send_batch(struct etcp_thread_context *ctxt)
{
    struct pcap_private_context *ppc = (struct pcap_private_context *)ctxt->io_private_context;
    int cnt = ppc->w_buf.cnt;
    struct mmsghdr msgs[cnt];
    struct iovec iov[cnt];
    int sent;
    int send_cnt = 0;

    if (cnt == 0)
        return 0;
    memset(msgs, 0, sizeof(msgs));
    memset(iov, 0, sizeof(iov));

    for (int i = 0; i < cnt; i++)
    {
        uint8_t *buf = ppc->w_buf.pkts[i].buf;
        uint16_t len = ppc->w_buf.pkts[i].len;
        enum fi_pkt_type type;
        enum fi_action action;

        if (buf == NULL)
            continue;

        type = ClassifyOutgoingPacket(buf, len);
        action = DecideFiAction(ppc->fi, type);
        switch (action)
        {
        case FI_ACT_DROP:
            /* drop and forget */
            PrintCurrentTCPState(buf);
            break;

        case FI_ACT_SEND:
        default:
            iov[send_cnt].iov_base = buf;
            iov[send_cnt].iov_len = len;

            msgs[send_cnt].msg_hdr.msg_iov = &iov[send_cnt];
            msgs[send_cnt].msg_hdr.msg_iovlen = 1;
            msgs[send_cnt].msg_hdr.msg_name = &ppc->tx_addr;
            msgs[send_cnt].msg_hdr.msg_namelen = sizeof(ppc->tx_addr);
            send_cnt++;
            break;
        }
    }

    sent = 0;
    if (send_cnt > 0)
    {
        sent = sendmmsg(ppc->tx_sock, msgs, send_cnt, 0);
        if (sent < 0)
        {
            perror("sendmmsg error");
            sent = 0;
        }
    }

    /* free only successfully sent buffers */
    for (int i = 0; i < cnt; i++)
    {
        if (ppc->w_buf.pkts[i].buf != NULL)
        {
            free(ppc->w_buf.pkts[i].buf);
            ppc->w_buf.pkts[i].buf = NULL;
        }
    }

    ppc->w_buf.cnt = 0;

    return sent;
}

/*----------------------------------------------------------------------------*/

int32_t
pcap_send_pkts(struct etcp_thread_context *ctxt)
{
    return raw_send_batch(ctxt);
}
/*----------------------------------------------------------------------------*/
// used when etcp wants to reserve memory for packet. get a buffer for outgoing packet
uint8_t *
pcap_get_wptr(struct etcp_thread_context *ctxt, uint16_t len)
{
    struct pcap_private_context *ppc =
        (struct pcap_private_context *)ctxt->io_private_context;

    if (ppc->w_buf.cnt == MAX_TX_BURST) // if count reached maximum capacity flush it first, then allocate new buffer
    {
        raw_send_batch(ctxt);
    }

    // allocate buffer
    uint8_t *buf = malloc(len);
    if (!buf)
        return NULL;

    ppc->w_buf.pkts[ppc->w_buf.cnt].buf = buf;
    ppc->w_buf.pkts[ppc->w_buf.cnt].len = len;
    ppc->w_buf.cnt++;

    return buf; // caller fills this buffer
}
/*----------------------------------------------------------------------------*/
uint8_t *
pcap_get_rptr(struct etcp_thread_context *ctxt, int index, uint16_t *len)
{
    struct pcap_private_context *ppc;
    uint8_t *pktbuf;
    ppc = (struct pcap_private_context *)ctxt->io_private_context;
    pktbuf = (uint8_t *)ppc->pkts[index];
    if (ppc->hdrs[index])
        *len = ppc->hdrs[index]->caplen;
    else
        *len = 0;
    return pktbuf;
}
/*----------------------------------------------------------------------------*/
static void
pcap_packet_handler(u_char *user,
                    const struct pcap_pkthdr *h,
                    const u_char *bytes)
{
    struct pcap_private_context *ppc =
        (struct pcap_private_context *)user;

    if (ppc->num_pkts_in_chunk < CHUNK_SIZE)
    {
        int idx = ppc->num_pkts_in_chunk;
        ppc->hdr_copy[idx] = *h;
        ppc->hdrs[idx] = &ppc->hdr_copy[idx];
        ppc->pkts[idx] = bytes;
        ppc->num_pkts_in_chunk++;
    }
}
/*----------------------------------------------------------------------------*/
int32_t
pcap_recv_pkts(struct etcp_thread_context *ctxt)
{
    struct pcap_private_context *ppc = ctxt->io_private_context;
    int ret = pcap_dispatch(ppc->handle, CHUNK_SIZE, pcap_packet_handler, (u_char *)ppc);
    // receive up to max CHUNK_SIZE packets
    // packets in socket's receive buffers are copied to user application buffer
    if (ret < 0) // error
    {
        fprintf(stderr, "pcap_dispatch failed: %s\n", pcap_geterr(ppc->handle));
        return -1;
    }
    return ppc->num_pkts_in_chunk;
}
/*----------------------------------------------------------------------------*/
void pcap_destroy_handle(struct etcp_thread_context *ctxt)
{
    struct pcap_private_context *ppc = ctxt->io_private_context;
    if (ppc)
    {
        if (ppc->handle)
            pcap_close(ppc->handle);

        // final leak check before free ppc
        for (int i = 0; i < ppc->w_buf.cnt; i++)
        {
            if (ppc->w_buf.pkts[i].buf)
            {
                free(ppc->w_buf.pkts[i].buf);
                ppc->w_buf.pkts[i].buf = NULL;
            }
        }
        ppc->w_buf.cnt = 0;

        free(ppc);
    }
}
/*----------------------------------------------------------------------------*/
void pcap_load_module() // useless,, just a guard
{
    if (CONFIG.multi_process)
    {
        exit(EXIT_FAILURE);
    }
}
/*----------------------------------------------------------------------------*/
io_engine pcap_module_func = {
    .load_module = pcap_load_module,
    .init_handle = pcap_init_handle,
    .release_pkt = pcap_release_pkt,
    .send_pkts = pcap_send_pkts,
    .get_wptr = pcap_get_wptr,
    .recv_pkts = pcap_recv_pkts,
    .get_rptr = pcap_get_rptr,
    .destroy_handle = pcap_destroy_handle,
    .dev_ioctl = NULL};
/*----------------------------------------------------------------------------*/