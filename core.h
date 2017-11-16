// FORWARD DECLARATIONS ////////////////////////////////////////////////////////////////////////////////
struct lval;
typedef struct lval lval;
struct lenv;
typedef struct lenv lenv;
typedef lval* (*lbuiltin)(lenv *, lval *);

lval *read(mpc_ast_t *t) ;
lval *read_num(mpc_ast_t *t);
lval *read_bool(mpc_ast_t *t);
lval *builtin_greaterthan(lenv *e, lval *args);
lval *builtin_lessthan(lenv *e, lval *args);
lval *builtin_lambda(lenv *e, lval *args);
lval *builtin_macro(lenv *e, lval *args);
lval *builtin_list(lenv *e, lval *args);
lval *builtin_head(lenv *e, lval *args);
lval *builtin_tail(lenv *e, lval *args);
lval *builtin_def(lenv *e, lval *args);
lval *builtin_put(lenv *e, lval *a);
lval *builtin_var(lenv *e, lval *a, char *func);
lval *builtin_join(lenv *e, lval *args);
lval *builtin_if(lenv *e, lval *args);
lval *builtin_add(lenv *e, lval *args);
lval *builtin_sub(lenv *e, lval *args);
lval *builtin_multiply(lenv *e, lval *args);
lval *builtin_divide(lenv *e, lval *args);
lval *builtin_eval(lenv *e, lval *args);
lval *builtin_equal(lenv *e, lval *args);
lval *builtin_cons(lenv *e, lval *args);
lval *builtin_len(lenv *e, lval *args);
lval *builtin_op(lenv *e, char *op, lval *args);

lval *lval_sym(char *sym);
lval *lval_err(char *fmt, ...);
char *ltype_name(int t);
lval *lval_lambda(lval *formals, lval *body);
lval *lval_fn(lbuiltin fn);
lval *lval_copy(lval *v);
void print_lval(lval *v);
void print_lval_sexp(lval *v, char open, char close);
lval *lval_eval_sexp(lenv *e, lval *v);
lval *lval_pop(lval *v, int i);
lval *lval_add(lval *v, lval *x);
lval *lval_take(lval *v, int i);
lval *lval_eval(lenv *e, lval *v);
void lval_del(lval *v);

lenv *lenv_new(void);
void lenv_del(lenv *env);
lenv *lenv_copy(lenv *env);
lval *lenv_get(lenv *env, char *name);
void lenv_put(lenv *env, lval *name, lval *v);
void lenv_add_builtin(lenv *e, char *name, lbuiltin fn);
void lenv_add_builtins(lenv *e);

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXP,
       LVAL_MACRO, LVAL_FN, LVAL_BOOL };

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
