CC=gcc
CFLAGS= -g -Wextra -Wall -pedantic -std=c11 -O2

default: tail

tail: tail.c
	$(CC) $(CFLAGS) tail.c -o tail

clean:
	rm -f tail