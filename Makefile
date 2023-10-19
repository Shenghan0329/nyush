CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all
all: nyush

nyush: nyush.o console.o validate.o command.o

nyush.o: nyush.c console.h

console.o: console.c console.h validate.h command.h errors.h

validate.o: validate.c validate.h

command.o: command.c command.h constants.h

.PHONY: clean
clean:
	rm -f *.o nyush
