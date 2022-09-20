
/*****************************************************************************/
/*** tcpclient.c                                                           ***/
/***                                                                       ***/
/*** Demonstrate an TCP client.                                            ***/
/*****************************************************************************/

#include <ctype.h>
#include <errno.h>
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
#include <sys/types.h>
#include <sys/time.h>

#define MSGQOBJ_D_C    "/d_c_queue"
#define MSGQOBJ_C_D    "/c_d_queue"

void panic(char *msg);
#define panic(m)	{perror(m); abort();}


#define MAX_MSG_LEN 10000 	/* max length of a message queue*/


/****************************************************************************/
/*** This program opens a connection to a server using either a port or a ***/
/*** service.  Once open, it sends the message from the command line.     ***/
/*** some protocols (like HTTP) require a couple newlines at the end of   ***/
/*** the message.                                                         ***/
/*** Compile and try 'tcpclient lwn.net http "GET / HTTP/1.0" '.          ***/
/****************************************************************************/
int main(int argc, char *argv[])
{	

	mqd_t 	msgq_id_d_c;
	mqd_t 	msgq_id_c_d;
	unsigned int msgprio = 1;
	
	int msgsz;
	char buffer[MAX_MSG_LEN];

	msgq_id_d_c = mq_open(MSGQOBJ_D_C, O_RDWR | O_NONBLOCK);
	if (msgq_id_d_c == (mqd_t)-1) {
    	perror("In mq_open()");
    	mq_close(msgq_id_d_c);
    	exit(1);
	}

	msgq_id_c_d = mq_open(MSGQOBJ_C_D, O_RDWR);
	if (msgq_id_c_d == (mqd_t)-1) {
    	perror("In mq_open()");
    	mq_close(msgq_id_c_d);
    	mq_close(msgq_id_d_c);
    	exit(1);
	}

	while(1)
	{
		if( mq_receive(msgq_id_d_c, buffer, MAX_MSG_LEN, NULL) == -1)
		{
			// get error from errno
			int err = errno;

			// is the queue empty?
			if(err == EAGAIN)
				// no more to read
				break;

			perror("In mq_receive()");
	    	mq_close(msgq_id_d_c);
	    	exit(1);
		}

		// else, print queued message
		puts(buffer);
	}

	for(int i = 1; i < argc; i++)
		mq_send(msgq_id_c_d, argv[i], strlen(argv[i])+1, msgprio);

}