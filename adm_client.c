#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include "talk.h"

#define DEBUG

int readflag = 0;

void *readchat(void *sock_fd)
{
	int fd = *((int*)sock_fd);
	char buf[MAXBUF];

	pthread_t tid = pthread_self();
	pthread_detach(tid);

	memset(buf, 0, sizeof(char)*MAXBUF);

	while(read(fd, buf, MAXBUF) > 0 && readflag==1)
	{
		printf("%s", buf);
		memset(buf, 0, sizeof(char)*MAXBUF);
	};

	return NULL;
}

int main (int argc, char **argv) {
	int sock_fd = 0;
	
	struct sockaddr_in server_addr;
	
	char *message = malloc(1024);

	char buf2[MAXBUF];
	char buf[MAXBUF];
	char myid[MAXBUF];

	pthread_t tid;

	memset(buf, 0, sizeof(char)*MAXBUF);
	memset(buf2, 0, sizeof(char)*MAXBUF);
	memset(myid, 0, sizeof(char)*MAXBUF);

	talk_intro();

	sock_fd = socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	
	// use loopback when debugging
#ifdef DEBUG
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
#else
	server_addr.sin_addr.s_addr = inet_addr("147.46.242.74");
#endif
	
	server_addr.sin_port = htons(PORT);

	connect(sock_fd, (struct sockaddr*) &server_addr,
		sizeof(server_addr));

	printf("ID: ");
	fgets(buf, MAXBUF, stdin);
	// ignore null input
	if(buf == NULL)
	{
		printf("login failed\n");
		free(message);
		close(sock_fd);
		return 1;
	}
	buf[strlen(buf)-1] = '\0';
	strcpy(myid, buf);

	// login protocol
	// l*id
	sprintf(buf2, "l*%s", buf);
	write(sock_fd, buf2, strlen(buf2));

	read(sock_fd, message, 1024);
	printf("server: %s\n", message);

	// server return "Welcome" when login success
	if(strcmp(message, "Welcome") != 0)
	{
		printf("failed...\n");
		free(message);
		close(sock_fd);
		return 1;
	}

	memset(buf, 0, sizeof(char)*MAXBUF);
	memset(buf2, 0, sizeof(char)*MAXBUF);

	readflag = 1;
	pthread_create(&tid, NULL, readchat, &sock_fd);

	// start chatting (sending)
	while(fgets(buf, MAXBUF, stdin) != NULL)
	{
		// skip null
		if(strlen(buf) == 1)
			continue;

		// string start with @ char will recognized as command
		if(buf[0] == '@')
		{
			// logoff
			if(strncmp(buf, "@logoff", 7) == 0)
			{
				break;
			}
			// invite someone
			else if(strncmp(buf, "@invite", 7) == 0)
			{
				// remove \n char
				buf[strlen(buf)-1] = '\0';

				// when no parameter
				if(strlen(buf) < 9 || buf[7] != ' ')
				{
					printf("Usage: @invite someone\n");
					memset(buf, 0, sizeof(char)*MAXBUF);
					continue;
				}
	
				// when invite itselt
				if(strcmp(myid, buf+8) == 0)
				{
					printf("You can't invite your self\n");
					memset(buf, 0, sizeof(char)*MAXBUF);
					continue;
				}

				write(sock_fd, buf, strlen(buf));
				memset(buf, 0, sizeof(char)*MAXBUF);
			}
			// quit chat room
			else if(strncmp(buf, "@quit", 5) == 0)
			{
				write(sock_fd, buf, strlen(buf));
				memset(buf, 0, sizeof(char)*MAXBUF);
				break;
			}
			else
			{
				// do not send msg
				printf("wrong command usage\n");
			}
		}
		else // normal chat
		{
			write(sock_fd, buf, strlen(buf));
			memset(buf, 0, sizeof(char)*MAXBUF);
		}
	}
 
	free(message);

	// turn off flag to stop read
	readflag = 0;

	// note, kill works better than join
	pthread_kill(tid, 0);
	// pthread_join(tid, NULL);

	close(sock_fd);
	
	printf("GOODBYE\n");
	return 0;
}
