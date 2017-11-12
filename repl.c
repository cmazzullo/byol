/*
  A small Lispoid REPL
*/

#include "mpc/mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define MAXLINE 1024

#define LASSERT(args, cond, fmt, ...)		\
  if (!(cond)) {				\
    lval *err = lval_err(fmt, ##__VA_ARGS__);	\
    lval_del(args);				\
    return err;					\
  }

#define LARGNUM(args, correctnum, funcname)				\
  if (args->count != correctnum) {					\
    lval *err = lval_err("ERROR: Function `%s` requires %d argument(s) (passed %d)!", \
			 funcname, correctnum, args->count);		\
    lval_del(args);							\
    return err;								\
  }

#define TYPEASSERT(args, recievedtype, righttype, funcname)		\
  if (recievedtype != righttype) {					\
    lval *err = lval_err("ERROR: Function `%s` requires argument(s) of type %s (passed %s)!", \
			 funcname,					\
			 ltype_name(righttype),				\
			 ltype_name(recievedtype));			\
    lval_del(args);							\
    return err;								\
  }


// Forward Declarations
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;
typedef lval*(*lbuiltin)(lenv *, lval *);

// Enums
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXP,
       LVAL_QEXP, LVAL_FN, LVAL_BOOL };

enum { TRUE, FALSE };

// Structs

struct lval { // lisp value
  int type;

  /* Basic */
  long num;
  short int bool;
  char* err;
  char* sym;

  /* Function */
  lbuiltin builtin;
  lenv *env;
  lval *formals;
  lval *body;

  /* Expression */
  int count;
  lval **cell;
};

struct lenv {
  int count;
  lval **vals;
  char **names;

  lenv *par;
};


// Function prototypes
lval *lval_lambda(lval *formals, lval *body);
lval *lval_fn(lbuiltin fn);
lval *lval_copy(lval *v);
lenv *lenv_new(void);
void lenv_del(lenv *env);
lenv *lenv_copy(lenv *env);
lval *lenv_get(lenv *env, char *name);
void lenv_put(lenv *env, lval *name, lval *v);
char *ltype_name(int t);
lval *builtin_lambda(lenv *e, lval *args);
lval *builtin_list(lenv *e, lval *args);
lval *builtin_head(lenv *e, lval *args);
lval *builtin_tail(lenv *e, lval *args);
lval *builtin_def(lenv *e, lval *args);
lval *builtin_put(lenv *e, lval *a);
lval *builtin_var(lenv *e, lval *a, char *func);
lval *builtin_join(lenv *e, lval *args);
void lenv_add_builtin(lenv *e, char *name, lbuiltin fn);
void lenv_add_builtins(lenv *e);
lval *builtin_add(lenv *e, lval *args);
lval *builtin_sub(lenv *e, lval *args);
lval *builtin_multiply(lenv *e, lval *args);
lval *builtin_divide(lenv *e, lval *args);
lval *builtin_eval(lenv *e, lval *args);
lval *builtin_cons(lenv *e, lval *args);
lval *builtin_len(lenv *e, lval *args);
void print_lval(lval *v);
void print_lval_sexp(lval *v, char open, char close);
lval *lval_eval_sexp(lenv *e, lval *v);
lval *builtin_def(lenv *e, lval *a);
lval *lval_pop(lval *v, int i);
lval *lval_add(lval *v, lval *x);
lval *builtin_op(lenv *e, char *op, lval *args);
lval *lval_take(lval *v, int i);
lval *lval_eval(lenv *e, lval *v);
lval * read_num(mpc_ast_t *t);
void lval_del(lval *v);

