OBJS=core.o map.o read.o

run: repl
	./repl

repl: repl.c $(OBJS)
	etags *
	gcc -g -o repl repl.c $(OBJS) mpc/mpc.c --std=c99 -Wall -I.
