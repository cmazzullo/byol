#include "mpc/mpc.h"
#include "core.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h> // for boolean values

#define MAXLINE 1024

// MAIN ////////////////////////////////////////////////////////////////////////////////

/* Main loop, provides a REPL */
int
main (int argc, char **argv)
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

  mpc_result_t r;
  lenv* e = lenv_new(); // create the environment
  lenv_add_builtins(e);

  char *line = malloc(MAXLINE * sizeof (char));

  while (true) {
    printf("> ");
    fgets(line, MAXLINE, stdin);

    if (mpc_parse("<stdin>", line, Input, &r)) {
      print_lval(lval_eval(e, read(r.output)));
      putchar('\n');
      mpc_ast_delete(r.output);
    } else { // catch syntax errors here
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  lenv_del(e);

  mpc_cleanup(6, Bool, Num, Symbol, Sexp, Exp, Input);
  putchar('\n');
  free(line);
  return 0;
}
