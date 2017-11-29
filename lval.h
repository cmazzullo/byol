#ifndef CORE_H
#define CORE_H
#include <stdbool.h>

#include "structs.h"

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXP,
       LVAL_MACRO, LVAL_FN, LVAL_BOOL, LVAL_DICT,
       LVAL_STRING };

void lval_del(lval *v);
lval *lval_copy(lval *v);
void print_lval(lval *v);

// Functions

char *ltype_name(int t);
lval *lval_eval(lenv *e, lval *v);
bool lval_equal(lval *x, lval *y);

lval *lval_first(lval *l);
lval *lval_rest(lval *l);
lval *lval_nth(lval *l, int n);
lval *lval_cons(lval *v, lval *x);


// Constructor

lval *lval_sexp(void);
lval *lval_num(long x);
lval *lval_bool(bool x);
lval *lval_dict(void);
lval *lval_sym(char *sym);
lval *lval_err(char *fmt, ...);
lval *lval_string(char *str);

lval *lval_lambda(lenv *e, lval *formals, lval *body);
lval *lval_macro(lenv *e, lval *formals, lval *body);
lval *lval_builtin_function(lenv *e, lbuiltin fn);
lval *lval_builtin_macro(lenv *e, lbuiltin fn);

// Dict

lval *lval_get(lval *d, lval *name);
lval *lval_put(lval *d, lval *name, lval *v);

// Accessor

int get_num(lval *l);
bool get_bool(lval *l);
int get_count(lval *l);
bool is_empty(lval *l);
char *get_sym(lval *l);
int get_type(lval *l);
lenv *get_env(lval *fn);
int get_count(lval *l);
char *get_string(lval *l);

// MACROS ////////////////////////////////////////////////////////////////////////////////

#define LASSERT(args, cond, fmt, ...)		\
  if (!(cond)) {				\
    lval *err = lval_err(fmt, ##__VA_ARGS__);	\
    lval_del(args);				\
    return err;					\
  }

#define ARGNUM(args, correctnum, funcname)				\
  if (get_count(args) != correctnum) {					\
    lval *err = lval_err("ERROR: Function `%s` requires %d argument(s) (passed %d)!", \
			 funcname, correctnum, get_count(args));	\
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

#endif
