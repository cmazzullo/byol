#ifndef CORE_H
#define CORE_H

typedef struct lval lval;
typedef lval* (*lbuiltin)(lval *, lval *);

#include <stdbool.h>
#include "list.h"

int lval_count(lval *l);
lval *lval_sexp(void);
lval *lval_num(long x);
lval *lval_bool(bool x);
lval *lval_dict(void);
lval *lval_sym(char *sym);
lval *lval_err(char *fmt, ...);
char *ltype_name(int t);
lval *lval_lambda(lval *env, lval *formals, lval *body);
lval *lval_macro(lval *env, lval *formals, lval *body);
lval *lval_fn(lbuiltin fn);
lval *lval_builtin_macro(lbuiltin fn);
lval *lval_copy(lval *v);
void print_lval(lval *v);
lval *lval_eval_sexp(lval *e, lval *v);
lval *lval_pop(lval *v);
lval *lval_take(lval *v);
lval *lval_eval(lval *e, lval *v);
void lval_del(lval *v);
lval *lval_get(lval *env, lval *name);
void lval_put(lval *env, lval *name, lval *v);
void env_add_builtin(lval *e, char *name, lbuiltin fn);
void env_add_builtins(lval *e);
int get_num(lval *l);
bool get_bool(lval *l);
int get_count(lval *l);
char *get_err(lval *l);
char *get_sym(lval *l);
int get_type(lval *l);
list *get_cell(lval *l);
lbuiltin get_builtin(lval *x);
lval *get_formals(lval *fn);
lval *get_env(lval *fn);
lval *get_body(lval *fn);
lval *lval_first(lval *l);
lval *lval_rest(lval *l);
lval *lval_cons(lval *v, lval *x);

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXP,
       LVAL_MACRO, LVAL_FN, LVAL_BOOL, LVAL_DICT };

// MACROS ////////////////////////////////////////////////////////////////////////////////

#define LASSERT(args, cond, fmt, ...)		\
  if (!(cond)) {				\
    lval *err = lval_err(fmt, ##__VA_ARGS__);	\
    lval_del(args);				\
    return err;					\
  }

#define ARGNUM(args, correctnum, funcname)				\
  if (lval_count(args) != correctnum) {					\
    lval *err = lval_err("ERROR: Function `%s` requires %d argument(s) (passed %d)!", \
			 funcname, correctnum, lval_count(args));	\
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
