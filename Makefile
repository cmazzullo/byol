repl: repl.c core.o map.o
	etags *
	gcc -g -o repl --std=c99 -Wall mpc/mpc.c core.c map.c repl.c

map.o core.o: map.c core.c
	gcc -c --std=c99 -Wall mpc/mpc.c map.c core.c -I.

run: repl
	./repl
