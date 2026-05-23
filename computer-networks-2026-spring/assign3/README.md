# eTCP Assignment 3

## 1. Implementation Strategy

Three files modified: `etcp/src/tcp_in.c`, `etcp/src/tcp_out.c`, `etcp/src/timer.c`.

### Part 1: Connection Management

#### Passive open (server)

`OpenPassive` allocates a new flow with `CreateFlow(passive_open=TRUE)`. The flow starts in `TCP_ST_LISTEN`. I record the peer's initial sequence (`rcvvar->irs = seq`), set `rcv_nxt = seq`, and capture the peer's window from the SYN.

`LISTEN_Handler` rejects any non-SYN packet. On a valid SYN, it increments `rcv_nxt` (SYN consumes one sequence number) only when the current state is still `TCP_ST_LISTEN`, transitions to `TCP_ST_SYN_RCVD`, and registers the flow on the control list to schedule the SYN+ACK.

`SYN_RCVD_Handler` handles the final ACK that completes the three-way handshake. I update `sndvar->snd_una = ack_seq`, set `snd_nxt = ack_seq`, transition to `TCP_ST_ESTABLISHED`, and remove the SYN+ACK's retransmission timer entry.

#### Active open (client)

`OpenActive` initialises the flow from the SYN+ACK contents: peer's ISN, advanced `rcv_nxt`, `snd_nxt = ack_seq`, peer window, and `snd_wl1`/`last_ack_seq` bookkeeping fields used later by ACK processing.

`SYN_SENT_Handler` covers the active open response. For a normal SYN+ACK, I call `OpenActive`, reset `nrtx`, clear the RTO entry, transition to `TCP_ST_ESTABLISHED`, fire a write event, and schedule the final ACK on the control list. For a SYN with no ACK (simultaneous open), I move to `TCP_ST_SYN_RCVD` and reset `snd_nxt` to `iss` so the upcoming SYN+ACK uses the right sequence number.

#### Teardown

`ESTABLISHED_Handler` checks the FIN flag after payload and ACK handling. An in-order FIN (`seq + payloadlen == rcv_nxt`) advances `rcv_nxt` past the FIN's sequence number, transitions to `TCP_ST_CLOSE_WAIT`, schedules the ACK, and fires a read event so the application observes EOF. An out-of-order FIN only triggers an immediate ACK.

`FIN_WAIT_1_Handler` discards stale packets (seq behind `rcv_nxt`) with an ACK retransmission. On a valid ACK, it runs `HandleACK` to process any acknowledged payload. If the incoming ACK acknowledges our FIN (`ack_seq == fss + 1`), the flow moves to `TCP_ST_FIN_WAIT_2`, updates `snd_una` and `snd_nxt`, and clears the FIN's RTO entry.

`FIN_WAIT_2_Handler` transitions to `TCP_ST_TIME_WAIT` when an in-order peer FIN arrives. It registers the flow in the timewait list and schedules the final ACK.

#### Output path

`MakeTCPPacket` covers the control flag block: SYN/ACK bit setting, ack number from `rcv_nxt`, `ts_lastack_sent` and `last_active_ts` updates, advertised window (capped at 65535), `doff`, and the SYN/FIN-consumes-one-sequence-number rule with RTO arming.

`MakeControlPacket` maps the current TCP state to the correct control flags. `TCP_ST_SYN_SENT` emits a SYN. `TCP_ST_SYN_RCVD` resets `snd_nxt` to `iss` and emits SYN+ACK. `TCP_ST_ESTABLISHED`, `TCP_ST_CLOSE_WAIT`, and `TCP_ST_FIN_WAIT_2` emit a pure ACK. `TCP_ST_LAST_ACK` and `TCP_ST_FIN_WAIT_1` emit FIN+ACK, but defer if data or ACK packets are still queued, so that pending segments flush before the FIN sequence number is consumed.

#### Retransmission timeout

`ProcessRetransmissionTimeOut` doubles the RTO in pre-`TCP_ST_ESTABLISHED` states. The general retransmission section resets `snd_nxt` to `snd_una` so the next send pass restarts from the unacknowledged byte.

### Part 2: Reliable Data Transfer and Flow Control

#### Receive path

`HandlePayload` validates the segment's sequence range. It rejects anything entirely before `rcv_nxt` and anything beyond `rcv_nxt + rcv_wnd`. It inserts the payload with `InsertToReceiveBuffer`, advances `rcv_nxt` to `head_seq + merged_len`, and refreshes `rcv_wnd`. If the flow is in `FIN_WAIT_1` or `FIN_WAIT_2`, the data is immediately discarded since the local side is no longer reading. The function returns FALSE when `rcv_nxt` did not advance (out of order arrival), signalling the caller to send an immediate ACK. On contiguous progress in `TCP_ST_ESTABLISHED`, it raises a read event.

#### ACK processing

`HandleACK` runs in six stages.

First, peer window update: I follow the RFC 793 wl1/wl2 rule. The window updates only when the segment is newer (larger seq, same seq with larger ack, or both equal with a larger window). When the window reopens from zero, I fire a write event so a blocked sender wakes up.

