/*
A well-behaved REPL
*/

#include "mpc/mpc.h"
#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 1024

// Structs
typedef struct { // lisp value
  int type;
  long num;
  char* err;
  char* sym;
  int count;
  struct lval **cell;
} lval;

// Function prototypes
lval *eval(lval *v);
lval *eval_op(lval *x, lval *op, lval *y);
lval *lval_num(long x);
lval *lval_sym(char *sym);
lval *lval_sexp(void);
lval *lval_err(char *err);
void print_lval(lval *v);
void println_lval(lval *v);
void lval_del(lval *v);
lval *lval_read_num(mpc_ast_t *t);
lval *lval_read_sym(mpc_ast_t *t);
lval *lval_add(lval *v, lval *x);
lval *apply(lval *fn, int argc, lval **args);
// Enums
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXP }; // values

lval *
lval_num(long x) // create new number
{
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval *
lval_err(char *err) // create new error
{
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(err) + 1);
  strcpy(v->err, err);
  return v;
}

lval *
lval_sym(char *sym) // create new symbol
{
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(sym) + 1);
  strcpy(v->sym, sym);
  return v;
}

lval *
lval_sexp(void) // create new empty sexp
{
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SEXP;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void
print_lval(lval *v)
{
  switch (v->type) {
  case LVAL_NUM: printf("%li", v->num); break;
  case LVAL_ERR: puts(v->err); break;
  case LVAL_SYM: puts(v->sym); break;
  case LVAL_SEXP: puts("sexp"); // TODO
  }
}

void
println_lval(lval *v) { print_lval(v); putchar('\n'); }

lval *
eval(lval *v)
{
  if (v->type == LVAL_SEXP) {
    return apply(v->cell[0], v->count - 1, &(v->cell[1]));
  }
  return v;
}

lval *
apply(lval *fn, int argc, lval **args)
{
  lval *x = args[0];
  for (int i = 1; i < argc; i++) {
    x = eval_op(x, fn, args[i]);
  }
  return x;
}


lval *
eval_op(lval *x, lval *op, lval *y)
{
  char *opc = op->sym;
  if (x->type == LVAL_ERR) { return x; }
  if (y->type == LVAL_ERR) { return y; }
  else {
    if (strstr(opc, "+")) return lval_num(x->num + y->num);
    if (strstr(opc, "*")) return lval_num(x->num * y->num);
    if (strstr(opc, "-")) return lval_num(x->num - y->num);
    if (strstr(opc, "/")) {
      if (y->num == 0) {
	return lval_err("ERROR: Division by zero!");
      } else { return lval_num(x->num / y->num); }
    }
  }
  return lval_err("Error: Invalid operator!");
}

void
lval_del(lval *v) // free memory for an lval
{
  switch(v->type) {
  case LVAL_NUM: break;
  case LVAL_ERR: free(v->err); break;
  case LVAL_SYM: free(v->sym); break;
  case LVAL_SEXP:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }
  free(v->cell);
  }
  free(v);
}

lval *
lval_read(mpc_ast_t *t) // convert the AST into a sexp
{
  if (strstr(t->tag, "num")) { return lval_read_num(t); }
  if (strstr(t->tag, "sym")) { return lval_read_sym(t); }

  // if the tree is the root or a sexp then create an empty sexp
  lval *v;
  if ((strcmp(t->tag, ">") == 0) || (strstr(t->tag, "sexp"))) {
    v = lval_sexp();
  }

  // fill the empty sexp with any expressions within
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->tag, "regex")) { continue; } // skip the noise
    if (strcmp(t->children[i]->contents, "(")) { continue; }
    if (strcmp(t->children[i]->contents, ")")) { continue; }
    lval_add(v, lval_read(t->children[i]));
  }

  return v;
}

lval *
lval_add(lval *v, lval *x)
{
  ++v->count;
  realloc(v->cell, sizeof(lval *) * (v->count)); // resize the cell array
  v->cell[v->count - 1] = x; // append x to the end of the cell array
  return v;
}

lval *
lval_read_sym(mpc_ast_t *t)
{
  char *sym = t->contents;
  return lval_sym(sym);
}

lval *
lval_read_num(mpc_ast_t *t)
{
  long x = strtol(t->contents, NULL, 10);
  if (errno != ERANGE) return lval_num(x);
  else return lval_err("ERROR: Invalid number!");
}

int
main (int argc, char **argv)
{
  char *line = malloc(MAXLINE * sizeof (char));
  mpc_parser_t *Num = mpc_new("num");
  mpc_parser_t *Symbol = mpc_new("symbol");
  mpc_parser_t *Exp = mpc_new("exp");
  mpc_parser_t *Sexp = mpc_new("sexp");
  mpc_parser_t *Input = mpc_new("input");

  mpca_lang(MPCA_LANG_DEFAULT,"\
  symbol: '+' | '-' | '*' | '/' | '%' ; \
  num: /-?[0-9]+/ ; \
  exp: <num> | <symbol> | <sexp> ;   \
  sexp: '(' <exp>* ')' ; \
  input: /^/ <exp> /$/ ;", Symbol, Num, Exp, Sexp, Input);

  mpc_result_t r;

  while (1) {
    printf("> ");
    fgets(line, MAXLINE, stdin);

    if (mpc_parse("<stdin>", line, Input, &r)) {
      println_lval(eval(lval_read(r.output)));
      mpc_ast_delete(r.output);
    } else { // catch syntax errors here
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  mpc_cleanup(5, Symbol, Num, Exp, Sexp, Input);
  putchar('\n');
  free(line);
  return 0;
}
