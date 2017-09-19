repl: repl.c
	gcc -o repl -Wall -lreadline -lm mpc/mpc.c repl.c
