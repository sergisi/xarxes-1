#compiler
CC=gcc
CFLAGS=-pedantic -Wall -ansi

TARGETS=client

all: $(TARGETS)
	
client: client.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o $(TARGETS)