OBJS=lval.o list.o environment.o builtin.o map.o read.o
CC=gcc
CFLAGS=-g -Wall

run: repl
	./repl

runtest: test
	./test

repl: repl.c $(OBJS)
	etags *
	$(CC) $(CFLAGS) -o repl repl.c $(OBJS) mpc/mpc.c --std=c99 -Wall -I.

test: test.c $(OBJS)
	etags *
	$(CC) $(CFLAGS) -o test test.c $(OBJS) mpc/mpc.c --std=c99 -Wall -I.

clean:
	rm *.o *.gch
