/**
* Pre-threaded chat server
* @author Ashwini
*/
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <pthread.h>
//global data structure to maintain data about clients
#define MAXPENDING 20
struct node;
struct node
{
	int tid;
	char tname[50];
	int fd;
	struct node* next;
};
typedef struct node* Clntnode;

Clntnode head = NULL;
Clntnode cur = NULL;
//auxiliary data structure to pass arguments while spawning threads
struct data
{
	int lfd;
	int thread_num;
}data[1];
//mutex on accept
pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ds_lock = PTHREAD_MUTEX_INITIALIZER;

static void *thread_runner(void *arg);
static void handle_client(int connfd, int tnum);

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		perror("Enter valid arguments - specify nthreads and server port number");
		exit(0);
	}
	int nthreads = atoi(argv[1]);
	int listenfd;
	pthread_t tid;
	struct sockaddr *cliaddr;
	struct sockaddr_in servaddr;
	listenfd = socket(AF_INET, SOCK_STREAM,0);
	if(listenfd == -1){ perror("Socket Failed"); exit(0); }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[2]));

	if(bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0)
	{
		perror("bind() failed");
		exit(0);
	}
	if(listen(listenfd,MAXPENDING) < 0)
	{
		perror("listen() failed");
		exit(0);
	}
	int i;
	printf("Spawning threads! %d \n",nthreads);
	/*spawn required number of threads */
	for(i=0;i<nthreads;i++)
	{
		data[1].lfd = listenfd;
		data[1].thread_num = i;
		pthread_create(&tid,NULL,&thread_runner,&data[1]);
		printf("thread created , tid : %u\n",tid);
	}
	for(;;)
		pause();
	return 0;
}
static void *thread_runner(void *arg)
{
	pthread_detach(pthread_self());
	//detach thread
	struct data *ptr = (struct data*)arg;
	int tnum = ptr->thread_num;
	int listenfd = ptr->lfd;
	int connfd;
	socklen_t clilen,addrlen;
	struct sockaddr *cliaddr;
	cliaddr = malloc(addrlen);
	while(1)
	{
		clilen = addrlen;
		pthread_mutex_lock(&accept_mutex);
		if((connfd = accept(listenfd, cliaddr, &clilen))<0)
		{
			perror("accept");
		}
		else
		 printf("\nconnection accepted! connfd : %d ",connfd);
		 pthread_mutex_unlock(&accept_mutex);
		 handle_client(connfd,tnum);
	}
}

static void handle_client(int connfd, int tnum)
{
	ssize_t n;
	char buf[256];
	char command[5];
	char tname[50];
	int k = 0;
	char nline[] = "\n";
	while((n=read(connfd,buf,256)) > 0)
	{
		buf[n] = '\0';
		printf("\n Current buf contents : ");
		puts(buf);
		strncpy(command,buf,4);
		command[4] = '\0';
		printf("\n command : %s ",command);
		if(!strcmp(command,"JOIN"))
		{
			int i,j;
			for(i=5,j=0; i < n;i++,j++)
				tname[j] = buf[i];
			tname[j-2] = '\0';
			pthread_mutex_lock(&ds_lock);
			if(head == NULL)
			{
				head = (struct node*)malloc(sizeof(struct node));
				head->tid = tnum;
				head->fd = connfd;
				strcpy(head->tname,tname);
				head->next = NULL;
			}
			else
			{	//insertion at head
				cur = (struct node*)malloc(sizeof(struct node));
				cur->tid = tnum;
				cur->fd = connfd;
				strcpy(cur->tname,tname);
				cur->next = head;
				head = cur;
			}
			pthread_mutex_unlock(&ds_lock);
			//printf("\nclient %s Joined! ",tname);
		}
		else if(!strcmp(command,"LIST"))
		{
			//printf("\n inside list");
			Clntnode ptr = head;
			char curname[50];
			pthread_mutex_lock(&ds_lock);
			while(ptr!=NULL)
			{
				write(connfd,ptr->tname,strlen(ptr->tname));
				write(connfd,nline,strlen(nline));
				ptr = ptr->next;
			}
			pthread_mutex_unlock(&ds_lock);
		}
		else if(!strcmp(command,"UMSG"))
		{
			//printf("\n inside unicast msg");
			Clntnode ptr = head;
			char name[50];
			char msg[256];
			int i,j;
			for(i=5,j=0; buf[i]!=' ';i++,j++)
				name[j] = buf[i];
			name[j] = '\0';
			for(j=0; buf[i] != '\0' ; i++,j++)
				msg[j] = buf[i];
			msg[j-2] = '\0';
 			printf("\nsending msg to name : %s msg : %s",name,msg);
			while(ptr != NULL)
			{
				//printf("\nname : %s (%d) tname : %s (%d) ",name, strlen(name),ptr->tname,strlen(ptr->tname));
				if(!strcmp(ptr->tname,name))
				{
					printf("\n found client! sending msg ");
					pthread_mutex_lock(&ds_lock);
					if(write(ptr->fd,msg,strlen(msg)) < 0)
						perror("write error");
					write(ptr->fd,nline,strlen(nline));
					pthread_mutex_unlock(&ds_lock);
					break;
				}
				ptr=ptr->next;
			}
			if(ptr == NULL)
				printf("\nCLnt not found!");
		}
		else if(!strcmp(command,"BMSG"))
		{
			//printf("\n inside bcast msg ");
			Clntnode ptr = head;
			char msg[256];
			int i,j;
			for(i=5,j=0; buf[i] != '\0'; i++,j++)
				msg[j] = buf[i];
			msg[j-2] = '\0';
			printf("\nbcast msg : %s ",msg);
			pthread_mutex_lock(&ds_lock);
			while(ptr != NULL)
			{
				if(ptr->fd != connfd)
				if(write(ptr->fd,msg,strlen(msg)) < 0)
					perror("write error");
				write(ptr->fd,nline,strlen(nline));
				ptr = ptr->next;
			}
			pthread_mutex_unlock(&ds_lock);
		}
		else if(!strcmp(command,"LEAV"))
		{
			if(head == NULL) return;
			Clntnode ptr = head;
			Clntnode prev = NULL;


			if(!strcmp(head->tname,tname))
			{

				Clntnode temp = head;
				pthread_mutex_lock(&ds_lock);
				head = head->next;
				pthread_mutex_unlock(&ds_lock);
				free(temp);
				printf("\nclient succesfully left");
			}
			else{
				ptr = head->next;
				prev = head;
				while(ptr != NULL)
				{
					if(!strcmp(tname,ptr->tname))
					{
						printf("\nclient succesfully left");
						Clntnode temp = ptr;
						pthread_mutex_lock(&ds_lock);
						prev->next = ptr->next;
						pthread_mutex_unlock(&ds_lock);
						free(temp);break;
					}
					prev = ptr;
					ptr = ptr->next;
				}
			}

			close(connfd);
			fflush(stdout);
			return;
		}
		else printf("\n command not supported!");
		fflush(stdout);
		buf[0] = '\0';
	}
}
