#include "read.h"
#include "mpc/mpc.h"
#include "lval.h"
#include "read.h"
#include "environment.h"
#include "builtin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1024

// MAIN ////////////////////////////////////////////////////////////////////////////////


void
run_repl(mpc_parser_t *Input, lenv *e)
{
  char *line = malloc(MAXLINE * sizeof (char));
  while (strcmp(line, "quit\n") != 0) {
    printf("> ");
    fgets(line, MAXLINE, stdin);
    lval *input = read_line(Input, line);
    lval *output = lval_eval(e, input);
    print_lval(output);
    putchar('\n');
  }
  free(line);
  putchar('\n');
}

mpc_parser_t *
make_parser(void)
{
  mpc_parser_t *Bool = mpc_new("bool");
  mpc_parser_t *Num = mpc_new("num");
  mpc_parser_t *Symbol = mpc_new("symbol");
  mpc_parser_t *Exp = mpc_new("exp");
  mpc_parser_t *Sexp = mpc_new("sexp");
  mpc_parser_t *Input = mpc_new("input");

  mpca_lang(MPCA_LANG_DEFAULT,"\
  bool : \"true\" | \"false\" ;						\
  num : /-?[0-9]+/ ;							\
  symbol : /[a-zA-Z0-9*+\\-\\/\\\\_=<>!&]+/ ;				\
  sexp : '(' <exp>* ')' ;						\
  exp : <bool> | <num> | <symbol> | <sexp> ; \
  input : /^/ <exp>? /$/ ;", Bool, Num, Symbol, Sexp, Exp, Input);

  return Input;
}

/* Main loop, provides a REPL */
int
main (int argc, char **argv)
{

  lenv *e = lenv_new(NULL); // Create the global environment
  env_add_builtins(e);

  run_repl(make_parser(), e);

  lenv_delete(e);
  //  mpc_cleanup(6, Bool, Num, Symbol, Sexp, Exp, Input);
  return 0;
}
