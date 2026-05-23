# type port number of eTCP server
# if you want to test between two eTCP stacks, type Debugging Port listed in spreadsheet 
PORT=8660
sudo /opt/etcp/bin/run_etcp_server.sh ${PORT} -f ETCP.conf 
