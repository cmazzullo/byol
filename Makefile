OBJS=core.o builtin.o map.o read.o list.o

run: repl
	./repl

repl: repl.c $(OBJS)
	etags *
	gcc -g -o repl repl.c $(OBJS) mpc/mpc.c --std=c99 -Wall -I.

clean:
	rm *.o *.gch
