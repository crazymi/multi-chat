#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include "talk.h"

#define MAX_ClIENT_NUMBER 4

// global variable
int num_of_connection = 0;
int fdlist[4];
int invlist[4];
int current_line = 0;
pthread_mutex_t log_mutex;

void *slave(void *vargp)
{
	FILE *fp;

	int last_line = -1;
	int invflag = -1;
	int qflag = 0;

	char buf[MAXBUF];
	char msgbuf[MAXBUF];

	memset(buf, 0, sizeof(char)*MAXBUF);
	memset(msgbuf, 0, sizeof(char)*MAXBUF);

	pthread_t tid = pthread_self();
	pthread_detach(tid);

	arginfo argp = *(arginfo*)vargp;
	free(vargp);

	printf("Current num: %d, serving from %d\n", argp.id, argp.fd);

	
	while(read(argp.fd, buf, MAXBUF) != 0)
	{
		// invite protocol
		if(strncmp(buf, "@invite", 7) == 0)
		{
			printf("invite command\n");

			invflag = talk_login(buf+8);
			// recover \n char
			buf[strlen(buf)] = '\n';
			if(invflag == -1)
			{
				printf("fail to invite: %s\n", buf+8);
				memset(buf, 0 , sizeof(char)*MAXBUF);
				continue;
			}
			else 
			{
				// if already exist in room
				if(invlist[invflag] == 1)
				{
					memset(buf, 0 , sizeof(char)*MAXBUF);
					continue;
				}					
				invlist[invflag] = 1;
				printf("Success to invite: %s[%d]\n", buf+8, invflag);
			}
		}

		// quit protocol
		if(strncmp(buf, "@quit", 5) == 0)
		{
			printf("[%d] quit the chat room\n", argp.id);
			invlist[argp.id] = 0;		
			qflag = 1;	
			break;
		}

		printf("talk_write call\n");
		last_line = talk_write(argp.id, buf, &log_mutex);
		
		sprintf(msgbuf, "[%s]: %s", argp.nickname, buf);
		printf("%d...%s", argp.fd, msgbuf);

		// send message to all connected client
		for(int i=0;i<4;i++)
		{
			printf("%d ", fdlist[i]);
			if(fdlist[i] != -1)
			{
				write(fdlist[i], msgbuf, sizeof(msgbuf));
				// also update last readed line
				talk_logout(i, last_line);
			}
		}

		memset(buf, 0, sizeof(char)*MAXBUF);
		memset(msgbuf, 0, sizeof(char)*MAXBUF);
	}

	close(argp.fd);

	num_of_connection--;
	printf("%d client close\n", argp.fd);

	// if quit delete id.talk
	if(qflag == 1)
	{
		memset(buf, 0, sizeof(char)*MAXBUF);
		sprintf(buf, "%d.talk", argp.id);
		if((fp = fopen(buf, "r")) != NULL)
		{
			printf("exists\n");
			fclose(fp);
			remove(buf);
		}
	}

	fdlist[argp.id] = -1;
	return NULL;
}

int main (int argc, char **argv) {
	char buf[MAXBUF];

	int flag;

	int sock_fd_server = 0;
	int sock_fd_client = 0;

	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;

	arginfo *vargp;
	int client_addr_size;

	pthread_t tid;

	// initialize
	for(int i=0;i<4;i++)
		fdlist[i] = -1;
	for(int i=0;i<4;i++)
		invlist[i] = 0;
	// at first, only A is in chat room
	invlist[0] = 1;

	pthread_mutex_init(&log_mutex, NULL);

	pthread_mutex_lock(&log_mutex);
	// define server socket	
	sock_fd_server = socket(PF_INET, SOCK_STREAM, 0);
	memset(&server_addr, 0, sizeof(struct sockaddr_in));

	// TCP/IP family
	server_addr.sin_family = AF_INET;
	// open to any address
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// specify port number in talk.h
	server_addr.sin_port = htons(PORT);

	pthread_mutex_unlock(&log_mutex);

	// bind addresss to sever socket
	if(bind(sock_fd_server, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in)) == -1){
		printf("unix error while binding\n");
		return 1;
	}
	// set server as listen state
	listen(sock_fd_server, 5);
 
	while(1){
		memset(buf, 0, sizeof(char)*MAXBUF);
		client_addr_size = sizeof(struct sockaddr_in);
		sock_fd_client = accept(sock_fd_server, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_size);

		read(sock_fd_client, buf, MAXBUF);
		printf("[%d]Get %s\n", sock_fd_client, buf);
		fflush(stdout);

		// login parser, run at initial accept
		if(buf[0] == 'l' && buf[1] == '*'){
			// remove protocol chars "l*"
			memmove(buf, buf+2, strlen(buf)-2);
			buf[strlen(buf)-2] = '\0';
			
			// flag contains id, if login success
			flag = talk_login(buf);

			// if not in user list = wrong id
			if(flag == -1){
				printf("[%d]Wrong id\n", sock_fd_client);
				write(sock_fd_client, "", MAXBUF);	
				continue;
			}

			// if not in chat room = not invited yet
			if(invlist[flag] == 0)
			{
				printf("[%d]Not allowed\n", sock_fd_client);
				write(sock_fd_client, "", MAXBUF);	
				continue;
			}

			// if duplicate login try
			if(fdlist[flag] != -1)
			{
				printf("[%d]Already logined\n", sock_fd_client);
				write(sock_fd_client, "", MAXBUF);	
				continue;
			}

			write(sock_fd_client, "Welcome", MAXBUF);
		} else {
			printf("[%d]Wrong login protocl\n", sock_fd_client);
			write(sock_fd_client, "", MAXBUF);
			continue;
		}

		// get unreaded line 
		char tmpbuf[MAXUNREAD];
		memset(tmpbuf, 0, sizeof(char)*MAXUNREAD);
		int ggline = talk_unread(flag, tmpbuf);
		printf("%d's last line is %d\n", flag, ggline);

		write(sock_fd_client, tmpbuf, MAXBUF);

		// update last_line
		talk_logout(flag, ggline);

		// zip information of client as params
		vargp = malloc(sizeof(arginfo));
		vargp->fd = sock_fd_client;
		vargp->port = 0;
		vargp->nickname = (char*)malloc(sizeof(char*)*MAXBUF);
		vargp->id = flag;
		strcpy(vargp->nickname, buf);

		num_of_connection++;

		// fdlist contains sock_number of each id
		fdlist[flag] = sock_fd_client;
		// note that, slave thread is detached
		pthread_create(&tid, NULL, slave, vargp);

		//pthread_join(tid, NULL);
	}
	
	// close socket
	close(sock_fd_server);
	return 0;
}
