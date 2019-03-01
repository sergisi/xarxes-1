#compiler
CC=gcc
CFLAGS=-ansi -pedantic -Wall

TARGETS=client

all: $(TARGETS)
	
client: client.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o $(TARGETS)