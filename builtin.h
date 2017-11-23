#ifndef BUILTIN_H
#define BUILTIN_H

#include "lval.h"

lval *builtin_eval(lval *e, lval *args);
lval *builtin_equal(lval *e, lval *args);
lval *builtin_list(lval *e, lval *args);

lval *builtin_lambda(lval *e, lval *args);
lval *builtin_macro(lval *e, lval *args);

lval *builtin_head(lval *e, lval *args);
lval *builtin_tail(lval *e, lval *args);
lval *builtin_cons(lval *e, lval *args);

lval *builtin_def(lval *e, lval *args);

lval *builtin_if(lval *e, lval *args);

lval *builtin_add(lval *e, lval *args);
lval *builtin_sub(lval *e, lval *args);
lval *builtin_multiply(lval *e, lval *args);
lval *builtin_divide(lval *e, lval *args);
lval *builtin_greaterthan(lval *e, lval *args);
lval *builtin_lessthan(lval *e, lval *args);

#endif
