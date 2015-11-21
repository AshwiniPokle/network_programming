/**
* This programs tests a server and prints stats. It takes server_name, server_port, N, requests per connectons,
* and file_to_request from command line.
* It makes N number of concurrent connections to a server at a specified port.
* Each thread connects to a server, sends request_per_conn HTTP GET requests for file_to_request.
* After sending each request, it waits for the reply. For each request it notes the time when it sends a request
* and when it receives the reply. Response time is the difference betwene the two. Each thread computes global
* average response time and prints it. It also prints thread_id and request_no whenever it receives a reply from server.
**/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#define MAX_LINE 256
#define GET_CMD "GET %s HTTP/1.0\r\n\r\n"

struct file{
	char serv_name[256];
	int port;
	char *f_name;
	int req_per_con;
	pthread_t f_tid;
	int index;
} file[1];

//global arr to maintain time taken by  individual threads
float *t_arr;

pthread_mutex_t arr_mutex = PTHREAD_MUTEX_INITIALIZER;

// server_name, server_port, N (threads), req_per_con, file_to_request
void *thread_runner(void*);

int main(int argc, char* argv[]) {
	if(argc < 6){ perror("Specify arguments correctly"); exit(0);}

	int N = atoi(argv[3]);
	file[1].port = atoi(argv[2]);
	file[1].req_per_con = atoi(argv[4]);
	strcpy(file[1].serv_name,argv[1]);
	file[1].f_name= argv[5];

	t_arr = (float*)malloc(N * sizeof(float));

	int index;
	pthread_t tid;
	pthread_t *tid_arr = (pthread_t*) malloc(sizeof(pthread_t)*N);
	int n,i;
	int nconn = 0;
	while(nconn < N) {
		file[1].index = nconn;
		pthread_create(&tid, NULL, &thread_runner,&file[1]);
		pthread_join(tid,NULL);
		nconn++;
	}

	float avg;
	for(i=0; i<N; i++) {
		pthread_mutex_lock(&arr_mutex);
		avg += (t_arr[i]/N);
		pthread_mutex_unlock(&arr_mutex);
	}
	printf("response time (in ms): %f",avg);
	exit(0);
}
//how to pass serv_name, f_name etc
void* thread_runner(void* iptr) {
	struct file *fptr = (struct file*)iptr;
	struct timeval start,end;
	float response_time;
	int index = (int)fptr->index;
	struct sockaddr_in servaddr;
	memset(&servaddr,0,sizeof(servaddr));
	char line[MAX_LINE];
	char reply[1000];

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		perror("socket() failed");exit(0);
	}
	servaddr.sin_family =	AF_INET;
	servaddr.sin_port = htons(fptr->port);


	char *ptr,**pptr,url[50],address[100];
	char str[INET_ADDRSTRLEN];
	struct hostent *hptr;

	ptr=fptr->serv_name;
	if((hptr=gethostbyname(fptr->serv_name))==NULL) {
		perror("could not resolve host:");
		exit(0);
	}

	switch(hptr->h_addrtype) {
		case AF_INET:
					pptr=hptr->h_addr_list;
					for(;*pptr!=NULL;pptr++) {
					   strcpy(address,inet_ntop(hptr->h_addrtype,*pptr,str,sizeof(str)));
					}
					break;
		default:
					break;
	}

	if(inet_pton(AF_INET, str,&servaddr.sin_addr) <= 0) {
		perror("inet_pton error");exit(0);
	}
	fflush(stdout);

	if(connect(sockfd,(struct sockaddr *) &servaddr,sizeof(servaddr)) < 0) {
		perror("connect() failed"); exit(0);
	}

	pthread_t tid = pthread_self();
	int n = snprintf(line, sizeof(line), GET_CMD, fptr->f_name);

	puts(line);

	int num_req = 0;
	while(num_req < fptr->req_per_con) {
		gettimeofday(&start, NULL);
		if(send(sockfd,line,strlen(line),0) < 0) {
			perror("Send Failed");
			continue;
		}

		if(recv(sockfd,reply,1000,0) < 0) {
			perror("recv failed");
			continue;
		}
		else {
		   printf("Reply received! thread : %u request no : %d\n",tid+index,num_req);
	  }
		gettimeofday(&end, NULL);
		num_req++;

		response_time += (end.tv_sec - start.tv_sec) * 1000u + (end.tv_usec - start.tv_usec)/1000;
		fflush(stdout);
	}
	response_time/=fptr->req_per_con; 	//avg response time over iterations
	pthread_mutex_lock(&arr_mutex);
	t_arr[index] = response_time;
	pthread_mutex_unlock(&arr_mutex);
	close(sockfd);
	return NULL;
}
