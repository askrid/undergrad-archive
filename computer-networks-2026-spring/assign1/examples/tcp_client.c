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

#define MAX_LINE 1024
int main(const int argc, const char** argv) 
{
  int s, port, len, res;
  char buf[MAX_LINE];  
  struct hostent *hp;
  struct sockaddr_in saddr;
    
  if (argc < 3 ||  ((port = atoi(argv[2])) < 1024 || port > 65535)) {
    fprintf(stderr, "usage: %s domain-name port(1024-65535)\n", argv[0]);
    return -1;    
  }    

  /* create a socket for connection */
  if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      perror("socket() failed");
      return -1;     
  }
  
  /* domain name lookup */
  if ((hp = gethostbyname(argv[1])) == NULL) {
     fprintf(stderr, "gethostbyname failed: %s\n", strerror(errno));
     exit(-1);
  }

  /* connect to one address returned by gethostbyname() */
  saddr.sin_family = AF_INET;
  memcpy(&saddr.sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
  saddr.sin_port = htons(port);
  if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    perror("connect()");
    exit(-1);      
  }
  
  /* this works only when the input size is small */
  if (fgets(buf, sizeof(buf), stdin) == NULL) {
    fprintf(stderr, "fgets() failed\n");
    close(s);
    return -1;
  }
    
  len = strlen(buf);
  if ((res = write(s, buf, len)) <= 0) {
      fprintf(stderr, "write() failed, red=%d errno=%d\n", res, errno);
      close(s);
      return -1;
  }

  if ((res = read(s, buf, sizeof(buf)-1)) <= 0) {
    fprintf(stderr, "read() failed, red=%d errno=%d\n", res, errno);
    close(s);
    return -1;
  }

  buf[res] = 0;
  printf("received %s", buf);

  close(s);  
  return 0;
}
        