// CONSTRUCTORS
// Basic
lval *
lval_num(long x) // create new number
{
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval *
lval_err(char *fmt, ...) // create new error
{
  lval *v = malloc(sizeof(lval));

  va_list ap;
  va_start(ap, fmt);

  v->err = malloc(MAXLINE * sizeof(char));
  vsnprintf(v->err, MAXLINE, fmt, ap);
  va_end(ap);

  v->type = LVAL_ERR;
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

// Function
lval *
lval_lambda(lval *formals, lval *body)
{
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_FN;
  v->builtin = NULL; // no builtin, this is a user defined func

  v->formals = formals;
  v->body = body;
  v->env = lenv_new();

  return v;
}

lval *
lval_fn(lbuiltin fn)
{
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_FN;
  v->builtin = fn;
  return v;
}

// Expression
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

lval *
lval_bool(char bool)
{
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_BOOL;
  v->bool = bool;
  return v;
}

lval* // calls a user-defined function
lval_call(lenv *e, lval *f, lval *a)
{

  if (f->builtin)
    return f->builtin(e, a);

  // record argument counts
  int given = a->count;
  int total = f->formals->count;

  while (a->count > 0) {
    // Check for extra arguments
    if (f->formals->count == 0) {
      lval_del(a);
      return lval_err("Function passed too many arguments. "
		      "Got %i, expected %i.", given, total);
    }
    lval *formal = lval_pop(f->formals, 0);
    if (strcmp(formal->sym, "&") == 0) {
      if (f->formals->count != 1) {
	lval_del(a);
	return lval_err("Function format invalid: "
			"Symbol `&` not followed by a single symbol.");
      }
      /* Bind the next formal to the remaining arguments */
      lval *nsym = lval_pop(f->formals, 0);
      lenv_put(f->env, nsym, builtin_list(e, a));
      lval_del(formal); lval_del(nsym);
      break;
    }
    lval *val = lval_pop(a, 0);
    lenv_put(f->env, formal, val);
    lval_del(val);
    lval_del(formal);
  }

  lval_del(a);

  /* if '&' remains in formal list bind to empty list */
  if (f->formals->count > 0 &&
      strcmp(f->formals->cell[0]->sym, "&") == 0) {
    /* Check to ensure that & is not passed invalidly. */
    if (f->formals->count != 2) {
      return lval_err(
		      "Function format invalid: "
		      "Symbol '&' not followed by single symbol.");
    }

    /* Pop and delete '&' symbol */
    lval_del(lval_pop(f->formals, 0));

    lval *sym = lval_pop(f->formals, 0);
    lval *val = lval_qexp();

    lenv_put(e, sym, val);
    lval_del(sym); lval_del(val);
  }

  if (f->formals->count == 0) {
    f->env->par = e;
    return builtin_eval(f->env, lval_add(lval_sexp(), lval_copy(f->body)));
  } else {
    return lval_copy(f);
  }
}

// DESTRUCTOR
void
lval_del(lval *v) // free memory for an lval
{
  switch(v->type) {
  case LVAL_NUM: break;
  case LVAL_FN:
    if (!v->builtin) {
      lenv_del(v->env);
      lval_del(v->formals);
      lval_del(v->body);
    }
    break;
  case LVAL_ERR:
    free(v->err);
    break;
  case LVAL_SYM:
    free(v->sym);
    break;
  case LVAL_SEXP:
  case LVAL_QEXP:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }
    free(v->cell);
  }
  free(v);
}

lval *
lval_copy(lval *v)
{
  lval *x = malloc(sizeof(lval));
  x->type = v->type;

  switch (v->type) {
  case LVAL_NUM: x->num = v->num; break;
  case LVAL_FN:
    x->builtin = v->builtin;
    if (!v->builtin) {
      x->formals = lval_copy(v->formals);
      x->body = lval_copy(v->body);
      x->env = lenv_copy(v->env);
    }
    break;
  case LVAL_ERR:
    x->err = malloc((strlen(v->err) + 1) * sizeof(char));
    strcpy(x->err, v->err);
    break;
  case LVAL_SYM:
    x->sym = malloc((strlen(v->sym) + 1) * sizeof(char));
    strcpy(x->sym, v->sym);
    break;
  case LVAL_SEXP:
  case LVAL_QEXP:
    x->count = v->count;
    x->cell = malloc(x->count * sizeof(lval *));
    for (int i = 0; i < x->count; i++) {
      x->cell[i] = lval_copy(v->cell[i]);
    }
    break;
  }
  return x;
}

// ENVIRONMENT
lenv *
lenv_new(void)
{
  lenv *env = malloc(sizeof(lenv));
  env->count = 0;
  env->names = NULL;
  env->vals = NULL;
  env->par = NULL;
  return env;
}

void
lenv_del(lenv *env)
{
  for (int i = 0; i < env->count; i++) {
    free(env->names[i]);
    lval_del(env->vals[i]);
  }
  free(env->names);
  free(env->vals);
  free(env);
}

lenv *
lenv_copy(lenv *env)
{
  lenv *x = lenv_new();

  x->par = env->par;
  x->count = env->count;
  x->names = malloc(sizeof(char *) * env->count);
  x->vals = malloc(sizeof(lval *) * env->count);

  for (int i = 0; i < env->count; i++) {
    lenv_put(x, lval_sym(env->names[i]), env->vals[i]);
  }
  return x;
}

// Get a value from the environment
lval *
lenv_get(lenv *env, char *name)
{
  for (int i = 0; i < env->count; i++) {
    if (strcmp(name, env->names[i]) == 0)
      return lval_copy(env->vals[i]);
  }

  // search parent environment if variable isn't found
  if (env->par) {
    return lenv_get(env->par, name);
  }
  return lval_err("ERROR: Variable `%s` not found", name);
}

// Add a name/value pair to the environment
void
lenv_put(lenv *env, lval *name, lval *v)
{
  // Check if the name is taken
  for (int i = 0; i < env->count; i++) {
    if (strcmp(name->sym, env->names[i]) == 0) {
      lval_del(env->vals[i]);
      env->vals[i] = lval_copy(v);
      return;
    }
  }

  // Add size to the arrays in env by realloc'ing
  env->count++;
  env->names = realloc(env->names, env->count * sizeof(char *));
  env->vals = realloc(env->vals, env->count * sizeof(lval *));

  env->vals[env->count - 1] = lval_copy(v);
  env->names[env->count - 1] = malloc(strlen(name->sym) + 1);
  strcpy(env->names[env->count - 1], name->sym);
}

// Define a variable in the global environment (the one at the top of
// the parent chain
void
lenv_def(lenv *env, lval *k, lval *v)
{
  if (env->par) {
    lenv_def(env->par, k, v);
  } else {
    lenv_put(env, k, v);
  }
}

// Given an lval type, return its name
char *
ltype_name(int t)
{
  switch(t) {
  case LVAL_NUM: return "num";
  case LVAL_ERR: return "err";
  case LVAL_SYM: return "sym";
  case LVAL_SEXP: return "sexp";
  case LVAL_QEXP: return "qexp";
  case LVAL_FN: return "fn";
  default: return "unknown";
  }
}

// BUILTIN FUNCTIONS

lval *
builtin_lambda(lenv *e, lval *args)
{
  // Make sure `formals` contains only symbols
  lval *formals = lval_pop(args, 0);
 for (int i = 0; i < formals->count; i++) {
    TYPEASSERT(args, formals->cell[i]->type, LVAL_SYM, "lambda");
  }

  lval *body = lval_pop(args, 0);
  lval_del(args);

  return lval_lambda(formals, body);
}

void
lenv_add_builtin(lenv *e, char *name, lbuiltin fn)
{
  lval *v = lval_fn(fn);
  lenv_put(e, lval_sym(name), v);
  lval_del(v);
}

void
lenv_add_builtins(lenv *e)
{
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "def", builtin_def);
  lenv_add_builtin(e, "=", builtin_put);
  lenv_add_builtin(e, "len", builtin_len);
  lenv_add_builtin(e, "cons", builtin_cons);
  lenv_add_builtin(e, "\\", builtin_lambda);

  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_multiply);
  lenv_add_builtin(e, "/", builtin_divide);
}

