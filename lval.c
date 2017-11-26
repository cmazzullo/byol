#include "lval.h"
#include "list.h"
#include "mpc/mpc.h"
#include "environment.h"
#include "map.h"
#include "builtin.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h> // for boolean values

#define MAXERR 1024 // Maximum error string length


struct lval { // lisp value
  int type;

  /* Basic */
  long num;
  bool boolean;
  char* err;
  char* sym;

  /* Function/Macro */
  lbuiltin builtin;
  lenv *env;
  lval *formals;
  lval *body;

  /* Expression */
  list *cell;

  /* Dict */
  map *dict;
};

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
lval *
lval_num(long x) // create new number
{
  lval *v = calloc(1, sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval *
lval_dict(void)
{
  lval *v = calloc(1, sizeof(lval));
  v->type = LVAL_DICT;
  v->dict = map_new();
  return v;
}

lval *
lval_bool(bool boolean)
{
  lval *v = calloc(1, sizeof(lval));
  v->type = LVAL_BOOL;
  v->boolean = boolean;
  return v;
}

lval *
lval_err(char *fmt, ...) // create new error
{
  lval *v = calloc(1, sizeof(lval));
  va_list ap;
  va_start(ap, fmt);
  v->err = calloc(1, MAXERR * sizeof(char));
  vsnprintf(v->err, MAXERR, fmt, ap);
  va_end(ap);
  v->type = LVAL_ERR;
  return v;
}

lval *
lval_sym(char *sym) // create new symbol
{
  lval *v = calloc(1, sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = calloc(1, strlen(sym) + 1);
  strcpy(v->sym, sym);
  return v;
}

// Returns a user-defined function w/ env, formals and body
lval *
lval_lambda(lenv *env, lval *formals, lval *body)
{
  lval *v = calloc(1, sizeof(lval));
  v->type = LVAL_FN;
  v->builtin = NULL; // no builtin, this is a user defined func
  v->formals = formals;
  v->body = body;
  v->env = lenv_new(env);
  return v;
}

lval *
lval_macro(lenv *e, lval *formals, lval *body) // create new user-defined macro
{
  lval *v = lval_lambda(e, formals, body);
  v->type = LVAL_MACRO;
  return v;
}

lval *
lval_builtin_function(lenv *e, lbuiltin fn)
{
  lval *v = calloc(1, sizeof(lval));
  v->type = LVAL_FN;
  v->formals = NULL;
  v->body = NULL;
  v->env = e;
  v->builtin = fn;
  return v;
}

lval *
lval_builtin_macro(lenv *e, lbuiltin fn) // create new empty macro
{
  lval *v = lval_builtin_function(e, fn);
  v->type = LVAL_MACRO;
  return v;
}

lval *
lval_sexp(void) // create new empty sexp
{
  lval *v = calloc(1, sizeof(lval));
  v->type = LVAL_SEXP;
  v->cell = NULL;
  return v;
}

void
lval_del(lval *v) // free memory for an lval
{
  switch(v->type) {
  case LVAL_DICT:
    map_delete(v->dict);
    break;
  case LVAL_NUM:
    break;
  case LVAL_MACRO:
  case LVAL_FN:
    if (!v->builtin) {
      lenv_delete(v->env);
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
    list_delete(v->cell);
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
      x = lval_builtin_macro(lenv_copy(v->env), v->builtin);
    } else {
      x = lval_macro(lenv_copy(v->env), lval_copy(v->formals), lval_copy(v->body));
    }
    break;
  case LVAL_FN:
    if (v->builtin) {
      x = lval_builtin_function(lenv_copy(v->env), v->builtin);
    } else {
      x = lval_lambda(lenv_copy(v->env), lval_copy(v->formals), lval_copy(v->body));
    }
    break;
  case LVAL_ERR:
    x = lval_err(v->err);
    break;
  case LVAL_SYM:
    x = lval_sym(get_sym(v));
    break;
  case LVAL_SEXP:
    x = lval_sexp();
    x->cell = list_copy(v->cell);
    break;
  }
  return x;
}

bool
lval_equal(lval *x, lval *y)
{
  if (get_type(x) != get_type(y)) { return lval_bool(false); }
  switch (get_type(x)) {
  case LVAL_BOOL:
    return get_bool(x) == get_bool(y);
    break;
  case LVAL_NUM:
    return get_num(x) == get_num(y);
    break;
  case LVAL_ERR:
    return strcmp(get_err(x), get_err(y)) == 0;
    break;
  case LVAL_SYM:
    return strcmp(get_sym(x), get_sym(y)) == 0;
    break;
  case LVAL_SEXP:
    if (get_count(x) == 0 && get_count(y) == 0) {
      return true;
    } else if (get_count(x) == 0 || get_count(y) == 0) {
      return false;
    } else {
      return lval_equal(lval_first(x), lval_first(y)) &&
	lval_equal(lval_rest(x), lval_rest(y));
    }
    break;
  case LVAL_MACRO:
  case LVAL_FN:
    if (get_builtin(x) && get_builtin(y)) {
      return get_builtin(x) == get_builtin(y);
    } else if (!get_builtin(x) && !get_builtin(y)) {
      return lval_equal(get_formals(x), get_formals(y)) &&
	lval_equal(get_body(x), get_body(y));
    } else {
      return false;
    }
    break;
  case LVAL_DICT:
    return false; // TODO fix this
    break;
  }
  return false;
}

void
print_lval(lval *v)
{
  if (!v) { printf("print_lval recieved `NULL`\n"); return; }
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
      //print_lval(v->env);
      printf("(\\ "); print_lval(v->formals);
      putchar(' '); print_lval(v->body); putchar(')');
    }
    break;
  case LVAL_ERR: printf(v->err); break;
  case LVAL_SYM: printf(v->sym); break;
  case LVAL_SEXP:
    putchar('(');
    list_print(v->cell); // print the internal linked list of lvals
    printf(")");
    break;
  case LVAL_BOOL:
    if (v->boolean)
      printf("true");
    else printf("false");
    break;
  }
}

// FUNCTIONS

lval *lval_first(lval *l) { return list_first(l->cell); }

lval *lval_rest(lval *l)
{
  if (!get_count(l)) return lval_err("ERROR: `lval_rest` called on empty list");

  lval *v = lval_sexp();
  v->cell = list_rest(l->cell);
  return v;
}

lval * /* Get nth element of l, 0-indexed */
lval_nth(lval *l, int n)
{
  if (n == 0) {
    return lval_first(l);
  } else {
    return lval_nth(lval_rest(l), n - 1);
  }
}

lval * // append x to list v
lval_cons(lval *v, lval *x)
{
  v->cell = list_cons(x, v->cell);
  return v;
}

lval *
lval_call(lval* fn, lval *args)
{
  if (fn->builtin) return fn->builtin(fn->env, args);
  lenv *new_e = lenv_new(fn->env);
  lval *formals = fn->formals;
  while (!empty(args)) {
    if (empty(formals)) {
      return lval_err("ERROR: Function passed too many arguments.");
    }
    lenv_set(new_e, lval_first(formals), lval_first(args));
    formals = lval_rest(formals);
    args = lval_rest(args);
  }
  if (empty(formals)) {
    return lval_eval(new_e, get_body(fn));
  } else { // this allows currying:
    return lval_lambda(new_e, formals, get_body(fn));
  }
}

lval *
lval_eval_sexp(lenv *e, lval *s)
{
  if (is_empty(s)) { return s; } // Return `()`
  lval *first = lval_eval(e, lval_first(s));
  if (get_type(first) == LVAL_MACRO) { return lval_call(first, lval_rest(s)); }
  if (get_type(first) == LVAL_ERR) { return first; }
  if (get_type(first) != LVAL_FN) {
    return lval_err("ERROR: First element of a SEXP must be a function or macro, recieved `%s`",
		    ltype_name(get_type(first)));
  }
  // Now we know we have a function as the first element
  lval *rest = lval_rest(s);
  lval *children = lval_sexp();

  while (!empty(rest)) {
    lval *child = lval_eval(e, lval_first(rest));
    if (get_type(child) == LVAL_ERR) { return child; } // check for errors
    lval_cons(children, child); // accumulate evalled children
    rest = lval_rest(rest);
  }

  return lval_call(first, children);
}

lval *
lval_eval(lenv *e, lval *v) // evaluates an lval recursively
{
  switch (v->type) {
  case LVAL_SYM:
    return lenv_get(e, v);
    break;
  case LVAL_SEXP:
    return lval_eval_sexp(e, v);
    break;
  default: /* Numbers, functions, errors and bools evaluate to themselves */
    return lval_copy(v);
    break;
  }
}

//DICT FUNCTIONS

// Get a value from a dict
lval *
lval_get(lval *dict, lval *name)
{
  TYPEASSERT(dict, dict->type, LVAL_DICT, "lval_get");
  lval *v;
  if ((v = map_get(dict->dict, name))) {
    return v;
  } else {
    return lval_err("ERROR: Variable `%s` not found", name);
  }
}

// Add a name/value pair to a dict
lval *
lval_put(lval *dict, lval *name, lval *v)
{
  map_add(dict->dict, name, v);
  return dict;
}


// Accessor

int get_num(lval *l) {return l->num;}
bool get_bool(lval *l) {return l->boolean;}
char *get_err(lval *l) {return l->err;}
char *get_sym(lval *l) {return l->sym;}
int get_type(lval *l) {return l->type;}
int get_count(lval *l){ return list_count(l->cell); }
bool is_empty(lval *l){ return list_count(l->cell) == 0; }
bool empty(lval *l) { return get_count(l) == 0; }

lbuiltin get_builtin(lval *x) {return x->builtin;}

lval *get_formals(lval *fn) { return fn->formals; }
lenv *get_env(lval *fn) { return fn->env; }
lval *get_body(lval *fn) { return fn->body; }
