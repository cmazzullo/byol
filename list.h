#ifndef LIST_H
#define LIST_H

typedef struct list list;

#include "core.h"

list *new_list(lval * data, list *next);
void delete_list(list *l);
lval *first(list *l);
list *rest(list *l);
list *cons(lval * e, list *l);
list *list_remove(list *l, lval *key);
void print_list(list *l);
int count(list *l);
list *copy_list(list *l);
lval *list_get(list *l, lval *k);

#endif
