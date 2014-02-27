CC = gcc

pdp8: pdp8.c
	$(CC) -Wall -O2 -g pdp8.c -o pdp8