Second, duplicate ACK detection: a snapshot of `peer_wnd` taken before the window update lets me check the spec definition (ack number does not advance, no payload, no meaningful new window).

Third, fast retransmit on the third duplicate ACK. I set `ssthresh = max(cwnd / 2, 2 * mss)`, inflate `cwnd = ssthresh + 3 * mss`, reset `snd_nxt = snd_una`, and put the flow on the data send list to retransmit immediately.

Fourth, additional duplicates during fast recovery inflate `cwnd` by one MSS each.

Fifth, congestion control on a newly advancing ACK. Fast recovery exit deflates `cwnd` to `ssthresh`. Slow start (`cwnd < ssthresh`) adds one MSS per ACK. Congestion avoidance adds `mss * mss / cwnd`, floored at one byte so cwnd still grows when very large.

Sixth, send buffer advancement. `RemoveFromSendBuffer(rmlen)` peels off acknowledged data, `snd_una` advances, `snd_wnd` refreshes from buffer free space, and a write event fires. `RenewRetransmissionTimer` then either clears the RTO entry or rearms it based on remaining in-flight data.

#### Send path

`MakeTCPPacket` payload block copies the payload bytes after the TCP header, advances `snd_nxt += payloadlen`, sets `ts_rto = cur_ts + rto`, and adds the flow to the RTO list.

`ProcessSendBuffer` derives the next unsent byte from `snd_nxt` and the send buffer offsets. The window calculation is `remaining_window = MIN(cwnd, peer_wnd) - (snd_nxt - snd_una)`, which combines flow control and congestion control in one expression.

#### RTO with RTT estimator

In `TCP_ST_ESTABLISHED` and later, RTO uses the smoothed RTT and variance from the Jacobson/Karels estimator. On each timeout I compute `base = (srtt >> 3) + rttvar` and shift it left by `min(nrtx, TCP_MAX_BACKOFF)`. When no RTT samples exist yet, I fall back to the current RTO value. Result is clamped at 60 seconds.

### Congestion Control Bonus

The implementation follows TCP Reno.

Slow start adds one MSS per advancing ACK while `cwnd < ssthresh`.

Congestion avoidance adds `mss * mss / cwnd` per ACK once `cwnd >= ssthresh`. This approximates one MSS per RTT.

Three duplicate ACKs trigger fast retransmit and fast recovery: ssthresh halves, cwnd inflates by three MSS, the missing segment retransmits immediately, and each additional duplicate ACK during recovery adds one MSS to cwnd. A new ACK exits recovery by deflating cwnd back to ssthresh.

Retransmission timeout treats loss as more severe: ssthresh halves, cwnd resets to one MSS, retransmission restarts from `snd_una`, and slow start resumes from the bottom.

All transitions log through `CC_LOG` so the trace file shows event type, ack number, snd_una, snd_nxt, cwnd, ssthresh, peer_wnd, dup_acks, nrtx, and rto.

## 2. Testing

I tested on the assigned machines across all combinations the environment supports.

### Part 1

Connection management was tested in both directions:

1. eTCP client on nw03 against linux_server on nw04.
2. eTCP server on nw03 against linux_client on nw04.
3. eTCP on both sides for direct two-stack debugging.

Concurrency was varied from 1 to 5 for each direction. All connections established, exchanged the application-level traffic (which the connection-management test ignores), and tore down cleanly.

#### Harsh conditions

I varied the fault-injection options across their full ranges:

- `drop_syn_nth` from 0 to 3.
- `drop_synack_nth` from 0 to 3.
- `drop_fin_nth` from 0 to 3.

All combinations with concurrency 1 to 5 were tested. Under packet loss the expected exponential RTO backoff and per-process drop counter saturation behaviour was observed. Connections still established and tore down correctly.

### Part 2

File transfer was tested with the four required file sizes:

- 1 KB
- 1 MB
- 10 MB
- 100 MB

For each size I ran:

1. eTCP client against linux_server.
2. eTCP server against linux_client.

Both directions completed with `diff` showing identical content against the reference file. Concurrency was tested from 1 to 5 for each scenario.

#### Harsh conditions

I tested with `drop_data_rate` and `drop_ack_rate` both set to 0, 2, 5, and 10, across concurrency 1 to 5. At each rate I confirmed:

- Transfer eventually completed.
- File content matched the reference.
- Congestion control trace showed the expected mix of fast retransmit, fast recovery, and retransmission timeout events depending on loss pattern.

I also induced deterministic three duplicate ACK scenarios by setting `drop_data_nth` to a specific position. The CC trace log showed:

- cwnd growing roughly one MSS per ACK in slow start.
- cwnd growing roughly one MSS per RTT in congestion avoidance.
- ssthresh halving and cwnd inflating on the third duplicate ACK.
- cwnd deflating to ssthresh on recovery exit.
- cwnd resetting to one MSS with ssthresh halved on retransmission timeout.

## 3. Known Bugs

None.

## 4. Collaborators

None.
