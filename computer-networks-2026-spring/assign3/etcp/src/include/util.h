#ifndef UTIL_H
#define UTIL_H

#include "etcp.h"
#include "tcp_flow.h"

#define MAX(a, b) ((a)>(b)?(a):(b))
#define MIN(a, b) ((a)<(b)?(a):(b))
/* ============================================================
 * Checksum utility 
 * ============================================================ */
uint16_t
TCPCalcChecksum(uint16_t *buf, uint16_t len,
                uint32_t saddr, uint32_t daddr);
#endif