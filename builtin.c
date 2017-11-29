/*
  Builtin functions
 */

#include "lval.h"
#include "read.h"
#include "list.h"
#include "builtin.h"
#include "structs.h"
#include "environment.h"

#include <string.h>
#include <stdlib.h>

enum { FUNCTION, MACRO };

/* Adds a builtin to the environment */
void
env_add_builtin(lenv *e, char *name, lbuiltin fn, int type)
{
  lval *v;
  switch (type) {
  case FUNCTION:
    v = lval_builtin_function(e, fn);
    break;
  case MACRO:
    v = lval_builtin_macro(e, fn);
    break;
  }
  lenv_set(e, lval_sym(name), v);
}

void
env_add_builtins(lenv *e)
{
  env_add_builtin(e, "\\", builtin_lambda, MACRO);
  env_add_builtin(e, "macro", builtin_macro, MACRO);
  env_add_builtin(e, "if", builtin_if, MACRO);
  env_add_builtin(e, "def", builtin_def, MACRO);
  env_add_builtin(e, "progn", builtin_progn, MACRO);

  env_add_builtin(e, "list", builtin_list, FUNCTION);
  env_add_builtin(e, "head", builtin_head, FUNCTION);
  env_add_builtin(e, "tail", builtin_tail, FUNCTION);
  env_add_builtin(e, "eval", builtin_eval, FUNCTION);
  env_add_builtin(e, "read", builtin_read, FUNCTION);
  env_add_builtin(e, "load", builtin_load, FUNCTION);
  env_add_builtin(e, "cons", builtin_cons, FUNCTION);
  env_add_builtin(e, "=", builtin_equal, FUNCTION);

  env_add_builtin(e, "+", builtin_add, FUNCTION);
  env_add_builtin(e, "-", builtin_sub, FUNCTION);
  env_add_builtin(e, "*", builtin_multiply, FUNCTION);
  env_add_builtin(e, "/", builtin_divide, FUNCTION);
  env_add_builtin(e, ">", builtin_greaterthan, FUNCTION);
  env_add_builtin(e, "<", builtin_lessthan, FUNCTION);
}

/* Given args (formals, body), returns a function or macro */
lval *
builtin_func(lenv *e, lval *args, char *func_name)
{
  ARGNUM(args, 2, "<builtin lambda>");
  // Make sure `formals` contains only symbols
  lval *formals = lval_first(args);
  /* for (int i = 0; i < lval_count(formals); i++) { */
  /*   TYPEASSERT(args, formals->cell[i]->type, LVAL_SYM, "builtin_lambda"); */
  /* } */
  lval *body = lval_first(lval_rest(args));

  if (strcmp(func_name, "macro") == 0) {
    return lval_macro(e, formals, body);
  } else {
    return lval_lambda(e, formals, body);
  }
}

lval *builtin_macro(lenv *e, lval *a) {return builtin_func(e, a, "macro");}
lval *builtin_lambda(lenv *e, lval *a) {return builtin_func(e, a, "lambda");}

/* Operations on numbers */

lval *
builtin_add(lenv *e, lval *args) {
  if (is_empty(args)) { return lval_num(0); }
  long x = get_num(lval_first(args));
  long y = get_num(builtin_add(e, lval_rest(args)));
  return lval_num(x + y);
}


lval *
builtin_sub(lenv *e, lval *args) {
  if (is_empty(args)) { return lval_num(0); }
  long x = get_num(lval_first(args));
  long y = get_num(builtin_sub(e, lval_rest(args)));
  return lval_num(x - y);
}


lval *
builtin_multiply(lenv *e, lval *args) {
  if (is_empty(args)) { return lval_num(1); }
  long x = get_num(lval_first(args));
  long y = get_num(builtin_multiply(e, lval_rest(args)));
  return lval_num(x * y);
}


lval *
builtin_divide(lenv *e, lval *args) {
  if (is_empty(args)) { return lval_num(1); }
  long x = get_num(lval_first(args));
  long y = get_num(builtin_divide(e, lval_rest(args)));
  return lval_num(x / y);
}


lval *
builtin_greaterthan(lenv *e, lval *args)
{
  ARGNUM(args, 2, ">"); /* Make sure we have 2 args */
  lval *l = lval_first(args);
  lval *r = lval_first(lval_rest(args));
  lval *result = lval_bool(get_num(l) > get_num(r));
  lval_del(args);
  return result;
}

