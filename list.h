#ifndef LIST_H
#define LIST_H

#include "structs.h"

list *list_new(lval *data, list *next);
void list_delete(list *l);
list *list_copy(list *l);
void list_print(list *l);

lval *list_first(list *l);
list *list_rest(list *l);
list *list_cons(lval * e, list *l);
int list_count(list *l);

#endif
