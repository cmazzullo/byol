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
  long num;
  int err;
} lval;


// Function prototypes
lval eval(mpc_ast_t *t);
lval eval_op(lval x, char *op, lval y);
lval lval_num(long x);
lval lval_err(long x);
void print_lval(lval v);
void println_lval(lval v);


// Enums
enum { LVAL_NUM, LVAL_ERR }; // values
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM }; // errors


lval lval_num(long x) { // create new lisp value
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}


lval lval_err(long x) { // create new error
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}


void print_lval(lval v) {
  switch (v.type) {
  case LVAL_NUM: printf("%li", v.num); break;

  case LVAL_ERR:
    if (v.err == LERR_DIV_ZERO) printf("ERROR: Division by zero");
    if (v.err == LERR_BAD_OP) printf("ERROR: Invalid operator");
    if (v.err == LERR_BAD_NUM) printf("ERROR: Invalid number");
    break;
  }
}

void println_lval(lval v) { print_lval(v); putchar('\n'); }


lval eval(mpc_ast_t *t) {
  if (strstr(t->tag, "num")) {
    long x = strtol(t->contents, NULL, 10);
    if (errno != ERANGE) return lval_num(x);
    else return lval_err(LERR_BAD_NUM);
  }

  char *op = t->children[1]->contents;
  lval x = eval(t->children[2]); // store first arg's value
  // iterate through the rest of the children
  for (int i = 3; strstr(t->children[i]->tag, "exp"); i++){
    x = eval_op(x, op, eval(t->children[i]));
  }
  return x;
}


lval eval_op(lval x, char *op, lval y) {
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }
  else {
    if (strstr(op, "+")) return lval_num(x.num + y.num);
    if (strstr(op, "*")) return lval_num(x.num * y.num);
    if (strstr(op, "-")) return lval_num(x.num - y.num);
    if (strstr(op, "/")) {
      if (y.num == 0) {
	return lval_err(LERR_DIV_ZERO);
      } else { return lval_num(x.num / y.num); }
    }
  }
  return lval_err(LERR_BAD_OP);
}


int main (int argc, char **argv) {
  char *line = malloc(MAXLINE * sizeof (char));
  mpc_parser_t *Num = mpc_new("num");
  mpc_parser_t *Op = mpc_new("op");
  mpc_parser_t *Exp = mpc_new("exp");
  mpc_parser_t *Input = mpc_new("input");

  // Prefix Notation
  mpca_lang(MPCA_LANG_DEFAULT,"op: '+' | '-' | '*' | '/' | '%' ; \
  num: /-?[0-9]+/ ; \
  exp: <num> | '(' <op> <exp>* ')' ;   \
  input: /^/ <op> <exp>+ /$/ ;", Op, Num, Exp, Input);

  mpc_result_t r;

  while (1) {
    printf("> ");
    fgets(line, MAXLINE, stdin);

    if (mpc_parse("<stdin>", line, Input, &r)) {
      println_lval(eval(r.output));
      // mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    } else { // catch syntax errors here
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  mpc_cleanup(4, Op, Num, Exp, Input);
  putchar('\n');
  free(line);
  return 0;
}
