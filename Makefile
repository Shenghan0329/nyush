CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all
all: nyush

nyush: nyush.o console.o

nyush.o: nyush.c console.h

console.o: console.c console.h

.PHONY: clean
clean:
	rm -f *.o nyush
