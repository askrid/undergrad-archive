#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cc_trace.h"
#include "tcp_in.h"   
#include "tcp_out.h"  
#include "tcp_flow.h"      

FILE *g_cc_log_fp = NULL;

int InitCCLogFile(const char *path)
{
    if (!path || path[0] == '\0') {
        return -1;
    }

    g_cc_log_fp = fopen(path, "w");
    if (!g_cc_log_fp) {
        perror("fopen CC log");
        return -1;
    }
    return 0;
}

void CloseCCLogFile(void)
{
    if (g_cc_log_fp) {
        fclose(g_cc_log_fp);
        g_cc_log_fp = NULL;
    }
}

void LogCCEvent(struct tcp_flow *flow,
                uint32_t ts_us,
                const char *event,
                uint32_t ack_seq,
                const char *extra)
{
    if (!g_cc_log_fp || !flow || !flow->sndvar || !flow->rcvvar) {
        return;
    }

    fprintf(g_cc_log_fp,
            "ts_us: %u,flow_id: %d,event: %s,TCP state: %s,ack_seq: %u,snd_una: %u,snd_nxt: %u,cwnd: %u,ssthresh: %u,peer_wnd: %u,dup_acks: %u,nrtx: %u,rto: %u,extra: %s\n",
            ts_us,
            flow->id,
            event ? event : "-",
            GetStateAsString(flow),
            ack_seq,
            flow->sndvar->snd_una,
            flow->snd_nxt,
            flow->sndvar->cwnd,
            flow->sndvar->ssthresh,
            flow->sndvar->peer_wnd,
            flow->rcvvar->dup_acks,
            flow->sndvar->nrtx,
            flow->sndvar->rto,
            extra ? extra : "-");

    fflush(g_cc_log_fp);
}