#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H
#include "list.h"
#include "lval.h"

typedef list lenv;

lenv *lenv_new();
lenv *lenv_copy(lenv *e);
void lenv_print(lenv *e);

lval *lenv_get(lenv *e, lval *k);
void lenv_set(lenv *e, lval *k, lval *v);

#endif
