#include "mpc/mpc.h"
#include "map.h"
#include "core.h"
#include "builtin.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h> // for boolean values

#define MAXERR 1024 // Maximum error string length

// LVALS ////////////////////////////////////////////////////////////////////////////////

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
lval_macro(lval *env, lval *formals, lval *body) // create new user-defined macro
{
  lval *v = lval_lambda(env, formals, body);
  v->type = LVAL_MACRO;
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
lval_builtin_macro(lbuiltin fn) // create new empty macro
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
    delete_list(v->cell);
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
      x = lval_builtin_macro(v->builtin);
    } else {
      x = lval_macro(lval_copy(v->env), lval_copy(v->formals), lval_copy(v->body));
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
    x->cell = copy_list(v->cell);
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

  // BUILTIN FUNCTION
  if (f->builtin) {
    return f->builtin(e, a);
  }

  // USER-DEFINED FUNCTION
  if (f->formals->count < a->count) {     // Check for extra arguments
    lval_del(a);
    return lval_err("Function passed too many arguments. ");
  }

  while (a->count > 0) {
    lval *formal = lval_pop(f->formals);

    if (strcmp(formal->sym, "&") == 0) { // if we encounter the `&` sign (variable parameters)
      if (f->formals->count != 1) {
	lval_del(a);
	lval_del(f->env);
	return lval_err("Function format invalid: "
			"Symbol `&` not followed by a single symbol.");
      }
      /* Bind the next formal to the remaining arguments */
      lval *nsym = lval_pop(f->formals);
      lval_put(f->env, nsym, builtin_list(e, a));
      lval_del(formal);
      lval_del(nsym);
      break;
    }

    lval *val = lval_pop(a);
    lval_put(f->env, lval_copy(formal), lval_copy(val)); // Here's where we bind formals to values
    lval_del(val);
    lval_del(formal);
  }
  lval_del(a);

  /* if '&' remains in formal list bind to empty list */
  if (f->formals->count > 0 &&
      strcmp(first(f->formals->cell)->sym, "&") == 0) {
    /* Check to ensure that & is not passed invalidly. */
    if (f->formals->count != 2) {
      lval_del(f->env);
      return lval_err("Function format invalid: "
		      "Symbol '&' not followed by single symbol.");
    }
    /* Pop and delete '&' symbol */
    lval_del(lval_pop(f->formals));
    lval *sym = lval_pop(f->formals);
    lval *val = lval_sexp();
    lval_put(e, sym, val);
    lval_del(sym); lval_del(val);
  }

  if (f->formals->count == 0) {
    return lval_eval(f->env, f->body);
  } else {
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
      //print_lval(v->env);
      printf("(\\ "); print_lval(v->formals);
      putchar(' '); print_lval(v->body); putchar(')');
    }
    break;
  case LVAL_ERR: printf(v->err); break;
  case LVAL_SYM: printf(v->sym); break;
  case LVAL_SEXP:
    putchar('(');
    list *l = v->cell;
    for (int i = 0; i < v->count; i++) {
      if (i != 0) putchar(' ');
      print_lval(first(l));
      l = rest(l);
    }
    putchar(')');
    break;
  case LVAL_BOOL:
    if (v->boolean)
      printf("true");
    else printf("false");
    break;
  }
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
    result = lval_get(e, v);
    break;
  case LVAL_SEXP:
    result = lval_eval_sexp(e, v);
    break;
  }
  return result;
}

lval * /* Check to see if any sexp entry is an ERROR */
error_check(list *l)
{
  if (!l) { return NULL; }
  if (get_type(first(l)) == LVAL_ERR) {
    return first(l);
  } else {
    return error_check(rest(l));
  }
}

lval *
lval_eval_sexp(lval *e, lval *v)
{
  if (v->count == 0) {
    return lval_copy(v); // return `()` as-is
  }

  lval *first = lval_eval(e, lval_pop(v)); // pops off the first element
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
  lval *result = lval_sexp();
  for (int i = 0; i < v->count; i++) {
    lval_cons(result, lval_eval(e, lval_pop(v)));
  }

  // catch errors in the rest of the args
  lval *err = error_check(result->cell);
  if (err) { return err; }

  return lval_call(e, first, v);
}

lval *
lval_pop(lval *v) // pop off the first child of the sexp (car)
{
  lval *x = first(v->cell);
  v->cell = rest(v->cell);
  v->count--;
  return x;
}

lval *
lval_take(lval *v) // like pop but delete the resulting list
{
  lval *x = lval_pop(v);
  lval_del(v);
  return x;
}

lval * // append x to list v
lval_cons(lval *v, lval *x)
{
  (v->count)++;
  v->cell = cons(x, v->cell);
  return v;
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
void
lval_put(lval *dict, lval *name, lval *v)
{
  map_add(dict->dict, name, v);
}

/* Return the number of elements in a sexp, -1 if not a sexp */
int
lval_count(lval *l)
{
  if (l->type != LVAL_SEXP) {
    return -1;
  } else {
    return l->count;
  }
}

int get_num(lval *l) {return l->num;}
bool get_bool(lval *l) {return l->boolean;}
char *get_err(lval *l) {return l->err;}
char *get_sym(lval *l) {return l->sym;}
int get_type(lval *l) {return l->type;}
int get_count(lval *l) {return l->count;}
list *get_cell(lval *l) {return l->cell;}
lbuiltin get_builtin(lval *x) {return x->builtin;}

lval *get_formals(lval *fn) { return fn->formals; }
lval *get_env(lval *fn) { return fn->env; }
lval *get_body(lval *fn) { return fn->body; }


lval *lval_first(lval *l) {return first(l->cell);}

lval *lval_rest(lval *l)
{
  if (l->count == 0)
    return lval_err("ERROR: `lval_rest` called on empty list");
  lval *v = lval_sexp();
  v->cell = rest(l->cell);
  v->count = l->count - 1;
  return v;
}
