/*
A well-behaved REPL
*/

#include "mpc/mpc.h"
#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 1024

// Structs
typedef struct lval { // lisp value
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
void print_lval_sexp(lval *v, char open, char close);
void lval_del(lval *v);
lval *lval_read_num(mpc_ast_t *t);
lval *lval_add(lval *v, lval *x);
lval *apply(lval *fn, int argc, lval **args);
lval *builtin_op(lval *fn, lval *args);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);

// Enums
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXP }; // values


// CONSTRUCTORS
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

// DESTRUCTOR
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

void
print_lval(lval *v)
{
  switch (v->type) {
  case LVAL_NUM: printf("%li", v->num); break;
  case LVAL_ERR: printf(v->err); break;
  case LVAL_SYM: printf(v->sym); break;
  case LVAL_SEXP: print_lval_sexp(v, '(', ')');
  }
}

void
print_lval_sexp(lval *v, char open, char close)
{
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    if (i != 0) putchar(' ');
    print_lval(v->cell[i]);
  }
  putchar(close);
}

void
println_lval(lval *v) { print_lval(v); putchar('\n'); }

lval *
eval(lval *v) // evaluates an lval recursively
{
  if (v->type == LVAL_SEXP) {
    if (v->count <= 1) return v; // return `()` and `(5)` as-is

    // evaluate all children
    for (int i = 0; i < v->count; i++) {
      v->cell[i] = eval(v->cell[i]);
      if (v->cell[i]->type == LVAL_ERR)
	return v->cell[i]; // handle errors
    }



    lval *first = lval_pop(v, 0); // pops off the first element
    if (first->type == LVAL_SYM) {
      return builtin_op(first, v); // evaluates `first` as a function with args = `v`
    } else {
      return lval_err("ERROR: Invalid function");
    }
  }
  return v;
}

lval *
builtin_op(lval *fn, lval *args) // apply fn to args
{
  char *op = fn->sym;

  for (int i = 0; i < args->count; i++) { // check for non-number args
    if (args->cell[i]->type != LVAL_NUM)
      return lval_err("ERROR: Cannot operate on non-number!");
  }

  // pop first arg
  lval *first = lval_pop(args, 0);

  while (args->count > 0) {
    lval *v = lval_pop(args, 0);
    if (strcmp(op, "+") == 0) first->num += v->num;
    if (strcmp(op, "-") == 0) first->num -= v->num;
    if (strcmp(op, "*") == 0) first->num *= v->num;
    if (strcmp(op, "/") == 0) {
      if (v->num == 0) return lval_err("ERROR: Division by zero!");
      first->num /= v->num;
    }
    lval_del(v);
  }
  lval_del(args);
  return first;
}

lval *
lval_pop(lval *v, int i) // pop off the first child of the sexp (car)
{
  lval *x = v->cell[i];

  // shift the lvals after i to the left
  memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (v->count - i));
  v->count--;
  v->cell = realloc(v->cell, sizeof(lval *) * v->count);
  return x;
}

lval *
lval_take(lval *v, int i) // like pop but delete the resulting list
{
  lval *x = lval_pop(v, i);
  lval_del(v);
  return x;
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

lval *
lval_read(mpc_ast_t *t) // convert the AST into a sexp
{
  if (strstr(t->tag, "num")) { return lval_read_num(t); }
  if (strstr(t->tag, "sym")) { return lval_sym(t->contents); }

  // if the tree is the root or a sexp then create an empty sexp
  lval *v;
  if ((strcmp(t->tag, ">") == 0) || (strstr(t->tag, "sexp"))) {
    v = lval_sexp();
  }

  // fill the empty sexp with any expressions within
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->tag, "regex") == 0) { continue; } // skip the noise
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    lval_add(v, lval_read(t->children[i]));
  }
  return v;
}

lval *
lval_add(lval *v, lval *x)
{
  ++(v->count);
  v->cell = realloc(v->cell, sizeof(lval *) * (v->count)); // resize the cell array
  v->cell[v->count - 1] = x; // append x to the end of the cell array
  return v;
}

lval *
lval_read_num(mpc_ast_t *t) // Convert an AST to an LVAL containing a number
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
  input: /^/ <exp>+ /$/ ;", Symbol, Num, Exp, Sexp, Input);

  mpc_result_t r;

  while (1) {
    printf("> ");
    fgets(line, MAXLINE, stdin);

    if (mpc_parse("<stdin>", line, Input, &r)) {
      println_lval(eval(lval_read(r.output)));
      /* mpc_ast_print(r.output); */
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
