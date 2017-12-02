#include "read.h"
#include "lval.h"
#include "environment.h"
#include "builtin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1024

// MAIN ////////////////////////////////////////////////////////////////////////////////

void
run_repl(lenv *e)
{
  char *line = malloc(MAXLINE * sizeof (char));
  while (strcmp(line, "quit\n") != 0) {
    printf("> ");
    fgets(line, MAXLINE, stdin);
    lval *input = read_line(line);
    lval *output = lval_eval(e, input);
    print_lval(output);
    putchar('\n');
  }
  free(line);
  putchar('\n');
}

/* Main loop, provides a REPL */
int
main (int argc, char **argv)
{

  lenv *e = lenv_new(NULL); // Create the global environment
  env_add_builtins(e);
  read_initialize();

  // Read in the standard library
  lval *args = lval_sexp();
  lval_cons(args, lval_string("stdlib.byol"));
  builtin_load(e, args);

  run_repl(e);
  read_cleanup();
  lenv_delete(e);
  return 0;
}
