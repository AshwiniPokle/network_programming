/**
* This program reads from a text file 'webpages.txt' that has one webpage URL per line.
* Each URL has a domain name and the path of file.
* It verfies
*	1. whether domain name exists or not yb finding corresponding IP.
*	2. whether that domain runs a webserver at port 80
* 3. whether page exists by sending a HTTP GET request
* Prints the results for each of the web pages to the screen
*/

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>

#define BUF_SIZE 256

int main()
{
	char *ptr,**pptr,url[256],address[256];
	char servername[100];int fd;
	char path[256]; int i;
	char str[INET_ADDRSTRLEN];
	struct hostent *hptr;

	FILE* fp = fopen("webpages.txt","r");
	while(fscanf(fp,"%s",url) == 1)
	//Part 1 :  check whether a domain exists
	{
		printf("\n\nurl : %s \n",url);
		for(i=0;url[i]!= '\0' && url[i]!='/';i++) {
			servername[i] = url[i];
		}

		servername[i] = '\0';
		int j=0;
		for(;url[i]!='\0';j++,i++) {
			path[j] = url[i];
		}
		path[j] = '\0';

		ptr=servername;
		if((hptr=gethostbyname(ptr))==NULL) {
			printf("Url does not exist!\n");
			continue;
		}

		strcpy(servername,hptr->h_name);
		switch(hptr->h_addrtype) {
			case AF_INET:
					pptr=hptr->h_addr_list;
					while(*pptr!=NULL){
					printf("IP address:%s\n",inet_ntop(hptr->h_addrtype,*pptr,str,sizeof(str)));
					pptr++;
					}
					break;
			default:
					break;

		}

		//Part 2: Check whether domain runs a webserver at port 80

		struct sockaddr_in serv_addr;
		memset(&serv_addr,0,sizeof(serv_addr));

		fd = socket(AF_INET, SOCK_STREAM, 0);
		if(fd == -1) {
			perror("socket() failed");
			continue;
		}
		serv_addr.sin_family =	AF_INET;
		serv_addr.sin_port = htons(80);
		serv_addr.sin_addr.s_addr = inet_addr(str);

		if(connect(fd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))<0) {
			perror("connect() failed");continue;
		}
		else printf("Connected at port 80!");

		//part 3: check whether page exists by sending HTTP GET request
		char reply[1024];
		char msg[270];
		strcpy(msg,"GET ");
		strcat(msg,path);
		char *car_return = "\r\n\r\n";
		strcat(msg,car_return);

		if(send(fd,msg,strlen(msg),0) < 0) {
			perror("Send Failed");
		continue;
	}
	if(recv(fd,reply,1000,0) < 0) {
		perror("recv failed");
		continue;
	}
	if(strstr(reply,"404 Not Found") == NULL) {
		printf("\nRequested page exists");
	}
	else
		printf("\nRequested page does not exist, 404 NOT Found");
	}
	fclose(fp);
	close(fd);
	exit(0);
}
