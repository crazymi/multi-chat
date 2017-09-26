CC = gcc
CFLAGS = -Wall -std=c99 -g
LIBS=-lpthread

TARGETS = talk.o adm_server adm_client

FILES = Makefile adm_server.c adm_client.c talk.c talk.h

all: $(TARGETS)

talk.o: talk.c talk.h
	$(CC) $(CFLAGS) -c talk.c

adm_server: adm_server.c talk.c talk.h
	$(CC) $(CFLAGS) -o adm_server adm_server.c talk.o $(LIBS)

adm_client: adm_client.c talk.c talk.h
	$(CC) $(CFLAGS) -o adm_client adm_client.c talk.o $(LIBS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

.PHONY: clean
clean:
	-rm -f adm_server adm_client talk.o talk.log *.talk
