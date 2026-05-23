SERVER_IP=192.168.0.71  # type ip address of ETCP server
PORT=8660  # type port number of ETCP server
CONCURRENCY=1 # type number of concurrent clients to the ETCP server (1 ~ 5)

./linux_client "$SERVER_IP" "$PORT" "$CONCURRENCY"
