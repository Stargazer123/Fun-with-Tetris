CC=gcc
OPTS=-O3 -std=c99 -Wall

tetris:
	$(CC) $(OPTS) -o tetris tetris.c -lncurses
	$(CC) $(OPTS) -o netris netris.c -lncurses -lpthread

clean:
	rm netris
	rm tetris





