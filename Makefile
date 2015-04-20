CC=gcc
CFLAGS=-Wall -g -o 
LDFLAGS=-lm -lpthread
all:
	$(CC) $(CFLAGS) bookorder bookorder.c $(LDFLAGS)
clean:
	rm -rf bookorder