lval *
builtin_add(lenv *e, lval *args)
{
  return builtin_op(e, "+", args);
}

lval *
builtin_sub(lenv *e, lval *args)
{
  return builtin_op(e, "-", args);
}

lval *
builtin_multiply(lenv *e, lval *args)
{
  return builtin_op(e, "*", args);
}

lval *
builtin_divide(lenv *e, lval *args)
{
  return builtin_op(e, "/", args);
}

lval *
builtin_list(lenv *e, lval *args) // Takes one or more args, returns a qexp containing them
{
  args->type = LVAL_QEXP;
  return args;
}

lval *
builtin_head(lenv *e, lval *args) // Returns the first element of a qexp
{
  LARGNUM(args, 1, "head");
  LASSERT(args, args->cell[0]->type == LVAL_QEXP,
	  "ERROR: Cannot take head of a non-QEXP! (recieved `%s`)",
	  ltype_name(args->cell[0]->type));
  LASSERT(args, args->cell[0]->count != 0,
	  "ERROR: Cannot take head of an empty QEXP!");

  lval *v = lval_take(args, 0);
  while (v->count > 1) lval_del(lval_pop(v, 1)); // pop & delete the second lval leaving only the head
  return v;
}

lval *
builtin_tail(lenv *e, lval *args) // Returns the cdr of a qexp
{
  LARGNUM(args, 1,"tail");
  LASSERT(args, args->cell[0]->type == LVAL_QEXP,
	  "ERROR: Cannot take tail of a non-QEXP!");
  LASSERT(args, args->cell[0]->count != 0,
	  "ERROR: Cannot take tail of an empty QEXP!");

  lval *v = lval_take(args, 0);
  lval_del(lval_pop(v, 0)); // pop & delete the first lval leaving the cdr
  return v;
}

