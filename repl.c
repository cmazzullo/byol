#include "mpc/mpc.h"
#include "lval.h"
#include "read.h"
#include "environment.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1024

// MAIN ////////////////////////////////////////////////////////////////////////////////

void
run_repl(mpc_parser_t *Input, lenv *e)
{
  char *line = malloc(MAXLINE * sizeof (char));
  mpc_result_t r;
  while (strcmp(line, "quit\n") != 0) {
    printf("> ");
    fgets(line, MAXLINE, stdin);
    if (mpc_parse("<stdin>", line, Input, &r)) {
      mpc_ast_t *ast_input = r.output;
      lval *lval_input = read(ast_input);
      lval *result = lval_eval(e, lval_input);
      print_lval(result);
      putchar('\n');
      mpc_ast_delete(r.output);
    } else { // catch syntax errors here
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }
  free(line);
  putchar('\n');
}

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

  lenv *e = lenv_new(NULL); // Create the global environment
  env_add_builtins(e);

  run_repl(Input, e);

  lenv_delete(e);
  mpc_cleanup(6, Bool, Num, Symbol, Sexp, Exp, Input);
  return 0;
}
