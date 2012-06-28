all:cmc
CC=gcc
CFLAGS=-Wall -g -c -DDEBUG  -ansi -DPROG_NAME=\"cmc\" -DHAVE_ISATTY
midi.o: midi.c midi.h stream.h
	$(CC) $(CFLAGS) midi.c
util.o: util.c util.h
	$(CC) $(CFLAGS) util.c
stream.o: stream.c stream.h
	$(CC) $(CFLAGS) stream.c
cmc.o: cmc.c midi.h util.h scanner.h
	$(CC) $(CFLAGS) cmc.c
scanner.o: scanner.c scanner.h stream.h
	$(CC) $(CFLAGS) scanner.c
cmc: stream.o midi.o cmc.o util.o scanner.o
	$(CC) stream.o midi.o util.o scanner.o cmc.o -o cmc
