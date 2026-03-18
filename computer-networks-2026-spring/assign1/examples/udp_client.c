#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

#define MAX_LINE 1024

int main(const int argc, const char** argv) 
{
  int s, port, len, res;
  char buf[MAX_LINE];
  struct hostent *hp;
  struct sockaddr_in saddr, raddr;
  socklen_t fromlen;

  if (argc < 3 ||  ((port = atoi(argv[2])) <= 0 || port > 65535)) {
    fprintf(stderr, "usage: %s server-domain-name port-num\n", argv[0]);
    return -1;
  }
  
  if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    fprintf(stderr, "socket() failed, errno=%d\n", errno);
    return -1;
  }

  if ((hp = gethostbyname(argv[1])) == NULL){
    fprintf(stderr, "gethostbyname() failed, domain-name:%s\n", argv[1]);
    close(s);
    return -1;
  }

  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  memcpy(&saddr.sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
  saddr.sin_port = htons(port);

  if (fgets(buf, sizeof(buf), stdin) == NULL)  {
    fprintf(stderr, "fgets() failed, errno=%d\n", errno);
    close(s);
    return -1;
  }
      
  len = strlen(buf);
  if ((res = sendto(s, buf, len, 0, 
                     (struct sockaddr *)&saddr, sizeof(saddr))) < 0){
    fprintf(stderr, "sendto() failed, errno=%d\n", errno);
    close(s);
    return -1;
  }

  fromlen = sizeof(raddr);
  if ((res = recvfrom(s, buf, sizeof(buf)-1, 0, 
                     (struct sockaddr *)&raddr, &fromlen)) < 0) {
    fprintf(stderr, "recvfrom()) failed, errno=%d\n", errno);
    close(s);
    return -1;
  }
   
  buf[res] = 0;
  printf("received: %s", buf);

  close(s);
  return 0;
}

