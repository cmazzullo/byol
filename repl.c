/*
A well-behaved REPL
*/

#include "mpc/mpc.h"
#include <stdio.h>
#include <stdlib.h>

/* #include <readline/readline.h> */
/* #include <readline/history.h> */

#define MAXLINE 1024


// Structs
typedef struct { // lisp value
  int type;
  int num;
  int err;
} lval;


// Function prototypes
int eval(mpc_ast_t *t);
int eval_op(int x, char *op, int y);
lval lval_num(int x);
lval lval_err(int x);
void print_lval(lval v);

// Enums
enum { LVAL_NUM, LVAL_ERR }; // values
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM }; // errors


lval lval_num(int x) { // create new lisp value
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}


lval lval_err(int x) { // create new error
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}


void print_lval(lval v) {
  switch (v.type) {
  case LVAL_NUM: printf("%d", v.num); break;

  case LVAL_ERR:
    if (v.err == LERR_DIV_ZERO) printf("ERROR: Division by zero");
    if (v.err == LERR_BAD_OP) printf("ERROR: Invalid operator");
    if (v.err == LERR_BAD_NUM) printf("ERROR: Invalid number");
    break;
  }
}


int eval(mpc_ast_t *t) {
  if (strstr(t->tag, "num"))
    return atoi(t->contents);

  char *op = t->children[1]->contents;

  int x = eval(t->children[2]); // store first arg's value

  // iterate through the rest of the children
  for (int i = 3; strstr(t->children[i]->tag, "exp"); i++){
    x = eval_op(x, op, eval(t->children[i]));
  }
  return x;
}


int eval_op(int x, char *op, int y) {
  if (strstr(op, "+")) return x + y;
  if (strstr(op, "*")) return x * y;
  if (strstr(op, "-")) return x - y;
  if (strstr(op, "/")) return x / y;
  return 0;
}

int main (int argc, char **argv) {
  char *line = malloc(MAXLINE * sizeof (char));
  mpc_parser_t *Num = mpc_new("num");
  mpc_parser_t *Op = mpc_new("op");
  mpc_parser_t *Exp = mpc_new("exp");
  mpc_parser_t *Input = mpc_new("input");

  // Prefix Notation
  mpca_lang(MPCA_LANG_DEFAULT," \
  op: '+' | '-' | '*' | '/' ; \
  num: /-?[0-9]+/ ;                    \
  exp: <num> | '(' <op> <exp>* ')' ;   \
  input: /^/ <op> <exp>+ /$/ ;    \
  ", Op, Num, Exp, Input);

  mpc_result_t r;

  while (1) {
    printf("> ");
    fgets(line, MAXLINE, stdin);

    if (mpc_parse("<stdin>", line, Input, &r)) {
      printf("%d\n", eval(r.output));
      mpc_ast_delete(r.output);
    } else { // catch syntax errors here
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  mpc_cleanup(4, Num, Op, Exp, Input);
  putchar('\n');
  free(line);
  return 0;
}

/* void run_repl(mpc_parser_t *Input) { */
/*   char *line; */

/*   puts( "MZLisp v0.01"); */
/*   puts( "C-c to exit"); */
/*   while ((line = readline("MZL> ")) != NULL) { */
/*     parse_input(line, Input); */
/*     add_history(line); */
/*     free(line); */
/*   } */
/*   putchar('\n'); */
/* } */
