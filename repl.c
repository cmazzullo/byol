/*
A well-behaved REPL
*/

#include "mpc/mpc.h"
#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 1024
#define LASSERT(args, cond, err) \
  if (!(cond)) { lval_del(args); return lval_err(err); }

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
lval *lval_eval(lval *v);
lval *lval_num(long x);
lval *lval_sym(char *sym);
lval *lval_sexp(void);
lval *lval_qexp(void);
lval *lval_err(char *err);
void print_lval(lval *v);
void println_lval(lval *v);
void print_lval_sexp(lval *v, char open, char close);
void lval_del(lval *v);
lval *read_num(mpc_ast_t *t);
lval *lval_add(lval *v, lval *x);
lval *builtin_op(lval *fn, lval *args);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lval *builtin_head(lval *a);
lval *list(lval *v);
lval *head(lval *v);
lval *tail(lval *v);
lval *join(lval *v);
lval *eval(lval *v);

// Enums
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXP, LVAL_QEXP }; // values


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

lval *
lval_qexp(void) // create new empty qexp
{
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_QEXP;
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
  case LVAL_QEXP:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }
  free(v->cell);
  }
  free(v);
}

// BUILTIN FUNCTIONS
lval *
builtin_list(lval *args) // Takes one or more args, returns a qexp containing them
{
  lval *q = lval_qexp();
  while (args->count > 0) lval_add(q, lval_pop(args, 0));

  lval_del(args);
  return q;
}

lval *
builtin_head(lval *args) // Returns the first element of a qexp
{
  LASSERT(args, args->count == 1,
	  "ERROR: Function head passed too many arguments!");
  LASSERT(args, args->cell[0]->type == LVAL_QEXP,
	  "ERROR: Cannot take head of a non-QEXP!");
  LASSERT(args, args->cell[0]->count != 0,
	  "ERROR: Cannot take head of an empty QEXP!");

  lval *v = lval_take(args, 0);
  while (v->count > 1) lval_del(lval_pop(v, 1)); // pop & delete the second lval leaving only the head
  return v;
}

lval *
builtin_tail(lval *args) // Returns the cdr of a qexp
{
  LASSERT(args, args->count == 1,
	  "ERROR: Function tail passed too many arguments!");
  LASSERT(args, args->cell[0]->type == LVAL_QEXP,
	  "ERROR: Cannot take tail of a non-QEXP!");
  LASSERT(args, args->cell[0]->count != 0,
	  "ERROR: Cannot take tail of an empty QEXP!");

  lval *v = lval_take(args, 0);
  lval_del(lval_pop(v, 0)); // pop & delete the first lval leaving the cdr
  return v;
}

lval *
builtin_join(lval *args) // Join together one or more qexps
{
  LASSERT(args, args->count != 0,
	  "ERROR: Function join passed 0 arguments!");

  lval *v = lval_qexp();
  while (args->count > 0) {
    lval *a = lval_pop(args, 0);
    LASSERT(a, a->type == LVAL_QEXP,
	    "ERROR: Cannot take tail of a non-QEXP!");
    while (a->count > 0) lval_add(v, lval_pop(a, 0));
    lval_del(a);
  }

  lval_del(args);
  return v;
}

lval *
eval(lval *args)
{
  LASSERT(args, args->count == 1,
	  "ERROR: Function eval passed too many arguments!");
  lval *a = args->cell[0];
  LASSERT(args, a->type == LVAL_QEXP, "ERROR: Cannot eval a non-QEXP!");
  lval *v = lval_sexp();
  while (a->count > 0) lval_add(v, lval_pop(a, 0));
  lval_del(args);
  return lval_eval(v);
}

