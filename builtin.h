#ifndef BUILTIN_H
#define BUILTIN_H

#include "core.h"

lval *builtin_func(lval *e, lval *args, char *func_name);
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

#endif
