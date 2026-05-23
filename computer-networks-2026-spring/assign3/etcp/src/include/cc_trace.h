#ifndef CC_TRACE_H
#define CC_TRACE_H

#include <stdio.h>
#include <stdint.h>

struct tcp_flow;

extern FILE *g_cc_log_fp;

int InitCCLogFile(const char *path);
void CloseCCLogFile(void);

void LogCCEvent(struct tcp_flow *flow,
                uint32_t ts_us,
                const char *event,
                uint32_t ack_seq,
                const char *extra);

#define CC_LOG(flow, ts, event, ack_seq, extra) \
    do { \
        if (g_cc_log_fp) { \
            LogCCEvent((flow), (ts), (event), (ack_seq), (extra)); \
        } \
    } while (0)

#endif