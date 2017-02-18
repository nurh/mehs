# A really simple Makefile. LibEV is required to compile.

CC=gcc

CFLAGS=-O2 -Wall -fno-strict-aliasing -D_GNU_SOURCE

LIBS=-lev

all: mehs

mehs: mehs.o
	$(CC) -o mehs mehs.o $(LIBS)

mehs.o: mehs.c
	$(CC) $(CFLAGS) -c mehs.c

clean:
	rm -f mehs *.o