// PRINTING
void
print_lval(lval *v)
{
  switch (v->type) {
  case LVAL_NUM: printf("%li", v->num); break;
  case LVAL_ERR: printf(v->err); break;
  case LVAL_SYM: printf(v->sym); break;
  case LVAL_SEXP: print_lval_sexp(v, '(', ')'); break;
  case LVAL_QEXP: print_lval_sexp(v, '{', '}'); break;
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

// EVALUATION
lval *
lval_eval(lval *v) // evaluates an lval recursively
{
  if (v->type == LVAL_SEXP) {
    if (v->count <= 1) return v; // return `()` and `(5)` as-is

    // evaluate all children
    for (int i = 0; i < v->count; i++) {
      v->cell[i] = lval_eval(v->cell[i]);
      if (v->cell[i]->type == LVAL_ERR)
	return v->cell[i]; // handle errors
    }



    lval *first = lval_pop(v, 0); // pops off the first element
    LASSERT(first, first->type == LVAL_SYM,
	    "ERROR: Invalid function");
      return builtin_op(first, v); // evaluates `first` as a function with args = `v`
  }
  return v;
}

lval *
builtin_op(lval *fn, lval *args) // apply fn to args
{
  char *op = fn->sym;

  if (strcmp(op, "list") == 0) { return builtin_list(args); }
  if (strcmp(op, "head") == 0) { return builtin_head(args); }
  if (strcmp(op, "tail") == 0) { return builtin_tail(args); }
  if (strcmp(op, "join") == 0) { return builtin_join(args); }
  if (strcmp(op, "eval") == 0) { return eval(args); }

  // Numerical functions -
  for (int i = 0; i < args->count; i++) { // check for non-number args
    LASSERT(args, args->cell[i]->type == LVAL_NUM,
	    "ERROR: Cannot operate on non-number!");
  }

  lval *first = lval_pop(args, 0);

  while (args->count > 0) {
    lval *v = lval_pop(args, 0);
    if (strcmp(op, "+") == 0) first->num += v->num;
    if (strcmp(op, "-") == 0) first->num -= v->num;
    if (strcmp(op, "*") == 0) first->num *= v->num;
    if (strcmp(op, "/") == 0) {
      LASSERT(v, v->num != 0, "ERROR: Division by zero!");
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
read(mpc_ast_t *t) // convert the AST into a sexp
{
  if (strstr(t->tag, "num")) { return read_num(t); }
  if (strstr(t->tag, "sym")) { return lval_sym(t->contents); }

  // if the tree is the root or a sexp then create an empty sexp
  lval *v;
  if ((strcmp(t->tag, ">") == 0) || (strstr(t->tag, "sexp"))) {
    v = lval_sexp();
  }
  else if (strstr(t->tag, "qexp")) {
    v = lval_qexp();
  }


    // fill the empty sexp with any expressions within
    for (int i = 0; i < t->children_num; i++) {
      if (strcmp(t->children[i]->tag, "regex") == 0) { continue; } // skip the noise
      if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
      if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
      if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
      if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
      lval_add(v, read(t->children[i]));
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
read_num(mpc_ast_t *t) // Convert an AST to an LVAL containing a number
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
  mpc_parser_t *Qexp = mpc_new("qexp");
  mpc_parser_t *Input = mpc_new("input");

  mpca_lang(MPCA_LANG_DEFAULT,"\
  symbol: \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" | '+' | '-' | '*' | '/' | '%' ; \
  num: /-?[0-9]+/ ; \
  exp: <num> | <symbol> | <sexp> | <qexp> ;   \
  sexp: '(' <exp>* ')' ; \
  qexp: '{' <exp>* '}' ; \
  input: /^/ <exp>+ /$/ ;", Symbol, Num, Exp, Sexp, Qexp, Input);

  mpc_result_t r;

  while (1) {
    printf("> ");
    fgets(line, MAXLINE, stdin);

    if (mpc_parse("<stdin>", line, Input, &r)) {
      println_lval(read(r.output));
      println_lval(lval_eval(read(r.output)));

      /* mpc_ast_print(r.output); */
      mpc_ast_delete(r.output);
    } else { // catch syntax errors here
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  mpc_cleanup(6, Symbol, Num, Exp, Sexp, Qexp, Input);
  putchar('\n');
  free(line);
  return 0;
}
