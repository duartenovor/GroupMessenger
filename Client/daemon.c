
/*****************************************************************************/
/*** tcpclient.c                                                           ***/
/***                                                                       ***/
/*** Demonstrate an TCP client.                                            ***/
/*****************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>	
#include <mqueue.h>   /* mq_* functions */
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
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/time.h>

#include "../Parser/ucmd.h"

#define MSGQOBJ_D_C    "/d_c_queue"
#define MSGQOBJ_C_D    "/c_d_queue"

#define AFK_TIM		20		/* Online or AFK */

#define MAX_MSG_LEN 	10000 	/* max length of a message queue*/
#define BUFFERSIZE_128	128
#define BUFFERSIZE_256	256

/********************************************************************
 * 
 * State Constants
 * 
 *******************************************************************/

typedef enum { DEAD, ONLINE, AFK } state_t;
const char states[3][8] = {"DEAD","ALIVE","AFK"};


/********************************************************************
 * 
 * Parser
 * 
 *******************************************************************/

#define DELIMETER " "

Command_t my_cmd_list[];


/********************************************************************
 * 
 * Client Global Variables
 * 
 *******************************************************************/

static const char str_close[32] = "close";

state_t state;
char 	name[64];
//char 	msgobj_name[8];
mqd_t 	msgq_id_d_c;
mqd_t 	msgq_id_c_d;
int 	server_id;
int     client_id;

/********************************************************************
 * 
 * Device Driver Variables
 * 
 *******************************************************************/
  int fd0 = 0; 
  char ledOn = '1', ledOff = '0';

/********************************************************************
 * 
 * Prototypes
 * 
 *******************************************************************/

void close_queues();
static void sig_handler(int sig);
void timer(int s, int usec);
void *client_server(void *arg);
void *server_client(void *arg);

