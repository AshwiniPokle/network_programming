/**
* Demonstrates use of unix domain protocols to pass file descriptor and a message
8 from parent to child processes. Parent process runs on a server, child processes are clients.
*/

#include <alloca.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef CMSG_DATA
#define CMSG_DATA(cmsg) ((cmsg)->cmsg_data)
#endif

	void childProcess(int sock) {
				char readbuf[256];
				char buf1[8],buf2[8];            /* space to read file name into */
	    	struct iovec vector[2];					/* range of bytes from parent */
	    	struct msghdr msg;							/* full message */
	    	struct cmsghdr * cmsg;      		/* control message with the fd */
	    	int fd;

	    	/* set up the iovec for the file name */
	    	vector[0].iov_base = buf1;
	    	vector[0].iov_len = 8;

				vector[1].iov_base = buf2;
	    	vector[1].iov_len = 8;
	    	/* the message we're expecting to receive */
	    	msg.msg_name = NULL;
	    	msg.msg_namelen = 0;
	    	msg.msg_iov = &vector;
	    	msg.msg_iovlen = 2;

	    	/* dynamically allocate so we can leave room for the file descriptor */
	    	cmsg = alloca(sizeof(struct cmsghdr) + sizeof(fd));
	    	cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(fd);
	    	msg.msg_control = cmsg;
	    	msg.msg_controllen = cmsg->cmsg_len;

	    	if (!recvmsg(sock, &msg, 0))
	    	    return;

	    	/* grab the file descriptor from the control structure */
	   		fd = *(int *)CMSG_DATA(cmsg);
	    	int lo = atoi(vector[0].iov_base);
				int hi = atoi(vector[1].iov_base);

				printf("\nchild (%d): ", getpid());
				if(lseek(fd,lo,SEEK_SET) == -1)
					perror("lseek");

				int cnt;
				if((cnt = read(fd,readbuf,hi-lo+1)) <= 0)
					perror("read from file");

				readbuf[cnt] = '\0';

				int i;
				for(i=0;i<cnt;i++) {
					if(isalpha(readbuf[i])){
						if(islower(readbuf[i]))
							printf("%c",toupper(readbuf[i]));
						else
							printf("%c",readbuf[i]);
						}
					else	printf("%c",readbuf[i]);
				}
				readbuf[0] = '\0';
				fflush(stdout);
	}

	void parentProcess(int sock, int fd,char lo[], char hi[]) {

				struct iovec vector[2];        /* range of bytes to pass w/ the fd */
				struct msghdr msg;             /* the complete message */
				struct cmsghdr *cmsg;         /* the control message, which will */
	                                    /* include the fd */
		/* Send the file name down the socket, including the trailing '\0' */

	      vector[0].iov_base = lo;
				vector[0].iov_len = 8;

				vector[1].iov_base = hi;
				vector[1].iov_len = 8;

		/* Put together the first part of the message. Include the
	       file name iovec */
				 msg.msg_name = NULL;
				 msg.msg_namelen = 0;
				 msg.msg_iov = &vector;
				 msg.msg_iovlen = 2;

		/* Now for the control message. We have to allocate room for
	       the file descriptor. */
	    	cmsg = alloca(sizeof(struct cmsghdr) + sizeof(fd));
	    	cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(fd);
	    	cmsg->cmsg_level = SOL_SOCKET;
	    	cmsg->cmsg_type = SCM_RIGHTS;

	  /* copy the file descriptor onto the end of the control
	       message */
				*(int *)CMSG_DATA(cmsg) = fd;

	    	msg.msg_control = cmsg;
	    	msg.msg_controllen = cmsg->cmsg_len;

	    	if (sendmsg(sock, &msg, 0) < 0)
	     		perror("sendmsg");

		return;
	}

	int main(int argc, char ** argv) {

		int socks[4][2];
		int status;

		if (argc != 2) {
	        fprintf(stderr, "file name not specified\n");
	        return 1;
		}

		//create socket pairs between each parent and child
		int i;
		for(i=0; i<4; i++) {
			if (socketpair(PF_UNIX, SOCK_STREAM, 0, socks[i]))
		        perror("Socket callfailed");

			if(!fork())	{ /* child */

						close(socks[i][0]);
						childProcess(socks[i][1]);
						exit(0);
			}
			else {
				    /* parent */
	    			close(socks[i][1]);
			}
		}

		char lo[4][8];
		char hi[4][8];
		int j;
		printf("\nSpecify range of characters to read for each of the 4 chidren : ");
		for(j=0; j<4; j++) {
			    printf("\nEnter two numbers lo and hi for process %d : ",j+1);
			    scanf("%s",lo[j]);
			    scanf("%s",hi[j]);
		}
		int fd;
		if ((fd = open(argv[1], O_RDONLY)) < 0) {
	     	  perror("open");
	        return;
		}
	 	for(i=0; i<4; i++) {
			    parentProcess(socks[i][0],fd,lo[i],hi[i]);
		    	wait(&status);
		}

		if (WEXITSTATUS(status)) {
	      	fprintf(stderr, "child failed\n");
	  }
		return 0;
}
