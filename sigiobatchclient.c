/**
* This is a batchmode client, that keeps sending requests to server without waiting
* for reply. Whenever a reply comes, it reads it and receives SIGRTMIN + 1 signal.
* In this particular implementation, metadata of the webpages, present in the file
* being passed from commandline, is being fetched from the server.
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <string.h>

#define LISTENQ 15
#define MAXLINE 512

char buf[MAXLINE];

int count = 0;
int fd;
int i = 0;
static void sighandler (int sig, siginfo_t * si, void *ucontext) {
  fflush (stdout);S
  if (si->si_code == POLL_IN) {
    int n = read (si->si_fd, buf, MAXLINE);
    if (n == 0 ) {
      close (si->si_fd);
    }
    else if (n > 0) {
      i++;
      buf[n] = '\0';
      printf ("Data from connfd %d:len %d: \n", si->si_fd,n); puts(buf);
      int k;
      if (write (si->si_fd, buf, n) < 0)  {
        perror("write");
      }
    } else {
      printf ("Socket %d error\n", si->si_fd);
      perror ("socket");
    }
  }
  if (si->si_code == POLL_ERR)
  {
    int err;
    int errlen = sizeof (int);
    getsockopt (si->si_fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    if (err > 0)
    printf ("error on socket %d : %s", si->si_fd, strerror (err));
  }
  fflush (stdout);
}


// client
int main (int argc, char **argv)
{
  char msg[270];
  char path[256];
  FILE* fp = fopen(argv[1],"r");
  char buf[MAXLINE];
  socklen_t clilen;
  struct sockaddr_in cliaddr, servaddr;
  struct sigaction sa;
  memset (&sa, '\0', sizeof (sa));

  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = &sighandler;	//for accepting new conn
  sigaction (SIGIO, &sa, NULL);
  sigaction (SIGRTMIN + 1, &sa, NULL);
  while(fscanf(fp,"%s",path)==1) {
    count++;
    fd = socket (AF_INET, SOCK_STREAM, 0);
    bzero (&servaddr, sizeof (servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("172.24.2.36");
    servaddr.sin_port = htons (80);

    while(connect(fd,(struct sockaddr *) &servaddr,sizeof(servaddr)) < 0) {
      sleep(1);
    }

    fcntl (fd, F_SETOWN, getpid ());
    int flags = fcntl (fd, F_GETFL);	// Get current flags
    fcntl (fd, F_SETFL, flags | O_ASYNC | O_NONBLOCK);	//set signal driven IO
    fcntl (fd, F_SETSIG, SIGRTMIN + 1);	//replace SIGIO with realtime signal

    strcpy(msg,"HEAD ");
    strcat(msg,path);
    char *car_return = " HTTP/1.0\r\n\r\n";
    strcat(msg,car_return);
    if(send(fd,msg,strlen(msg),0) < 0) {
      perror("Send Failed");
      continue;
    }
  }

  for(;;) {
    pause();
    if(i == count) exit(0);
  }
}
