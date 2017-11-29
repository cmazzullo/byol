#ifndef BUILTIN_H
#define BUILTIN_H

#include "structs.h"

lval *builtin_eval(lenv *e, lval *args);
lval *builtin_read(lenv *e, lval *args);
lval *builtin_load(lenv *e, lval *args);

lval *builtin_def(lenv *e, lval *args);
void env_add_builtins(lenv *e);

lval *builtin_equal(lenv *e, lval *args);
lval *builtin_progn(lenv *e, lval *args);

lval *builtin_list(lenv *e, lval *args);
lval *builtin_head(lenv *e, lval *args);
lval *builtin_tail(lenv *e, lval *args);
lval *builtin_cons(lenv *e, lval *args);

lval *builtin_lambda(lenv *e, lval *args);
lval *builtin_macro(lenv *e, lval *args);

lval *builtin_if(lenv *e, lval *args);

lval *builtin_add(lenv *e, lval *args);
lval *builtin_sub(lenv *e, lval *args);
lval *builtin_multiply(lenv *e, lval *args);
lval *builtin_divide(lenv *e, lval *args);
lval *builtin_greaterthan(lenv *e, lval *args);
lval *builtin_lessthan(lenv *e, lval *args);

#endif
