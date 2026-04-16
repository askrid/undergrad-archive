# shttpd

## Implementation Strategy

Single-threaded, event-driven HTTP server using epoll in edge-triggered mode with non-blocking I/O.

Each connection has a per-socket context struct (conn_t) that tracks its state, request buffer, parsed URL, response header, file descriptor, and send progress. Contexts are stored in an fd-indexed array for O(1) lookup from epoll events.

The connection state machine has two states: STATE_READ and STATE_SEND. In STATE_READ, the server accumulates the request in a buffer until \r\n\r\n is found, then parses the request line and headers. In STATE_SEND, it writes the response header via write() and the file body via sendfile() (zero-copy). Both reads and writes loop until EAGAIN since edge-triggered epoll requires draining all available data per event.

For concurrent connections, all client sockets are registered with epoll using EPOLLIN | EPOLLOUT | EPOLLET. The main loop calls epoll_wait and dispatches events to a unified handler (handle_conn) based on the connection's current state. This naturally handles any number of concurrent connections without threads or forking.

Persistent connections are supported by resetting the connection state after a complete response and looping back to STATE_READ within the same handler invocation. This also avoids the edge-triggered stall problem where buffered data would not trigger a new EPOLLIN event.

File delivery uses sendfile() with 1MB chunks to avoid resource exhaustion under concurrent load. File sizes use off_t to support files larger than 2GB.

## Testing

Tested with curl for all four connection modes (HTTP/1.0 ephemeral, HTTP/1.0 persistent, HTTP/1.1 ephemeral, HTTP/1.1 persistent), error responses (400, 404), 100MB file integrity via md5sum, and 5 concurrent connections.

Stress tested with flexiclient using single file workload (5 persistent connections, 10 seconds) and zipfian workload (5 persistent connections, 10 seconds). Both passed with no errors.

## Known Bugs

None known.

## Collaborators

None.