lval *
builtin_join(lenv *e, lval *args) // Join together one or more qexps
{
  // LARGNUM(args, >0, "join");
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
builtin_eval(lenv *e, lval *args)
{
  LARGNUM(args, 1, "eval");
  LASSERT(args, args->cell[0]->type == LVAL_QEXP,
	  "ERROR: Cannot eval a non-QEXP!");
  lval *x = lval_take(args, 0);
  x->type = LVAL_SEXP;
  return lval_eval(e, x);
}

lval *
builtin_cons(lenv *e, lval *args)
{
  LARGNUM(args, 2, "cons");
  LASSERT(args, args->cell[1]->type == LVAL_QEXP,
	  "ERROR: Cons function requires a QEXP as a second argument");
  lval *q = lval_qexp();
  lval_add(q, args->cell[0]);
  lval *a = lval_pop(args, 1);
  while (a->count > 0) lval_add(q, lval_pop(a, 0));
  return q;
}

lval *
builtin_len(lenv *e, lval *args)
{
  LARGNUM(args, 1, "len");
  LASSERT(args, args->cell[0]->type == LVAL_QEXP,
	  "ERROR: Argument to len function was not a QEXP");

  return lval_num(args->cell[0]->count);
}

// PRINTING
void
print_lval(lval *v)
{
  switch (v->type) {
  case LVAL_NUM: printf("%li", v->num); break;
  case LVAL_FN:
    if (v->builtin) {
      printf("<builtin>");
    } else {
      printf("(\\ "); print_lval(v->formals);
      putchar(' '); print_lval(v->body); putchar(')');
    }
    break;
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
lval_eval(lenv *e, lval *v) // evaluates an lval recursively
{
  if (v->type == LVAL_SYM) { // look up symbols in the environment
    lval* x = lenv_get(e, v->sym);
    lval_del(v);
    return x;
  }

  if (v->type == LVAL_SEXP)
    return lval_eval_sexp(e, v);

  return v;
}

lval *
lval_eval_sexp(lenv *e, lval *v)
{
  // evaluate children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
  }

  // catch errors
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR)
      return v->cell[i];
  }

  if (v->count == 0)
      return v; // return `()` as-is

  lval *first = lval_pop(v, 0); // pops off the first element

  if (first->type != LVAL_FN) {
    lval_del(first);
    lval_del(v);
    return lval_err("ERROR: Invalid function");
  }

  lval *result = lval_call(e, first, v);
  // memory leak here?
  //lval_del(first);
  return result;
}


lval *
builtin_op(lenv *e, char *op, lval *args) // apply fn to args
{
  for (int i = 0; i < args->count; i++) {
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

// Global variable definition
lval *
builtin_def(lenv *e, lval *a)
{
  return builtin_var(e, a, "def");
}

// Local variable definition
lval *
builtin_put(lenv *e, lval *a)
{
  return builtin_var(e, a, "put");
}

// Variable definition helper function
lval *
builtin_var(lenv *e, lval *a, char *func)
{
  LASSERT(a, a->count > 0,
	  "ERROR: Not enough arguments to `%s`!", func);
  lval *names = lval_pop(a, 0);
  LASSERT(a, a->count == names->count,
	  "ERROR: Number of names does not match number of values!");
  LASSERT(names, names->type == LVAL_QEXP,
	  "ERROR: Function `%s` not passed a QEXP as argument 1!", func)
  // a is a qexp containing a list of names + a number of values
  for (int i = 0; i < a->count; i++) {
    LASSERT(a, names->cell[i]->type == LVAL_SYM,
	    "ERROR: Function `%s` requires a list of symbols!", func);
  }

  for (int i = 0; i < a->count; i++) {
    if (strcmp(func, "def") == 0) {
      lenv_def(e, lval_copy(names->cell[i]), lval_copy(a->cell[i]));
    } else if (strcmp(func, "put") == 0) {
      lenv_put(e, lval_copy(names->cell[i]), lval_copy(a->cell[i]));
    }
  }

  lval_del(a);
  return lval_sexp();
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
  // if the tree is the root, return its first child
  if (strcmp(t->tag, ">") == 0) {
    return read(t->children[1]);
  }

  if (strstr(t->tag, "num")) { return read_num(t); }
  if (strstr(t->tag, "sym")) { return lval_sym(t->contents); }

  // if the tree a sexp then create an empty sexp
  lval *v;
  if (strstr(t->tag, "sexp")) {
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

lval * // append x to list v
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
  num: /-?[0-9]+/ ;							\
  symbol: /[a-zA-Z0-9*+\\-\\/\\\\_=<>!&]+/ ;				\
  sexp: '(' <exp>* ')' ;						\
  qexp: '{' <exp>* '}' ;						\
  exp: <num> | <symbol> | <sexp> | <qexp> ; \
  input: /^/ <exp>? /$/ ;", Num, Symbol, Sexp, Qexp, Exp, Input);

  mpc_result_t r;

  lenv* e = lenv_new(); // create the environment
  lenv_add_builtins(e);

  while (1) {
    printf("> ");
    fgets(line, MAXLINE, stdin);

    if (mpc_parse("<stdin>", line, Input, &r)) {
      println_lval(read(r.output));
      println_lval(lval_eval(e, read(r.output)));
      mpc_ast_delete(r.output);
    } else { // catch syntax errors here
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  lenv_del(e);

  mpc_cleanup(6, Symbol, Num, Exp, Sexp, Qexp, Input);
  putchar('\n');
  free(line);
  return 0;
}
