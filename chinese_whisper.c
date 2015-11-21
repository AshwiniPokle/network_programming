/**
* This is a fun simulation of chinese whisper game, albeit between processes,
* with inter-process communication being done through pipes. Total 6 processes
* are a part of this simulation ( 1 parent + 5 child processes). Each process changes
* string in some way (as it happens in the actual chinese whisper game)
**/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

int main (void) {
  int nrch, i;
  nrch = 5;

  char msg[256];
  pid_t pids[5];

  printf("\nEnter string : ");
  scanf("%s",msg);

  int fdold[2];
  int prev_in, parent_in, parent_out;

  if (pipe(fdold) == -1) {
    error("Can't create the pipe");
  }

  parent_in = fdold [0];
  parent_out = fdold [1];

  /* Start children. */
  prev_in = parent_in;
  for (i = 0; i < nrch; ++i) {
    int fd[2];
    if (pipe (fd) == -1) {
      error ("Can't create the pipe");
    }
    if ((pids[i] = fork()) < 0) {
      error("Can't fork process");
    }
    else if (pids[i] == 0) {
      int my_in, my_out;
      /* Child */
      close (fd [0]);     /* The sibling reads handle, this becomes prev_in
                             and is passed to the next sibling.  */
      close (parent_out); /* The parent writes handle, only the parent
                             wants to hold it, every child will close it.  */

      my_in = prev_in;
      my_out = fd [1];

      change_string(i, my_in, my_out,getpid());
      exit (0);
    }
    else
    {
      /* Parent.  */
      close (prev_in);   /* The PREV_IN was passed to the child we just
                          created, we no longer need this.  */
      prev_in = fd [0];  /* The read handle from the pipe we just made
                          will be passed to the next sibling.  */
      close (fd [1]);    /* The write handle from the pipe we just made
                          was grabbed by the child we just made, we no
                          longer need this.  */
    }
  }

  start(prev_in, fdold [1], msg,getpid());

  pid_t wpid;
  int status;
  wpid = wait(&status);

  return 0;
}

char* modify (char *word,int num) {
  int i;
  switch(num) {
    case 1:   for(i=0;word[i]!='\0';i++) {
                word[i] = toupper(word[i]);
              }
              break;
    case 2:
    case 4:   for(i=0;word[i+1]!= '\0';i++) {
                word[i] = word[i+1];
              }
              word[i] = '\0';
              break;
    case 3:
    case 5:   word[strlen(word)-1] = '\0';
              break;
  }
  return word;
}

void change_string(int id,int in,int out,pid_t pid) {
  char msg [256];

  read(in, msg, sizeof(msg));
  modify (msg, id+1);
  printf("c%d: %d %s\n",id+1,pid,msg);
  write(out, msg, sizeof(msg));
}

void start(int in,int out,char *msg,pid_t pid) {
  char buf [256];
  write(out, msg, strlen(msg)+1);

  read(in, buf, sizeof(buf));
  printf("P1: %d %s\n", pid,buf);
}
