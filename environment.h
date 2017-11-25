#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "structs.h"

lenv *lenv_new(lenv *parent);
lenv *lenv_copy(lenv *e);
void lenv_print(lenv *e);
void lenv_delete(lenv *e);

lval *lenv_get(lenv *e, lval *k);
void lenv_set(lenv *e, lval *k, lval *v);

#endif
