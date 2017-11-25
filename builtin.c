#include "lval.h"
#include "list.h"
#include "builtin.h"
#include "structs.h"
#include "environment.h"

#include <string.h>
#include <stdlib.h>

/* Adds a builtin to the environment */
void
env_add_builtin(lenv *e, char *name, lbuiltin fn)
{
  lval *v = lval_builtin_function(fn);
  lenv_set(e, lval_sym(name), lval_copy(v));
  lval_del(v);
}

/* Adds a macro to the environment */
void
env_add_builtin_macro(lenv *e, char *name, lbuiltin fn)
{
  lval *v = lval_builtin_macro(fn);
  lenv_set(e, lval_sym(name), lval_copy(v));
  lval_del(v);
}

void
env_add_builtins(lenv *e)
{
  env_add_builtin_macro(e, "\\", builtin_lambda);
  env_add_builtin_macro(e, "macro", builtin_macro);
  env_add_builtin_macro(e, "if", builtin_if);
  env_add_builtin_macro(e, "def", builtin_def);

  env_add_builtin(e, "list", builtin_list);
  env_add_builtin(e, "head", builtin_head);
  env_add_builtin(e, "tail", builtin_tail);
  env_add_builtin(e, "eval", builtin_eval);
  env_add_builtin(e, "cons", builtin_cons);
  env_add_builtin(e, "=", builtin_equal);

  env_add_builtin(e, "+", builtin_add);
  env_add_builtin(e, "-", builtin_sub);
  env_add_builtin(e, "*", builtin_multiply);
  env_add_builtin(e, "/", builtin_divide);
  env_add_builtin(e, ">", builtin_greaterthan);
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
builtin_op(lenv *e, char *op, lval *args)
{
  long result;
  if ((strcmp(op, "+") == 0) || (strcmp(op, "-") == 0)) {
    result = 0;
  } else {
    result = 1;
  }
  while (!is_empty(args)) {
    long x = get_num(lval_first(args));
    if (strcmp(op, "+") == 0) result += x;
    if (strcmp(op, "-") == 0) result -= x;
    if (strcmp(op, "*") == 0) result *= x;
    if (strcmp(op, "/") == 0) {
      LASSERT(args, x != 0, "ERROR: Division by zero!");
      result /= x;
    }
    args = lval_rest(args);
  }
  return lval_num(result);
}

lval *builtin_add(lenv *e, lval *args) {return builtin_op(e, "+", args);}
lval *builtin_sub(lenv *e, lval *args) {return builtin_op(e, "-", args);}
lval *builtin_multiply(lenv *e, lval *args) {return builtin_op(e, "*", args);}
lval *builtin_divide(lenv *e, lval *args) {return builtin_op(e, "/", args);}

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

lval *builtin_lessthan(lenv *e, lval *args) {return builtin_op(e, "<", args);}

lval *builtin_equal(lenv *e, lval *args) {
  ARGNUM(args, 2, "=");
  lval *x = lval_first(args);
  lval *y = lval_nth(args, 1);
  return lval_bool(lval_equal(x, y));
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
  return lval_cons(lval_rest(args), lval_first(args));
}

lval *
builtin_eval(lenv *e, lval *args)
{
  ARGNUM(args, 1, "eval");
  return lval_eval(e, lval_first(args));
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
  lval_del(args);
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
  lenv_set(e, name, value);
  return value;
}
