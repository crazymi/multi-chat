#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include "talk.h"

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
	// ������ ���� �ּ� ������ ���� ����ü
	struct sockaddr_in server_addr;
	// �������� �޾ƿ� ������ ���� ����
	char *message = malloc(1024);

	char buf2[MAXBUF];
	char buf[MAXBUF];

	pthread_t tid;

	memset(buf, 0, sizeof(char)*MAXBUF);
	memset(buf2, 0, sizeof(char)*MAXBUF);

	// talk_intro();

	// ���� ������ ���� ����(TCP) ����
	sock_fd = socket(PF_INET, SOCK_STREAM, 0);
	// �ʱ�ȭ
	memset(&server_addr, 0, sizeof(server_addr));
	// TCP/IP ������ ���� �ּ� ����(Address Family)���� ����
	server_addr.sin_family = AF_INET;
	// ������ ������ �ּҴ� ���� ȣ��Ʈ(127.0.0.1)�� ����
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	// ������ ������ ��Ʈ ��ȣ�� 2000��
	server_addr.sin_port = htons(2000);

	connect(sock_fd, (struct sockaddr*) &server_addr,
		sizeof(server_addr));

	printf("ID: ");
	fgets(buf, MAXBUF, stdin);
	if(buf == NULL)
	{
		printf("login failed\n");
		free(message);
		close(sock_fd);
		return 1;
	}
	buf[strlen(buf)-1] = '\0';

	// login protocol
	// l*id
	sprintf(buf2, "l*%s", buf);
	write(sock_fd, buf2, strlen(buf2));

	read(sock_fd, message, 1024);
	printf("server: %s\n", message);
	if(strcmp(message, "s") != 0)
	{
		printf("login failed\n");
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

		// command
		if(buf[0] == '@')
		{
			// logoff
			if(strncmp(buf, "@logoff", 7) == 0)
			{
				break;
			}
			else if(strncmp(buf, "@invite", 7) == 0)
			{
				// no parameter
				if(strlen(buf) < 10)
				{
					printf("Usage: @invite someone\n");
					continue;
				}

				sprintf(buf2, "i*%s", buf+8);

				write(sock_fd, buf2, strlen(buf));

				memset(buf, 0, sizeof(char)*MAXBUF);
				read(sock_fd, buf, MAXBUF);
				printf("%s", buf);

				continue;
			}
			else
			{
				// do not send msg
				printf("wrong command usage\n");
				continue;
			}
		}
		else // normal chat
		{
			sprintf(buf2, "c*%s", buf);
			write(sock_fd, buf, strlen(buf));
			memset(buf2, 0, sizeof(char)*MAXBUF);
		}
	}
 
	free(message);

	readflag = 0;
	pthread_kill(tid, 0);

	close(sock_fd);
	// pthread_join(tid, NULL);

	// ���� ����

	printf("GOODBYE\n");
	return 0;
}