/****************************************************************************/
/*** This program opens a connection to a server using either a port or a ***/
/*** service.  Once open, it sends the message from the command line.     ***/
/*** some protocols (like HTTP) require a couple newlines at the end of   ***/
/*** the message.                                                         ***/
/*** Compile and try 'tcpclient lwn.net http "GET / HTTP/1.0" '.          ***/
/****************************************************************************/
int main(int argc, char *argv[])
{	
	/*** Deamon ***/
	pid_t pid, sid;
	int len, fd;

	pid = fork(); 			// create a new process

	if (pid < 0) 			// on error exit
	{ 
		syslog(LOG_ERR, "%s\n", "fork");
		exit(EXIT_FAILURE);
	}

	if (pid > 0)
	{  
		printf("Deamon PID: %d\n", pid);	
		exit(EXIT_SUCCESS); // parent process (exit)
	}
	
	sid = setsid(); 		// create a new session
	if (sid < 0) 			// on error exit
	{ 
		syslog(LOG_ERR, "%s\n", "setsid");
		exit(EXIT_FAILURE);
	}
	
	// make '/' the root directory
	if (chdir("/") < 0) 	// on error exit
	{ 
		syslog(LOG_ERR, "%s\n", "chdir");
		exit(EXIT_FAILURE);
	}
	umask(0);
	close(STDIN_FILENO);  // close standard input file descriptor
	close(STDOUT_FILENO); // close standard output file descriptor
	close(STDERR_FILENO); // close standard error file descriptor


	/*** TCP ***/
	struct hostent* host;
	struct sockaddr_in addr;
	int port;

	char msg[BUFFERSIZE_128];

	if ( argc < 3 )
	{
		snprintf(msg, sizeof(msg), "usage: %s <servername> <protocol or portnum>\n", argv[0]);
		syslog(LOG_ERR, "%s", msg);
		exit(0);
	}

	/*---Get server's IP and standard service connection--*/
	host = gethostbyname(argv[1]);
	//printf("Server %s has IP address = %s\n", args[1],inet_ntoa(*(long*)host->h_addr_list[0]));
	if ( !isdigit(argv[2][0]) )
	{
		struct servent *srv = getservbyname(argv[2], "tcp");
		if ( srv == NULL )
			syslog(LOG_ERR, "%s", argv[2]);

		snprintf(msg, sizeof(msg), "%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
		syslog(LOG_INFO, "%s", msg);
		port = srv->s_port;
	}
	else
		port = htons(atoi(argv[2]));

	/*---Create socket and connect to server---*/
	server_id = socket(PF_INET, SOCK_STREAM, 0);        /* create socket */
	if ( server_id < 0 )
		syslog(LOG_ERR, "%s\n", "socket");

	/*--------------- print server name and port ---------------*/
	snprintf(msg, sizeof(msg), "Connecting to server on %s:%d\n", host->h_name, atoi(argv[2]));
	syslog(LOG_INFO, "%s", msg);
	
	memset(&addr, 0, sizeof(addr));       /* create & zero struct */
	addr.sin_family = AF_INET;        /* select internet protocol */
	addr.sin_port = port;                       /* set the port # */
	addr.sin_addr.s_addr = *(long*)(host->h_addr_list[0]);  /* set the addr */

	/*---If connection successful---*/
	if ( connect(server_id, (struct sockaddr*)&addr, sizeof(addr)) == 0 )
	{
		// //-----------------Config Device Driver--------------------
		syslog(LOG_INFO, "Inserting Device Driver...\n");
    	system("insmod /etc/led.ko");

    	syslog(LOG_INFO, "Check devicer driver:\n");
    	system("lsmod");

		syslog(LOG_INFO, "Is the device driver in /dev:\n");
    	system("ls -l /dev/led0");

    	fd0 = open("/dev/led0", O_WRONLY);
   		system("echo none >/sys/class/leds/led0/trigger");
    	//-----------------End Device Driver--------------------

   		//Led ON
		write(fd0, &ledOn, 1);

		state = ONLINE;

		msgq_id_d_c = mq_open(MSGQOBJ_D_C, O_RDWR | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG, NULL);

    	if (msgq_id_d_c == (mqd_t)-1) {
        	syslog(LOG_ERR, "%s\n", "In mq_open[D->C]()");
        	mq_close(msgq_id_d_c);
        	exit(1);
    	}

    	msgq_id_c_d = mq_open(MSGQOBJ_C_D, O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK, S_IRWXU | S_IRWXG, NULL);

    	if (msgq_id_c_d == (mqd_t)-1) {
        	syslog(LOG_ERR, "%s\n", "In mq_open[C->D]()");
        	mq_close(msgq_id_c_d);
        	mq_close(msgq_id_d_c);
        	exit(1);
    	}

		gethostname(name, sizeof(name));
		send(server_id,name,strlen(name)+1,0);

		//Save id client server
		recv(server_id,msg,sizeof(msg),0);
		client_id = atoi(msg);

		snprintf(msg, sizeof(msg), "My name: %s Connected to server (server sockfd:%d)",name, server_id);
		mq_send(msgq_id_d_c, msg, strlen(msg)+1, 1);
		syslog(LOG_INFO, "%s\n", msg);

    	//control+c signal
		signal(SIGINT,sig_handler);
		
		pthread_t recv;
		pthread_t send;
		pthread_t stat;

		/* start thread */
		pthread_create(&recv, 0, client_server, &server_id);
		pthread_create(&send, 0, server_client, &server_id);

		timer(AFK_TIM, 0);

		pthread_join(recv, NULL); 
		pthread_join(send, NULL); 
	}
	else
		syslog(LOG_ERR, "%s\n", "panic");

	send(server_id,str_close,strlen(str_close)+1,0);
	shutdown(server_id,SHUT_RD);
	shutdown(server_id,SHUT_WR);
	shutdown(server_id,SHUT_RDWR);
	mq_close(msgq_id_d_c);
	if(mq_unlink(MSGQOBJ_D_C) == -1)
		syslog(LOG_ERR, "%s\n", "In mq_unlink");
	mq_close(msgq_id_c_d);
	if(mq_unlink(MSGQOBJ_C_D) == -1)
		syslog(LOG_ERR, "%s\n", "In mq_unlink");

	write(fd0, &ledOff, 1);
	sleep(2);
	close(fd0);
	
	syslog(LOG_ERR, "%s\n", "Closed");
			
	exit(EXIT_SUCCESS);		
}


/********************************************************************
 * 
 * Handler
 * 
 *******************************************************************/

static void sig_handler(int sig)
{
	switch(sig) 
	{
		case SIGALRM:
			state = AFK;
			break;
		case SIGTERM:
			state = DEAD;
			syslog(LOG_INFO,"Terminate signal catched");
			break;
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
 * Thread Functions
 * 
 *******************************************************************/
void *server_client(void *arg)                    
{	

	int sd = *(int*)arg;            /* get & convert the socket */
	char buffer[BUFFERSIZE_256];
	
	while(state)
	{
		if( recv(sd,buffer,sizeof(buffer),0) )
		{
			ucmd_parse(my_cmd_list, DELIMETER, buffer);
		}
	}
	syslog(LOG_INFO,"Close Connection_s_c");	                 
    /* terminate the thread */
}

void *client_server(void *arg)                    
{	
	int sd = *(int*)arg;            /* get & convert the socket */
	char buffer_rcv[MAX_MSG_LEN];
	char buffer_snd[BUFFERSIZE_128];
	
	int msgsz;
 	unsigned int sender;

	while(state)
	{
		if( mq_receive(msgq_id_c_d, buffer_rcv, MAX_MSG_LEN, NULL) == -1 )
		{
			// get error from errno
			int err = errno;

			// is the queue empty?
			if(err != EAGAIN) {
				// no more to read
				syslog(LOG_ERR, "mq_receive error");
				break;
			}
				
        	
		}
		else if(strcmp(buffer_rcv, "close") == 0)
		{
			snprintf(buffer_snd, BUFFERSIZE_128, "close %d %s", client_id,  buffer_rcv);
			send(sd,buffer_snd,strlen(buffer_snd)+1,0);
			state = DEAD;
		}
		else
		{

			snprintf(buffer_snd, BUFFERSIZE_128, "client %d %s", client_id,  buffer_rcv);
			send(sd,buffer_snd,strlen(buffer_snd)+1,0);
			/* client is online -> reset timer */
			state = ONLINE;
			timer(AFK_TIM, 0);
		}
		
	}
	
	syslog(LOG_INFO,"Close Connectionc_c_s");
}


/********************************************************************
 * 
 * Callbacks
 * 
 *******************************************************************/

void close_cb(char *user_argv)
{
	state = DEAD;
}

void multicast_cb(char *user_argv)
{
	mq_send(msgq_id_d_c, user_argv, strlen(user_argv)+1, 1);
	syslog(LOG_INFO, "%s", user_argv); 
}

void state_cb(char *user_argv)
{
	char format[BUFFERSIZE_128];
	snprintf(format, sizeof(format), "state %d %s", client_id, states[state]);
	send(server_id,format,strlen(format)+1,0);
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
		"multicast",
		multicast_cb
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
