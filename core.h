#ifndef CORE_H
#define CORE_H

#include "mpc/mpc.h"

// FORWARD DECLARATIONS ////////////////////////////////////////////////////////////////////////////////
struct lval;
typedef struct lval lval;
typedef lval* (*lbuiltin)(lval *, lval *);

lval *read(mpc_ast_t *t) ;
lval *read_num(mpc_ast_t *t);
lval *read_bool(mpc_ast_t *t);
lval *builtin_greaterthan(lval *e, lval *args);
lval *builtin_lessthan(lval *e, lval *args);
lval *builtin_lambda(lval *e, lval *args);
lval *builtin_macro(lval *e, lval *args);
lval *builtin_list(lval *e, lval *args);
lval *builtin_head(lval *e, lval *args);
lval *builtin_tail(lval *e, lval *args);
lval *builtin_def(lval *e, lval *args);
lval *builtin_put(lval *e, lval *a);
lval *builtin_var(lval *e, lval *a, char *func);
lval *builtin_join(lval *e, lval *args);
lval *builtin_if(lval *e, lval *args);
lval *builtin_add(lval *e, lval *args);
lval *builtin_sub(lval *e, lval *args);
lval *builtin_multiply(lval *e, lval *args);
lval *builtin_divide(lval *e, lval *args);
lval *builtin_eval(lval *e, lval *args);
lval *builtin_equal(lval *e, lval *args);
lval *builtin_cons(lval *e, lval *args);
lval *builtin_len(lval *e, lval *args);
lval *builtin_op(lval *e, char *op, lval *args);

lval *lval_num(long x);
lval *lval_dict(void);
lval *lval_sym(char *sym);
lval *lval_err(char *fmt, ...);
char *ltype_name(int t);
lval *lval_lambda(lval *env, lval *formals, lval *body);
lval *lval_fn(lbuiltin fn);
lval *lval_macro(lbuiltin fn);
lval *lval_copy(lval *v);
void print_lval(lval *v);
void print_lval_sexp(lval *v, char open, char close);
lval *lval_eval_sexp(lval *e, lval *v);
lval *lval_pop(lval *v);
lval *lval_add(lval *v, lval *x);
lval *lval_take(lval *v);
lval *lval_eval(lval *e, lval *v);
void lval_del(lval *v);
lval *lval_get(lval *env, char *name);
void lval_put(lval *env, lval *name, lval *v);
void env_add_builtin(lval *e, char *name, lbuiltin fn);
void env_add_builtins(lval *e);

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

#endif
