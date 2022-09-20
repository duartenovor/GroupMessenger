
/*****************************************************************************/
/*** tcpserver.c                                                           ***/
/***                                                                       ***/
/*** Demonstrate an TCP server.                                            ***/
/*****************************************************************************/

#include <ctype.h>	
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <resolv.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <fcntl.h>

#include "../Parser/ucmd.h"

void panic(char *msg);
#define panic(m)	{perror(m); abort();}


#define MAX_CLIENT_NUM	5
#define SEC 			5

#define MAX_MSG_LEN 	10000 	/* max length of a message queue*/
#define BUFFERSIZE_128	128
#define BUFFERSIZE_256	256

/********************************************************************
 * 
 * State Constants
 * 
 *******************************************************************/

typedef enum { DEAD, ALIVE } state_t;


/********************************************************************
 * 
 * Parser
 * 
 *******************************************************************/

#define DELIMETER " "

Command_t my_cmd_list[];


/********************************************************************
 * 
 * Server Global Variables
 * 
 *******************************************************************/

state_t server_state = ALIVE;

typedef struct client_socket_into client_socket_info_t;
struct client_socket_into
{
	int socket;
	int index;
	state_t state;
	char name[32];
}; 

client_socket_info_t socket_table[MAX_CLIENT_NUM];

int clients = 0;

/********************************************************************
 * 
 * Prototypes
 * 
 *******************************************************************/

static void sig_handler(int sig);
void timer(int s, int usec);
void *recvfunction(void *arg);
void *sendfunction(void *arg);


