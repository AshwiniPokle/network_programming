#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define BUF_SIZE 4096
int main(int argc, char *argv[])
{
	int clnt_fd;
	struct sockaddr_in serv_addr;
	
	char cmd[BUF_SIZE];
	char buf[BUF_SIZE];
	clnt_fd = socket(PF_INET, SOCK_STREAM, 0);
	if(clnt_fd == -1)
	{
		perror("socket() failed");
		exit(0);
	}
	// prepare for connect
	serv_addr.sin_family =	AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	// connect to server
	if(connect(clnt_fd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))<0)
	{
		perror("connect() failed");
		exit(0);
	}
	int size = BUF_SIZE;
	while (1) {
	printf("\n Enter a query (q to Exit) : \n ");
	gets(cmd);
	if(!strcmp(cmd,"q")) break;
	int n = write(clnt_fd, cmd, strlen(cmd)+1);
	n = read(clnt_fd, buf, BUF_SIZE);
	printf("\n Received Results : \n %s\n",buf);
	}
	close(clnt_fd);
	return 0;
}
