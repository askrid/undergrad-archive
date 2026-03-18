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
#include <signal.h>

#define MAX_LINE 1024

/*--------------------------------------------------------------------*/
void
ignore_signal(int sig)
{
    struct sigaction act;
    
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;    
    if (sigaction(sig, &act, NULL) < 0) {
      perror("sigaction");
      exit(-1);
    }

}
/*--------------------------------------------------------------------*/
void 
MainLoop(int s)
{
  int i, c, res, len, off;
  char buf[MAX_LINE];

  while ((c = accept(s, NULL, NULL)) >= 0) {

   while (1) {
      /* read the message from the client */
      if ((res = read(c,  buf,  sizeof(buf) - 1)) <= 0) {
          if (res == 0) printf("connection closed\n");
          if (res < 0) {
              if (errno == EINTR) continue;
              else perror("read");
          }
          break;
      }

      /* main logic - convert lowercase to uppercase */
      len = res;
      for (i = 0; i < len; i++) {
          if (islower(buf[i]))
            buf[i] = toupper(buf[i]);
      }

      off = 0;
      while (off < len) { 
        /* write may not send the entire message at one time 
           so, call write() repeatedly until the entire
           message is sent
        */
        if ((res = write(c, buf+off, len-off)) <= 0) {
            perror("write");       
            break;
        }
        off += res;
      }      
   }      
   /* this connection is done, so close it */
   close(c);
  }
}
/*--------------------------------------------------------------------*/
int 
main(const int argc, const char** argv) 
{
  int i, s, c, len, res, port;
  char buf[MAX_LINE];
  struct sockaddr_in saddr;
    
  if (argc < 2 ||  ((port = atoi(argv[1])) < 1024 || port > 65535)) {
    fprintf(stderr, "usage: %s port(1024-65535)\n", argv[0]);
    return -1;    
  }    
  
  if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    fprintf(stderr, "socket() failed, errno=%d\n", errno);
    return -1;    
  }    
     
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(port);
  if (bind(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    fprintf(stderr, "bind() failed, errno=%d\n", errno);
    close(s);
    return -1;    
  }   
    
  if (listen(s, 1024) < 0) {
    fprintf(stderr, "listen() failed, errno=%d\n", errno);
    close(s);
    return -1;    
  }   
  
  while ((c = accept(s, NULL, NULL)) >= 0) {

   if ((len = read(c,  buf,  sizeof(buf) -1)) <= 0) {
      if (len < 0)
        fprintf(stderr, "read() failed, errno=%d\n", errno);
      close(s);      
      continue;
   }      

   buf[len] = 0;
   for (i = 0; i < len; i++) {
      if (islower(buf[i]))
        buf[i] = toupper(buf[i]);
    }

    if ((res = write(c, buf, len)) < 0) {
      fprintf(stderr, "write() failed, errno=%d\n", errno);
      close(s); close(c);
      return (-1);      
     }      
     close(c);
  }
  
  close(s);
  return(0);
}
        