int main(int argc, char *argv[])
{	struct sockaddr_in addr;
	int listen_sd, port;

	if ( argc != 2 )	/* Program name + Port */
	{
		printf("usage: %s <protocol or portnum>\n", argv[0]);
		exit(0);
	}

	/*--------------Get server's IP and standard service connection--------------*/
	
	if ( !isdigit(argv[1][0]) )	/* if args[1] is not a digit */
	{
		/*
			struct servent {
	    		char  *s_name;     	official service name 
				char **s_aliases;   alias list 
	    		int    s_port;      port number
	    		char  *s_proto;     protocol to use 
			}
			getservbyname return a pointer to a statically allocated servent structure, 
			or a NULL pointer if an error occurs or the end of the file is reached. 
		*/

		struct servent *srv = getservbyname(argv[1], "tcp");

		if ( srv == NULL )
			panic(argv[1]);

		printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
		port = srv->s_port;
	}
	else
	{	
		/* convert an unsigned short int from host byte order to network byte order */
		port = htons(atoi(argv[1]));
	}

	/*-------------------------------- create socket -----------------------------*/

	/* 
	PF_INET -> IPv4 Internet protocols
	SOCK_STREAM -> Connection-based byte streams
	*/
	listen_sd = socket(PF_INET, SOCK_STREAM, 0);
	if ( listen_sd < 0 )
		panic("socket");

	int flags = fcntl(listen_sd, F_GETFL);	//"could not get flags on TCP listening socket");
	fcntl(listen_sd, F_SETFL, flags | O_NONBLOCK);

	/*------------------------- bind port/address to socket ----------------------*/

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = INADDR_ANY;                   /* any interface */
	if ( bind(listen_sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
		panic("bind");

	/*------------------------ make into listener with 10 slots -----------------*/
	if ( listen(listen_sd, MAX_CLIENT_NUM) != 0 )
		panic("listen")

	/*------------------------- begin waiting for connections -------------------*/
	else
	{	
		timer(SEC, 0);

		signal(SIGINT,sig_handler);

		char host_name[BUFFERSIZE_128];

		gethostname(host_name, sizeof(host_name));
		printf("Starting server on %s:%d\n", host_name, atoi(argv[1]));
		printf("Listening for incoming connections on socket:%d\n", listen_sd);

		pthread_t send_s;
		pthread_create(&send_s, 0, sendfunction, NULL);   
		pthread_detach(send_s); 
	
		while (server_state)                         /* process all incoming clients */
		{
			//int i=0;
			int n = sizeof(addr);
			int sd = accept(listen_sd, (struct sockaddr*)&addr, &n);
			
			if(sd == -1)    /* accept connection */
			{
				// get error from errno
				int err = errno;

				if(err != EAGAIN)
				{
					// no client to accept
					perror("client accept4");
		    		exit(1);
				}


			}

			if(sd != -1 && clients < MAX_CLIENT_NUM && server_state)
			{
				/* Waits for a message */
				recv(sd,socket_table[clients].name,sizeof(socket_table[clients].name),0);

				pthread_t recv;

				socket_table[clients].socket = sd;
				socket_table[clients].state = ALIVE;
				socket_table[clients].index = clients;

				//Send to client id number
				char aux[16];
				snprintf(aux,sizeof(aux),"%d",clients);
				
				send(sd, aux,strlen(aux)+1,0);
				
				printf("Client(%d) new connection (%d / %d)\n", 
					sd, clients, MAX_CLIENT_NUM);

				/* start thread */
				pthread_create(&recv, 0, recvfunction, &socket_table[clients]);
				clients++;


				/* don't track them */
				pthread_detach(recv); 
			}
			// else
			// 	printf("Cannot start a new connection\n");
		}
	}
}

/********************************************************************
 * Timer
 * 
 * sets a periodic alarm
 *******************************************************************/
void timer(int s, int usec)
{
	struct itimerval itv;

	// set handling of SIGALRM
	signal(SIGALRM,sig_handler);

	// period between successive timer interrupts
	itv.it_interval.tv_sec = s;
	itv.it_interval.tv_usec = usec;

	// period between now and the first timer interrupt
	itv.it_value.tv_sec = s;
	itv.it_value.tv_usec = usec;
	setitimer (ITIMER_REAL, &itv, NULL);
}

/********************************************************************
 * 
 * Handler
 * 
 *******************************************************************/

static void sig_handler(int sig)
{
	static const char broadcast[32] = "state";
	static const char str_close[32] = "close";

	unsigned int msgprio = 1;
	char msgcontent[MAX_MSG_LEN];
	int msgsz;
	unsigned int sender;
	char aux[8];

	switch(sig) 
	{
		case SIGALRM:
			for(int i = 0; i < MAX_CLIENT_NUM; i++)
			{
				if(socket_table[i].state)
				{
					send(socket_table[i].socket,broadcast,strlen(broadcast)+1,0);
				}
			}
			break;
		case SIGINT:
			for(int i = 0; i < MAX_CLIENT_NUM; i++)
			{
				if(socket_table[i].state)
				{
					send(socket_table[i].socket,str_close,strlen(str_close)+1,0);

					clients--;

					shutdown(socket_table[i].socket,SHUT_RD);
					shutdown(socket_table[i].socket,SHUT_WR);
					shutdown(socket_table[i].socket,SHUT_RDWR);
				}
			}
			printf("End of connection...");
			exit(0);
	}
}

/********************************************************************
 * 
 * Thread Functions
 * 
 *******************************************************************/
void *recvfunction(void *arg)                    
{	
	/* get & convert the socket */
	client_socket_info_t *info = (client_socket_info_t *)arg;
	char buffer[BUFFERSIZE_256];

	while(socket_table[info->index].state)
	{
		/* Waits for a message */
		if( recv(info->socket,buffer,sizeof(buffer),0) )
			ucmd_parse(my_cmd_list, DELIMETER, buffer);
	}

			                  			/* close the client's channel */
	return 0;                           /* terminate the thread */
}

void *sendfunction(void *arg)                    
{	
	/* get & convert the socket */
	client_socket_info_t *info = (client_socket_info_t *)arg;
	static const char str_close[32] = "close";
	
	char buffer[BUFFERSIZE_128];
	char format[BUFFERSIZE_256];

	while(server_state)
	{
		fgets(buffer, BUFFERSIZE_128 , stdin);
		buffer[strcspn(buffer, "\n")] = 0;			//remove newline

		/* end of connection */
		if( strcmp(buffer, "close") == 0 )
		{
			for(int i = 0; i < MAX_CLIENT_NUM; i++)
			{
				if(socket_table[i].state)
				{
					send(socket_table[i].socket,str_close,strlen(str_close)+1,0);

					socket_table[i].state = DEAD;	/* marked as a already closed channel*/
					clients--;

					shutdown(socket_table[i].socket,SHUT_RD);
					shutdown(socket_table[i].socket,SHUT_WR);
					shutdown(socket_table[i].socket,SHUT_RDWR);
					printf("Client(%d) connection closed rx\n", socket_table[i].socket);
				}
			}
			server_state = DEAD;
		}
		else
		{
			/* Broadcast */
			snprintf(format, sizeof(format), "multicast Server: %s", buffer);
			for(int i = 0; i < MAX_CLIENT_NUM; i++)
			{
				if(socket_table[i].state)
				{
					send(socket_table[i].socket,&format,strlen(format)+1,0);
				}
			}
		}
	}
			                  			/* close the client's channel */
	return 0;                           /* terminate the thread */
}


/********************************************************************
 * 
 * Callbacks
 * 
 *******************************************************************/

void close_cb(char *user_argv)
{
	char *client_number = strtok(user_argv, DELIMETER);
	int actual_client = atoi(user_argv);

	shutdown(socket_table[actual_client].socket,SHUT_RD);
	shutdown(socket_table[actual_client].socket,SHUT_WR);
	shutdown(socket_table[actual_client].socket,SHUT_RDWR);

	socket_table[actual_client].state = DEAD;	/* marked as a already closed channel*/
	clients--;

	printf("Client(%d) connection closed rx\n", socket_table[actual_client].socket);
}

void client_cb(char *user_argv)
{
	char *client_number = strtok(user_argv, DELIMETER);
	int actual_client = atoi(user_argv);

	user_argv = strtok(NULL, "\0");

	if(user_argv)
	{
		char format[BUFFERSIZE_256];
		snprintf(format, sizeof(format), "Client(%s)(%d) said: %s", socket_table[actual_client].name, socket_table[actual_client].index, user_argv);
		puts(format);

		snprintf(format, sizeof(format), "multicast Client(%s)(%d) said: %s", socket_table[actual_client].name, socket_table[actual_client].index, user_argv);

		/* Multicast */
		for(int i = 0; i < MAX_CLIENT_NUM; i++)
		{
			if(socket_table[i].state)
			 {
				send(socket_table[i].socket,&format,strlen(format)+1,0);
			 }
		}
	}
}

void state_cb(char *user_argv)
{
	/* NOTHING */
}


/********************************************************************
 * 
 * Command list
 * 
 *******************************************************************/

Command_t my_cmd_list[] = 
{
	{
		"close",
		close_cb
	},
	{
		"client",
		client_cb
	},
	{
		"state",
		state_cb
	},
	{
		"",
		NULL
	}
};