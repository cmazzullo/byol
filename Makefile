repl: repl.c core.o
	etags *
	gcc -g -o repl --std=c99 -Wall mpc/mpc.c core.c repl.c

core.o: core.c
	gcc -c --std=c99 -Wall mpc/mpc.c core.c
run: repl
	./repl
