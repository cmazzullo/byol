repl: repl.c
	gcc -o repl --std=c99 -lm mpc/mpc.c repl.c

run: repl
	./repl
