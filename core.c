#include "mpc/mpc.h"
#include "map.h"
#include "core.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h> // for boolean values

#define MAXERR 1024 // Maximum error string length

// TYPES ////////////////////////////////////////////////////////////////////////////////

struct lval { // lisp value
  int type;

  /* Basic */
  long num;
  bool boolean;
  char* err;
  char* sym;

  /* Function/Macro */
  lbuiltin builtin;
  lval *env;
  lval *formals;
  lval *body;

  /* Expression */
  int count;
  lval **cell;

  /* Dict */
  map *dict;
};


// LVALS ////////////////////////////////////////////////////////////////////////////////

char * /* Given an lval type, return its name */
ltype_name(int t)
{
  switch(t) {
  case LVAL_NUM: return "num";
  case LVAL_ERR: return "err";
  case LVAL_SYM: return "sym";
  case LVAL_SEXP: return "sexp";
  case LVAL_MACRO: return "macro";
  case LVAL_FN: return "fn";
  case LVAL_BOOL: return "bool";
  case LVAL_DICT: return "dict";
  default: return "unknown";
  }
}

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
lval_dict(void)
{
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_DICT;
  v->dict = map_new();
  return v;
}

lval *
lval_bool(bool boolean)
{
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_BOOL;
  v->boolean = boolean;
  return v;
}

