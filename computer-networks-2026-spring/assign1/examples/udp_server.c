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
  int i, s, len, res, port;
  char buf[MAX_LINE];
  struct sockaddr_in saddr, claddr;
  socklen_t fromlen;

  if (argc < 2 || ((port = atoi(argv[1])) <= 0 || port > 65535)) {
    fprintf(stderr, "usage: %s port-num\n", argv[0]);
    return -1;
  }
   
  if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    fprintf(stderr, "socket() failed, errno=%d\n", errno);
    return -1;
  }
   
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(port);
  if (bind(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
      fprintf(stderr, "bind()) failed, errno=%d\n", errno);
      close(s);
      return -1;
   }
    
   while (1) {
     fromlen = sizeof(claddr); 
     if ((len = recvfrom(s, buf, sizeof(buf)-1, 0,
                         (struct sockaddr *)&claddr, &fromlen)) < 0) {
        fprintf(stderr, "recvfrom()) failed, errno=%d\n", errno);
        close(s);
        return -1;
    }
    printf("client addr:%s port:%d\n", inet_ntoa(claddr.sin_addr), ntohs(claddr.sin_port));
    
    buf[len] = 0;
    printf("received: %s", buf);
    for (i = 0; i < len; i++) {
        if (islower(buf[i]))
          buf[i] = toupper(buf[i]);
    }

    if ((res = sendto(s, buf, len, 0, 
                      (struct sockaddr *)&claddr, fromlen)) < 0) {
        fprintf(stderr, "sendto() failed, errno=%d\n", errno);
        close(s);
        return -1;
     }    
  }

  close(s);
  return(0);
}

