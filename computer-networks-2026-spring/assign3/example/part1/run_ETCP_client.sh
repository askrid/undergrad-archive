SERVER_IP=192.168.0.70 # type ip address of remote server
# type port number of remote server
# if you want to test between two eTCP stacks, type Debugging Port listed in spreadsheet 
PORT=8660
CONCURRENCY=1 # type number of concurrent clients to the remote server (1 ~ 5)
sudo /opt/etcp/bin/run_etcp_client.sh ${SERVER_IP} ${PORT} ${CONCURRENCY} -f ETCP.conf
