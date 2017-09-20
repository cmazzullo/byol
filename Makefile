repl: repl.c
	gcc -o repl --std=c99 -Wall -lm mpc/mpc.c repl.c

run: repl
	./repl
