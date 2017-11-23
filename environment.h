#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H
#include "list.h"
#include "lval.h"

list *list_remove(list *l, lval *key);
lval *list_get(list *l, lval *k);

#endif
