# Project 1: SIMPLE/1.0 Client and Server

## Implementation Strategy

### sclient.c

Reads stdin into a buffer (up to 10 MB, capped by `MAX_CONT`), formats a `POST message SIMPLE/1.0` request with Host and Content-Length headers, and sends it over TCP. One malloc'd buffer (`MAX_HDR + MAX_CONT + 1` bytes) is shared between the outgoing request and the incoming response. The header goes at the front, content sits right after it, and the same region is reused to receive the server's reply.

On the response side: if the status line says `200 OK`, only the body is written to stdout. Otherwise the whole response goes out so you can see what went wrong.

- stdin beyond 10 MB is silently drained since the spec says to truncate.
- `SIGPIPE` is ignored; without this, writing to a closed socket kills the process instead of returning an error.
- Cleanup uses `goto` so there's one exit path for closing the socket and freeing memory.

### sserver.c

After bind/listen, the parent forks 5 children and then just sits in `pause()`. All 5 children share the same listening socket and block on `accept()`. The kernel wakes one of them when a connection comes in.

Per-connection flow:
1. `read_hdr()`: reads one byte at a time until `\r\n\r\n`. It may be slow, but it guarantees we don't accidentally consume body bytes. Buffered I/O (stdio) would help performance but introduces complexity around the header/body boundary, so I kept it simple.
2. `parse_header()`: walks the header with `strstr(p, "\r\n")` instead of `strtok_r` (which would split on `\r` and `\n` individually). Validates the request line allowing flexible whitespace, checks Host is non-empty, parses Content-Length. Unknown headers are skipped.
3. `readn()`: reads exactly Content-Length bytes for the body.
4. Sends back `200 OK` with the same body. Any parse failure returns `400 Bad Request`.

Signal handling:

- `SIGCHLD` set to `SIG_IGN` to auto-reap zombies
- `SIGPIPE` ignored same as the client.
- `SO_REUSEADDR` is on so restarting the server doesn't fail with "address already in use."

## Testing

Ran these on macOS (development) and Ubuntu 24.04 via Docker:

    # basic round-trip
    echo "hello world" | ./sclient -p 12345 -s localhost

    # concurrent: launches 10 clients in parallel, checks each gets its own message back
    bash test_concurrent.sh 12345 10 localhost

    # bad requests via nc
    printf "GET message SIMPLE/1.0\r\nHost: x\r\nContent-Length: 0\r\n\r\n" | nc localhost 12345
    printf "POST message SIMPLE/1.0\r\nContent-Length: 5\r\n\r\nhello" | nc localhost 12345
    # and so on...

Tested wrong method, missing headers, empty Host, corrupted Content-Length, etc. All return `400 Bad Request`. Also ran the reference client binary against my server on Ubuntu to check compatibility.

## Known Bugs

None.

## Collaborators

None.
