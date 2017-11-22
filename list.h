#ifndef LIST_H
#define LIST_H

typedef struct list list;

int first(list *l);
list *rest(list *l);
list *cons(int e, list *l);
list *new_list(int data, list *next)
void delete_list(list *l)
int first(list *l)
list *rest(list *l)
list *cons(int e, list *l)
void print_list(list *l)
list *list_remove(int e, list *l)

#endif
