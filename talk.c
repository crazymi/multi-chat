#include "talk.h"

void talk_intro()
{
    printf("*****************************\n");
    printf("*****************************\n");
    printf("********JJAB-TALK************\n");
    printf("*****************************\n");
    printf("*****************************\n");
}

int talk_login(char *buf)
{
	FILE *fp;

	char nick[MAXBUF];
	int id = -1;

	fp = fopen("talk.user", "r");
	while(fscanf(fp, "%s %d\n", nick, &id) != EOF)
	{
		printf("%s <> %s\n", nick, buf);
		if(strcmp(nick, buf) == 0)
		{
			return id;
		}
		id = -1;
	}
	fclose(fp);

	if(id == -1){
        return -1;
	}
    printf("Logined with: %s %d\n", buf, id);

    return id;
}


// return current linenumber of talk.log
int talk_len()
{
	FILE *fp;

	char buf[MAXBUF];
	int line = 0;

	fp = fopen("talk.log", "r");

	if(fp == NULL){
		return 0;
	}

	while(fgets(buf, 1024, fp) != NULL){
		line++;
	}

	fclose(fp);
	
	return line;
}

int talk_write(int id, char *str, pthread_mutex_t *mutex)
{
	FILE *fp;

	time_t tm_time;
	struct tm *st_time;
	char buf[MAXBUF];

	// divide 3 due to writing protocol
	int line = talk_len()/3;

	// if below line not exists, mutex_lock not work
	if(mutex == NULL){
		printf("mutex is null\n");
		return -1;
	}

	// lock log
	pthread_mutex_lock(mutex);
	fp = fopen("talk.log", "a");
	//printf("mutex locked\n");

	time(&tm_time);;
	st_time = localtime(&tm_time);
	strftime(buf, MAXBUF, "%m-%d %H:%M:%S", st_time);

	if(buf == NULL)
		return -1;

	// str: replace \n to \0
	// str[strlen(str)] = '\0';

	fprintf(fp, "[%d] %d\n", line+1, id);
	fprintf(fp, "%s\n", buf);
	fprintf(fp ,"%s", str);

	fclose(fp);
	// unlock log
	pthread_mutex_unlock(mutex);
	//printf("mutex unlocked\n");

	return line+1;
}


// return 1 for success else for fail
// read talk_struct at input line
int talk_read(int line, talk *ktalk)
{
	FILE *fp;
	int cline;
	int id;
	char buf[MAXBUF];
	char msg[MAXBUF];

	fp = fopen("talk.log", "r");

	while(fscanf(fp, "[%d] %d\n", &cline, &id) != EOF)
	{
		fgets(buf, MAXBUF, fp);
		fgets(msg, MAXBUF, fp);

		if(cline == line)
			break;

		memset(buf, 0, sizeof(char)*MAXBUF);
		memset(msg, 0, sizeof(char)*MAXBUF);
	} 

	fclose(fp);
	
	if(cline != line)
		return -1;

	// return struct
	ktalk->line = line;
	ktalk->id = id;
	strcpy(ktalk->msg, msg);
	strcpy(ktalk->date, buf);

	return 1;
}

int talk_logout(int id, int line)
{
	FILE *fp;
	char fname[MAXBUF];

	memset(fname, 0, sizeof(char)*MAXBUF);
	// if exists then overwrite
	sprintf(fname, "%d.talk", id);

	if((fp = fopen(fname, "w")) == NULL)
	{
		return -1;
	}
	fprintf(fp, "%d", line);

	fclose(fp);

	return 1;
}

int talk_unread(int id, char *str)
{
	FILE *fp;
	int last_line = -1;

	int cline = -1;
	int cid = -1;

	char *userlist[] = {"A", "B", "C", "D"};

	char buf[MAXBUF];
	char buf2[MAXBUF];
	char msgbuf[MAXBUF];
	char unread[MAXUNREAD];

	memset(buf, 0, sizeof(char)*MAXBUF);
	memset(buf2, 0, sizeof(char)*MAXBUF);
	memset(msgbuf, 0, sizeof(char)*MAXBUF);
	memset(unread, 0, sizeof(char)*MAXUNREAD);

	// read last_line from %d.talk file
	sprintf(buf, "%d.talk", id);

	// file not exists means invited
	if((fp = fopen(buf, "r")) == NULL)
	{
		return talk_len();
	}
	else 
	{
		fscanf(fp, "%d", &last_line);
		fclose(fp);
	}	

	if(last_line == -1)
		return 0;
	if((fp = fopen("talk.log", "r")) == NULL)
		return 0;

	while(fscanf(fp, "[%d] %d\n", &cline, &cid) != EOF)
	{
		// datetime to 'buf', message to 'buf2'
		fgets(buf, MAXBUF, fp);
		fgets(buf2, MAXBUF, fp);

		// if unreaded line, 
		if(cline > last_line)
		{
			sprintf(msgbuf, "[%s]: %s", userlist[cid], buf2);
			strcat(unread, msgbuf);
		}

		memset(buf, 0, sizeof(char)*MAXBUF);
		memset(buf2, 0, sizeof(char)*MAXBUF);
		memset(msgbuf, 0, sizeof(char)*MAXBUF);
	}

	// copy unread string to return string
	strcpy(str, unread);

	fclose(fp);

	return cline;
}
