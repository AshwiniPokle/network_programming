#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define BUF_SIZE 4096
#define MAXPENDING 256

static void zombieReaper(int sig)
{
	while(waitpid(-1,NULL,WNOHANG) > 0)
		continue;
}

char wbuf[BUF_SIZE];
int size_wbuf = 0;
static void handleRequest(int cfd)
{
	char buf[BUF_SIZE];
	char query[BUF_SIZE];
	char filename[256];
	ssize_t numread;
	int size = BUF_SIZE;
	int i;
	int status;
	while(1)
	{
		if((numread = read(cfd,buf,size)) > 0)
		{
			buf[numread] = '\0'; //append null at end of buffer

			i=0;
			while(buf[i]!='\0' && buf[i] !='$')
			{	query[i] = buf[i];
				i++;
			}
			query[i] = '\0';
			i++;
			char c = buf[i];
			int j=0;
			if(c == 'N')
			{
				i+=2;
				if(buf[i] == '\0')
				{	perror("file name not specified!");exit(0);
				}
				while(buf[i]!='\0' && buf[i] != '$')
				{
					filename[j] = buf[i];
					i++;j++;
				}
				filename[j] = '\0';
			}
			int flag = 0;
			int status;
			int p[2];	//for pipe from child - stdout
			int fd[2];	//for pipe to child - stdin
			pipe(p);	//child to parent (execlp output)
			if(c == 'N')
			{
				int ret = fork();
				if(ret == 0)
				{
					dup2(p[1],1); //point to stdout
					close(p[1]);
					close(p[0]);
					execlp("grep","grep",query,filename,NULL);
				}
				else
				{
					close(p[1]);
				}
			}
			else
			{
				pipe(fd);		//parent to child (for stdin)
				int ret = fork();
				if(ret == 0)
				{
					dup2(p[1],1); //point to stdout
					close(p[1]);
					close(p[0]);

					//search on results of prev query

					dup2(fd[0],0); //point to stdin
					close(fd[1]);
					execlp("grep","grep",query,NULL);
				}
				else
				{	//Parent
					close(p[1]);
					close(fd[0]);
					wbuf[size_wbuf] = '\0';
					int num = write(fd[1],wbuf,size_wbuf);
					close(fd[1]);
				}
			}
			//Parent
			wait(&status);
			int numbytes = -1;
			numbytes  = read(p[0],wbuf,size);
			close(p[0]);
			if(numbytes == 0)
			{
				FILE *fp = fopen(filename,"r");
				if(!fp)
				{
					strcpy(wbuf,"FILE_NOT_FOUND");
					numbytes = strlen("FILE_NOT_FOUND");
				}
				else{
					strcpy(wbuf,"RECORD_NOT_FOUND");
					numbytes = strlen("RECORD_NOT_FOUND");
					fclose(fp);
				}
			}
			wbuf[numbytes] = '\0';
			size_wbuf = numbytes;
			write(cfd,wbuf,size);	//write to client fd
		}
	}
}

int main(int argc, char* argv[])
{
	int lfd,cfd;
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = zombieReaper;
	if(sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		printf("SIGCHILD signal");
		exit(0);
	}
	struct sockaddr_in serv_addr,clnt_addr;
	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if(lfd == -1){ perror("Socket Failed"); exit(0); }
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(lfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
	{
		perror("bind() failed");
		exit(0);
	}
	if(listen(lfd,MAXPENDING) < 0)
	{
		perror("listen() failed");
		exit(0);
	}
	socklen_t clen = sizeof(clnt_addr);
	int status = 0;
	for(;;)
	{
		if((cfd = accept(lfd,(struct sockaddr *)&clnt_addr,&clen)) < 0)
		{
			perror("accept() failed");
		}
		else{
		printf("Accepted client cfd : %d\n",cfd);
		switch(fork())
		{
			case -1:
				close(cfd);
				break;
			case 0:		//Child
				close(lfd);
				handleRequest(cfd);
				exit(1);
			default:	//Parent
				close(cfd);
				break;
		}
		}
	}
}
