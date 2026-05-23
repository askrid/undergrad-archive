SERVER_IP=192.168.0.70  # type ip address of remote server
# type port number of remote server
# if you want to test between two eTCP stacks, use Debugging Port listed in spreadsheet 
PORT=8660 
CONCURRENCY=1 # type number of concurrent clients to the remote server (1 ~ 5)
FILENAME=1mb.txt  # type name of file you want to receive from remote server
sudo /opt/etcp/bin/run_etcp_client.sh ${SERVER_IP}/${FILENAME} ${PORT} ${CONCURRENCY} -f ETCP.conf -o output
