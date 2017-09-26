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
int current_line = 0;
pthread_mutex_t log_mutex;

// send message to all connected client
void *chat_all(void *str)
{
	char *msg = (char*)str;

	pthread_t tid = pthread_self();
	pthread_detach(tid);

	printf("in chat all");

	for(int i=0;i<4;i++)
	{
		printf("%d ", fdlist[i]);
		if(fdlist[i] != -1)
		{
			write(fdlist[i], msg, sizeof(msg));
		}
	}

	return NULL;
}

void *slave(void *vargp)
{
	int last_line = -1;

	char buf[MAXBUF];
	char msgbuf[MAXBUF];

	memset(buf, 0, sizeof(char)*MAXBUF);
	memset(msgbuf, 0, sizeof(char)*MAXBUF);

	pthread_t tid = pthread_self();
	pthread_detach(tid);

	arginfo argp = *(arginfo*)vargp;
	free(vargp);

	printf("Current num: %d, serving from %d\n", argp.id, argp.fd);

	// 클라이언트에 데이터 전송
	while(read(argp.fd, buf, MAXBUF) != 0)
	{
		printf("talk_write call\n");	
		last_line = talk_write(argp.id, buf, &log_mutex);
		
		sprintf(msgbuf, "[%s]: %s", argp.nickname, buf);
		printf("%d...%s", argp.fd, msgbuf);

		for(int i=0;i<4;i++)
		{
			printf("%d ", fdlist[i]);
			if(fdlist[i] != -1)
			{
				write(fdlist[i], msgbuf, sizeof(msgbuf));
			}
		}

		memset(buf, 0, sizeof(char)*MAXBUF);
		memset(msgbuf, 0, sizeof(char)*MAXBUF);
	}

	// 클라이언트 소켓 종료
	close(argp.fd);

	num_of_connection--;
	printf("%d client close\n", argp.fd);
	if(last_line != -1)
		talk_logout(argp.id, last_line);

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
	pthread_mutex_init(&log_mutex, NULL);

	pthread_mutex_lock(&log_mutex);
	// 서버 소켓 생성 (TCP)
	sock_fd_server = socket(PF_INET, SOCK_STREAM, 0);

	memset(&server_addr, 0, sizeof(struct sockaddr_in));

	// TCP/IP 스택의 주소 집합으로 설정
	server_addr.sin_family = AF_INET;
	// 어떤 클라이언트 주소도 접근할 수 있음
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// 서버의 포트를 2000번으로 설정
	server_addr.sin_port = htons(2000);

	pthread_mutex_unlock(&log_mutex);

	// 서버 소켓에 주소 할당	
	if(bind(sock_fd_server, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in)) == -1){
		// error
	}
	// 서버 대기 상태로 설정
	listen(sock_fd_server, 5);
 
	while(1){
		memset(buf, 0, sizeof(char)*MAXBUF);
		client_addr_size = sizeof(struct sockaddr_in);
		sock_fd_client = accept(sock_fd_server, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_size);

		read(sock_fd_client, buf, MAXBUF);
		printf("[%d]Get %s\n", sock_fd_client, buf);
		fflush(stdout);
		// login protocol
		if(buf[0] == 'l' && buf[1] == '*'){
			memmove(buf, buf+2, strlen(buf)-2);
			buf[strlen(buf)-2] = '\0';

			flag = talk_login(buf);

			if(flag == -1){
				printf("[%d]Wrong id\n", sock_fd_client);
				write(sock_fd_client, "", MAXBUF);	
				continue;
			}

			write(sock_fd_client, "s", MAXBUF);
		} else {
			printf("[%d]Wrong login protocl\n", sock_fd_client);
			write(sock_fd_client, "", MAXBUF);
			continue;
		}

		char tmpbuf[MAXBUF];
		memset(tmpbuf, 0, sizeof(char)*MAXBUF);
		int ggline = talk_unread(flag, tmpbuf);
		printf("%d's last line is %d\n", flag, ggline);
		talk_logout(flag, ggline);

		write(sock_fd_client, tmpbuf, MAXBUF);

		vargp = malloc(sizeof(arginfo));
		vargp->fd = sock_fd_client;
		vargp->port = 0;
		vargp->nickname = (char*)malloc(sizeof(char*)*MAXBUF);
		vargp->id = flag;
		strcpy(vargp->nickname, buf);

		num_of_connection++;

		fdlist[flag] = sock_fd_client;

		pthread_create(&tid, NULL, slave, vargp);

		//pthread_join(tid, NULL);
	}
	
	// 서버 소켓 종료
	close(sock_fd_server);
	return 0;
}
