#ifndef TALK_H
#define TALK_H

#define MAX_CLIENT_NUMBER 4
#define MAXBUF 1024

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    int fd;
    int port;
	char *nickname;
	int id;
} arginfo;

typedef struct {
	int line;
	int id;
	char *msg;
	char *date;
} talk;


void talk_intro();
int talk_login();

int talk_len();
// write talk data to talk.log, return line num if success
int talk_write(int id, char *str, pthread_mutex_t *mutex);
int talk_read(int line, talk *ktalk);
// last line data : %d.talk, id
int talk_logout(int id, int line);
// copy <id>'s unreaded chat to <str>
int talk_unread(int id, char *str);

#endif // TALK_H
