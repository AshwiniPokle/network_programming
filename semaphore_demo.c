/**
* Program that demonstrates use of semaphores to keep track of children.
* Process creates a semaphore and initializes it to n. It also creates n children
* where n is a command line argument. Each child reduces semaphore by 1, prints it's
* process id (pid) and exits. Parent wait until all children exit by waiting on semaphore.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define key (2011186)
#define key2 (2011)

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

int main (int argc, char *argv []) {
    int n, i;
    pid_t pid;
    n = atoi(argv[1]);

    if (n == 0) {
      error ("Invalid number of children");
    }
    int semid,semid2;
    struct sembuf sb,sb2;

    int retval;
    union semun setval,getval;
    setval.val = n;
    unsigned short val[1];

     semid = semget (key, 1, IPC_CREAT | 0666);   //semaphore to keep track of child processes
     semid2 = semget (key2, 1, IPC_CREAT | 0666); //for mutual exclusion between child processes
     if (semid >= 0) {
        //set sem value
        i = semctl (semid, 0, SETVAL, setval);
        if (i != -1) {
  	       perror ("semctl while setting sem value");
        }
        i = semctl (semid2, 0, SETVAL, 1);
        if (i != -1) {
  	       perror ("semctl while setting sem value");
        }
        //start forking n processes
        for (i = 0; i < n; ++i)v{
          if ((pid = fork()) < 0) {
            error("Can't fork process");
          }
          else if (pid == 0) { //child
  	         //acquire lock
            sb2.sem_num = 0;
            // Subtract 1 from semaphore value
            sb2.sem_op = -1;
            sb2.sem_flg = 0;
  	        retval = semop (semid2, &sb2, 1);

  	        //decrement count of semaphore

            sb.sem_num = 0;
            // Subtract 1 from semaphore value
            sb.sem_op = -1;
            sb.sem_flg = 0;
  	        retval = semop (semid, &sb, 1);
            if (retval == 0) {
  	           getval.array=val;
    	  	     semctl (semid, 0, GETALL, getval);
  	           printf("%d %d \n",getpid(), getval.array[0]);
  		        fflush(stdout);
  	        }

  	        //release lock
            sb2.sem_num = 0;
            sb2.sem_op = 1;
            sb2.sem_flg = 0;
  	        retval = semop (semid2, &sb2, 1);
            exit (0);
        }
      }

      pid_t wpid;
      int status;

      for(i=0;i<n;i++)
      wpid = wait(&status);
      printf("Parent exiting : pid %d\n",getpid());

      //remove semaphore
      semctl (semid,0,IPC_RMID,0);
      semctl (semid2,0,IPC_RMID,0);
    }
    return 0;
}