lval *
lval_err(char *fmt, ...) // create new error
{
  lval *v = malloc(sizeof(lval));

  va_list ap;
  va_start(ap, fmt);

  v->err = malloc(MAXERR * sizeof(char));
  vsnprintf(v->err, MAXERR, fmt, ap);
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
lval_lambda(lval *env, lval *formals, lval *body)
{
  TYPEASSERT(env, env->type, LVAL_DICT, "lval_lambda");
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_FN;
  v->builtin = NULL; // no builtin, this is a user defined func

  v->formals = formals;
  v->body = body;
  v->env = lval_copy(env);
  return v;
}

lval *
lval_fn(lbuiltin fn)
{
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_FN;
  v->formals = NULL;
  v->body = NULL;
  v->builtin = fn;
  return v;
}

lval *
lval_macro(lbuiltin fn) // create new empty macro
{
  lval *v = lval_fn(fn);
  v->type = LVAL_MACRO;
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

// DESTRUCTOR
void
lval_del(lval *v) // free memory for an lval
{
  switch(v->type) {
  case LVAL_DICT:
    map_del(v->dict);
    break;
  case LVAL_NUM:
    break;
  case LVAL_MACRO:
  case LVAL_FN:
    if (!v->builtin) {
      lval_del(v->env);
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
  lval *x;
  switch (v->type) {
  case LVAL_DICT:
    x = lval_dict();
    x->dict = map_copy(v->dict);
    break;
  case LVAL_BOOL:
    x = lval_bool(v->boolean);
    break;
  case LVAL_NUM:
    x = lval_num(v->num);
    break;
  case LVAL_MACRO:
    if (v->builtin) {
      x = lval_macro(v->builtin);
    } else {
      x = lval_lambda(lval_copy(v->env), lval_copy(v->formals), lval_copy(v->body));
    }
    break;
  case LVAL_FN:
    if (v->builtin) {
      x = lval_fn(v->builtin);
    } else {
      x = lval_lambda(lval_copy(v->env), lval_copy(v->formals), lval_copy(v->body));
    }
    break;
  case LVAL_ERR:
    x = lval_err(v->err);
    break;
  case LVAL_SYM:
    x = lval_sym(v->sym);
    break;
  case LVAL_SEXP:
    x = lval_sexp();
    for (int i = 0; i < v->count; i++) {
      lval_add(x, lval_copy(v->cell[i]));
    }
    break;
  }

  return x;
}

lval* // calls a user-defined function
lval_call(lval *e, lval *f, lval *a)
{
  TYPEASSERT(e, e->type, LVAL_DICT, "lval_call");
  LASSERT(a, f->type == LVAL_FN || f->type == LVAL_MACRO,
	  "ERROR: `lval_call` recieved %s instead of a function or macro",
	  ltype_name(f->type));
  TYPEASSERT(a, a->type, LVAL_SEXP, "lval_call");
  // new sub-environment

  // BUILTIN FUNCTION
  if (f->builtin) {
    return f->builtin(e, a);
  }

  lval *new_env = lval_copy(f->env);

  // USER-DEFINED FUNCTION
  if (f->formals->count < a->count) {     // Check for extra arguments
    lval_del(a);
    return lval_err("Function passed too many arguments. ");
  }

  while (a->count > 0) {
    lval *formal = lval_pop(f->formals, 0);

    if (strcmp(formal->sym, "&") == 0) { // if we encounter the `&` sign (variable parameters)
      if (f->formals->count != 1) {
	lval_del(a);
	lval_del(new_env);
	return lval_err("Function format invalid: "
			"Symbol `&` not followed by a single symbol.");
      }
      /* Bind the next formal to the remaining arguments */
      lval *nsym = lval_pop(f->formals, 0);
      lval_put(new_env, nsym, builtin_list(e, a));
      lval_del(formal);
      lval_del(nsym);
      break;
    }

    lval *val = lval_pop(a, 0);
    lval_put(new_env, lval_copy(formal), lval_copy(val)); // Here's where we bind formals to values
    lval_del(val);
    lval_del(formal);
  }

  lval_del(a);

  /* if '&' remains in formal list bind to empty list */
  if (f->formals->count > 0 &&
      strcmp(f->formals->cell[0]->sym, "&") == 0) {
    /* Check to ensure that & is not passed invalidly. */
    if (f->formals->count != 2) {
      lval_del(new_env);
      return lval_err("Function format invalid: "
		      "Symbol '&' not followed by single symbol.");
    }
    /* Pop and delete '&' symbol */
    lval_del(lval_pop(f->formals, 0));
    lval *sym = lval_pop(f->formals, 0);
    lval *val = lval_sexp();
    lval_put(e, sym, val);
    lval_del(sym); lval_del(val);
  }

  f->env = new_env;
  if (f->formals->count == 0) {
    return lval_eval(f->env, f->body);
  } else {
    f->env = new_env;
    return lval_copy(f);
  }
}


// PRINTING

void
print_lval(lval *v)
{
  switch (v->type) {
  case LVAL_DICT:
    map_print(v->dict);
    break;
  case LVAL_NUM: printf("%li", v->num); break;
  case LVAL_MACRO:
    if (v->builtin) {
      printf("<builtin macro>");
    } else {
      //print_lval(v->env);
      printf("(macro "); print_lval(v->formals);
      putchar(' '); print_lval(v->body); putchar(')');
    }
    break;
  case LVAL_FN:
    if (v->builtin) {
      printf("<builtin fn>");
    } else {
      print_lval(v->env);
      printf("(\\ "); print_lval(v->formals);
      putchar(' '); print_lval(v->body); putchar(')');
    }
    break;
  case LVAL_ERR: printf(v->err); break;
  case LVAL_SYM: printf(v->sym); break;
  case LVAL_SEXP: print_lval_sexp(v, '(', ')'); break;
  case LVAL_BOOL:
    if (v->boolean)
      printf("true");
    else printf("false");
    break;
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


// EVALUATION
lval *
lval_eval(lval *e, lval *v) // evaluates an lval recursively
{
  lval *result;
  /* Numbers, functions, errors and bools evaluate to themselves */
  switch (v->type) {
  case LVAL_DICT:
  case LVAL_NUM:
  case LVAL_BOOL:
  case LVAL_ERR:
  case LVAL_FN:
  case LVAL_MACRO:
    result = lval_copy(v);
    break;
  case LVAL_SYM:
    result = lval_get(e, v->sym);
    break;
  case LVAL_SEXP:
    result = lval_eval_sexp(e, v);
    break;
  }
  return result;
}

lval *
lval_eval_sexp(lval *e, lval *v)
{
  if (v->count == 0) {
    return lval_copy(v); // return `()` as-is
  }

  lval *first = lval_eval(e, lval_pop(v, 0)); // pops off the first element
  if (first->type == LVAL_MACRO) { // Short-circuit evaluation
    return lval_call(e, first, v);
  } else if (first->type == LVAL_ERR) {
    return first; // catch errors in first element
  } else if (first->type != LVAL_FN) {
    lval *result = lval_err("ERROR: First element of a SEXP must be a function or macro, recieved `%s`",
			    ltype_name(first->type));
    lval_del(first);
    return result;
  }

  // evaluate children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
  }

  // catch errors in the rest of the args
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR)
      return v->cell[i];
  }

  return lval_call(e, first, v);
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

lval * // append x to list v
lval_add(lval *v, lval *x)
{
  ++(v->count);
  v->cell = realloc(v->cell, sizeof(lval *) * (v->count)); // resize the cell array
  v->cell[v->count - 1] = x; // append x to the end of the cell array
  return v;
}

//DICT FUNCTIONS

// Get a value from a dict
lval *
lval_get(lval *dict, char *name)
{
  lval *v;
  if ((v = map_get(dict->dict, name))) {
    return v;
  } else {
    return lval_err("ERROR: Variable `%s` not found", name);
  }
}

// Add a name/value pair to a dict
void
lval_put(lval *dict, lval *name, lval *v)
{
  map_add(dict->dict, name->sym, v);
}

// BUILTINS ////////////////////////////////////////////////////////////////////////////////

/* Adds a builtin to the environment */
void
env_add_builtin(lval *e, char *name, lbuiltin fn)
{
  lval *v = lval_fn(fn);
  lval_put(e, lval_sym(name), lval_copy(v));
  lval_del(v);
}

/* Adds a macro to the environment */
void
env_add_builtin_macro(lval *e, char *name, lbuiltin fn)
{
  lval *v = lval_macro(fn);
  lval_put(e, lval_sym(name), lval_copy(v));
  lval_del(v);
}

void
env_add_builtins(lval *e)
{
  env_add_builtin_macro(e, "\\", builtin_lambda);
  env_add_builtin_macro(e, "macro", builtin_macro);
  env_add_builtin_macro(e, "if", builtin_if);
  env_add_builtin_macro(e, "def", builtin_def);

  env_add_builtin(e, "list", builtin_list);
  env_add_builtin(e, "head", builtin_head);
  env_add_builtin(e, "tail", builtin_tail);
  env_add_builtin(e, "eval", builtin_eval);
  env_add_builtin(e, "join", builtin_join);
  env_add_builtin(e, "len", builtin_len);
  env_add_builtin(e, "cons", builtin_cons);
  env_add_builtin(e, "=", builtin_equal);

  env_add_builtin(e, "+", builtin_add);
  env_add_builtin(e, "-", builtin_sub);
  env_add_builtin(e, "*", builtin_multiply);
  env_add_builtin(e, "/", builtin_divide);
  env_add_builtin(e, ">", builtin_greaterthan);
}

lval *
builtin_macro(lval *e, lval *args)
{
  lval *m = builtin_lambda(e, args);
  m->type = LVAL_MACRO;
  return m;
}

/* Given args (formals, body), returns a function */
lval *
builtin_lambda(lval *e, lval *args)
{
  ARGNUM(args, 2, "<builtin lambda>");
  // Make sure `formals` contains only symbols
  lval *formals = lval_pop(args, 0);
  for (int i = 0; i < formals->count; i++) {
    TYPEASSERT(args, formals->cell[i]->type, LVAL_SYM, "builtin_lambda");
  }
  lval *body = lval_take(args, 0);
  return lval_lambda(e, formals, body);
}

/* Operations on numbers */
lval *builtin_add(lval *e, lval *args) {return builtin_op(e, "+", args);}
lval *builtin_sub(lval *e, lval *args) {return builtin_op(e, "-", args);}
lval *builtin_multiply(lval *e, lval *args) {return builtin_op(e, "*", args);}
lval *builtin_divide(lval *e, lval *args) {return builtin_op(e, "/", args);}

lval *
builtin_greaterthan(lval *e, lval *args)
{
  for (int i = 0; i < args->count; i++) {
    LASSERT(args, args->cell[i]->type == LVAL_NUM,
	    "ERROR: Cannot operate on non-number!");
  }
  ARGNUM(args, 2, ">"); /* Make sure we have 2 args */
  lval *l = lval_pop(args, 0);
  lval *r = lval_pop(args, 0);
  lval *result = lval_bool(l->num > r->num);
  lval_del(args);
  return result;
}

lval *builtin_lessthan(lval *e, lval *args) {return builtin_op(e, "<", args);}

lval *builtin_equal(lval *e, lval *args) {
  ARGNUM(args, 2, "=");
  lval *x = lval_pop(args, 0);
  lval *y = lval_pop(args, 0);
  if (x->type != y->type) {
    lval_del(args);
    return lval_bool(false);
  }
  bool result;
  switch (x->type) {
  case LVAL_BOOL:
    result = x->boolean == y->boolean;
    break;
  case LVAL_NUM:
    result = x->num == y->num;
    break;
  case LVAL_ERR:
    result = strcmp(x->err, y->err) == 0;
    break;
  case LVAL_SYM:
    result = strcmp(x->sym, y->sym) == 0;
    break;
  case LVAL_SEXP:
    if (x->count != y->count) {
      result = false; break;
    }
    result = true;
    for (int i = 0; i < x->count; i++) {
      lval *new_args = lval_sexp();
      lval_add(new_args, x->cell[i]);
      lval_add(new_args, y->cell[i]);
      if (!builtin_equal(e, new_args)->boolean) /* if the cells aren't equal */
	result = false; /* return false */
      lval_del(new_args);
    }
    break;
  case LVAL_MACRO:
  case LVAL_FN:
    if (x->builtin && y->builtin) {
      result = x->builtin == y->builtin;
    } else if (!x->builtin && !y->builtin) {
      lval *formals = lval_sexp();
      lval_add(formals, x->formals);
      lval_add(formals, y->formals);
      lval *bodys = lval_sexp();
      lval_add(bodys, x->body);
      lval_add(bodys, y->body);
      result = builtin_equal(e, formals)->boolean &&
	builtin_equal(e, bodys)->boolean;
    } else {
      result = false;
    }
  }
  lval_del(args);
  return lval_bool(result);

  return NULL;
}

/* Operations on lists */
lval *
builtin_list(lval *e, lval *args) // Takes one or more args, returns a sexp containing them
{
  args->type = LVAL_SEXP;
  return args;
}

lval *
builtin_head(lval *e, lval *args) // Returns the first element of a sexp
{
  ARGNUM(args, 1, "head");
  LASSERT(args, args->cell[0]->type == LVAL_SEXP,
	  "ERROR: Cannot take head of a non-SEXP! (recieved `%s`)",
	  ltype_name(args->cell[0]->type));
  LASSERT(args, args->cell[0]->count != 0,
	  "ERROR: Cannot take head of an empty SEXP!");

  lval *v = lval_take(args, 0);
  while (v->count > 1) lval_del(lval_pop(v, 1)); // pop & delete the second lval leaving only the head
  return lval_take(v, 0);
}

lval *
builtin_tail(lval *e, lval *args) // Returns the cdr of a sexp
{

  ARGNUM(args, 1, "tail");
  LASSERT(args, args->cell[0]->type == LVAL_SEXP,
	  "ERROR: Cannot take tail of a non-SEXP!");
  LASSERT(args, args->cell[0]->count > 0,
	  "ERROR: Cannot take tail of an empty SEXP!");

  lval *v = lval_take(args, 0);
  lval_del(lval_pop(v, 0)); // pop & delete the first lval leaving the cdr
  lval *result = lval_copy(v);
  lval_del(v);
  return result;
}

lval *
builtin_join(lval *e, lval *args) // Join together one or more sexps
{
  // ARGNUM(args, >0, "join");
  LASSERT(args, args->count != 0,
	  "ERROR: Function join passed 0 arguments!");
  lval *v = lval_sexp();
  while (args->count > 0) {
    lval *a = lval_pop(args, 0);
    LASSERT(a, a->type == LVAL_SEXP,
	    "ERROR: Cannot take tail of a non-SEXP!");
    while (a->count > 0) lval_add(v, lval_pop(a, 0));
    lval_del(a);
  }

  lval_del(args);
  return v;
}

lval *
builtin_cons(lval *e, lval *args)
{
  ARGNUM(args, 2, "cons");
  LASSERT(args, args->cell[1]->type == LVAL_SEXP,
	  "ERROR: Cons function requires a SEXP as a second argument");
  lval *q = lval_sexp();
  lval_add(q, args->cell[0]);
  lval *a = lval_pop(args, 1);
  while (a->count > 0) lval_add(q, lval_pop(a, 0));
  return q;
}

lval *
builtin_len(lval *e, lval *args)
{
  ARGNUM(args, 1, "len");
  LASSERT(args, args->cell[0]->type == LVAL_SEXP,
	  "ERROR: Argument to len function was not a SEXP");

  return lval_num(args->cell[0]->count);
}

/* Function: eval sexp
The all-important EVAL */
lval *
builtin_eval(lval *e, lval *args)
{
  ARGNUM(args, 1, "eval");
  /* LASSERT(args, args->cell[0]->type == LVAL_SEXP, */
  /* 	  "ERROR: Cannot eval a non-SEXP!"); */

  lval *x = lval_take(args, 0);
  return lval_eval(e, x);
}

lval *
builtin_op(lval *e, char *op, lval *args) // apply fn to args
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

/* Macro: if cond body else-body */
lval *
builtin_if(lval *e, lval *args)
{
  ARGNUM(args, 3, "if");

  lval *cond = lval_eval(e, lval_pop(args, 0));
  if (cond->type != LVAL_BOOL) {
    return lval_err("ERROR: First argument to `if` must be a BOOL, recieved `%s`.",
		    ltype_name(cond->type));
  }
  lval *body = lval_pop(args, 0);
  lval *elsebody = lval_pop(args, 0);
  lval *result;
  if (cond->boolean) {
  result = lval_eval(e, body);
  }
  else {
    result = lval_eval(e, elsebody);
  }
  lval_del(args);
  return result;
}


// Local variable definition
lval *
builtin_def(lval *e, lval *a)
{
  ARGNUM(a, 2, "def");
  lval *name = lval_pop(a, 0);
  LASSERT(name, name->type == LVAL_SYM,
	  "ERROR: Function `%s` not passed a SYMBOL as argument 1!", "def");

  lval *value = lval_eval(e, lval_take(a, 0));
  if (value->type == LVAL_ERR)
    return value;
  lval_put(e, name, value);
  return value;
}


// READ ////////////////////////////////////////////////////////////////////////////////

lval *
read(mpc_ast_t *t) // convert the AST into a sexp
{
  // if the tree is the root, return its first child
  if (strcmp(t->tag, ">") == 0) {
    return read(t->children[1]);
  }
  if (strstr(t->tag, "bool")) { return read_bool(t); }
  if (strstr(t->tag, "num")) { return read_num(t); }
  if (strstr(t->tag, "sym")) { return lval_sym(t->contents); }

  // if the tree a sexp then create an empty sexp
  lval *v;
  if (strstr(t->tag, "sexp")) {
    v = lval_sexp();
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
read_num(mpc_ast_t *t) // Convert an AST to an LVAL containing a number
{
  long x = strtol(t->contents, NULL, 10);
  if (errno != ERANGE) return lval_num(x);
  else return lval_err("ERROR: Invalid number!");
}

lval *
read_bool(mpc_ast_t *t)
{
  if (strcmp(t->contents, "true") == 0) {
    return lval_bool(true);
  } else {
    return lval_bool(false);
  }
}