lval *builtin_lessthan(lenv *e, lval *args)
{
  print_lval(args);
  ARGNUM(args, 2, "<");
  lval *l = lval_first(args);
  lval *r = lval_first(lval_rest(args));
  lval *result = lval_bool(get_num(l) < get_num(r));
  lval_del(args);
  return result;
}

lval *builtin_equal(lenv *e, lval *args) {
  ARGNUM(args, 2, "=");
  lval *x = lval_first(args);
  lval *y = lval_nth(args, 1);
  return lval_bool(lval_equal(x, y));
}


lval *builtin_progn(lenv *e, lval *args) {
  int count = get_count(args);
  if (count == 0) { return lval_sexp(); }
  lval *first = lval_first(args);
  lval *result = lval_eval(e, first);
  if (count == 1) {
    return result;
  } else {
    return builtin_progn(e, lval_rest(args));
  }
}

/* Operations on lists */
// Takes one or more args, returns a sexp containing them:
lval *builtin_list(lenv *e, lval *args) { return args; }

lval *
builtin_head(lenv *e, lval *args) // Returns the first element of a sexp
{
  ARGNUM(args, 1, "head");
  LASSERT(args, get_type(lval_first(args)) == LVAL_SEXP,
	  "ERROR: Cannot take head of a non-SEXP! (recieved `%s`)",
	  ltype_name(get_type(lval_first(args))));
  LASSERT(args, get_count(lval_first(args)) != 0,
	  "ERROR: Cannot take head of an empty SEXP!");

  lval *v = lval_first(args);
  return lval_first(v);
}

lval *
builtin_tail(lenv *e, lval *args) // Returns the cdr of a sexp
{

  ARGNUM(args, 1, "tail");
  LASSERT(args, get_type(lval_first(args)) == LVAL_SEXP,
	  "ERROR: Cannot take tail of a non-SEXP!");
  LASSERT(args, get_count(lval_first(args)) > 0,
	  "ERROR: Cannot take tail of an empty SEXP!");

  return lval_rest(lval_first(args));
}

lval *
builtin_cons(lenv *e, lval *args)
{
  ARGNUM(args, 2, "cons");
  LASSERT(args, get_type(lval_first(lval_rest(args))) == LVAL_SEXP,
	  "ERROR: Cons function requires a SEXP as a second argument");
  lval *x = lval_first(args);
  lval *l = lval_nth(args, 1);
  lval_cons(l, x);
  return l;
}

lval *
builtin_eval(lenv *e, lval *args)
{
  ARGNUM(args, 1, "eval");
  return lval_eval(e, lval_first(args));
}

/* Evaluate a string containing lisp code */
lval *
builtin_read(lenv *e, lval *args)
{
  ARGNUM(args, 1, "read");
  lval *first = lval_first(args);
  char *str = get_string(first);
  lval *l = read_line(str);
  return l;
}

/* Read in and evaluate a file */
lval *
builtin_load(lenv *e, lval *args)
{
  ARGNUM(args, 1, "read");
  lval *first = lval_first(args);
  char *fname = get_string(first);
  lval *l = read_file(fname);
  //return lval_eval(e, l);
  return l;
}

/* Macro: if cond body else-body */
lval *
builtin_if(lenv *e, lval *args)
{
  ARGNUM(args, 3, "if");

  lval *cond = lval_eval(e, lval_first(args));
  if (get_type(cond) != LVAL_BOOL) {
    return lval_err("ERROR: First argument to `if` must be a BOOL, recieved `%s`.",
		    ltype_name(get_type(cond)));
  }
  lval *body = lval_nth(args, 1);
  lval *elsebody = lval_nth(args, 2);
  lval *result;
  if (get_bool(cond)) {
  result = lval_eval(e, body);
  }
  else {
    result = lval_eval(e, elsebody);
  }
  return result;
}


// Local variable definition
lval *
builtin_def(lenv *e, lval *a)
{
  ARGNUM(a, 2, "def");
  lval *name = lval_first(a);
  LASSERT(name, get_type(name) == LVAL_SYM,
	  "ERROR: Function `%s` not passed a SYMBOL as argument 1!", "def");
  lval *value = lval_eval(e, lval_nth(a, 1));
  if (get_type(value) == LVAL_ERR) { return value; }

  //list *parent = list_first(list_rest(e));
  lenv_set(e, name, value);
  return lval_bool(true);
}
