/*
   Linked list implementation for byol
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
typedef struct list list;

struct list {
  lval *data;
  list *next;
};

list *
new_list(lval *data, list *next)
{
  list *x = malloc(sizeof(list));
  x->data = data;
  x->next = next;
  return x;
}

list *
copy_list(list *l)
{
  if (!l) NULL;
  return new_list(lval_copy(first(l)), copy_list(rest(l)));
}

int
count(list *l)
{
  if (!l) 0;
  return 1 + count(rest(l));
}

void
delete_list(list *l)
{
  if (!l) { return; }
  delete_list(rest(l));
  free(l);
}

lval *first(list *l) {return l->data;}
list *rest(list *l) {return l->next;}
list *cons(lval *e, list *l) {return new_list(e, l);}

void
print_list(list *l)
{
  if (l) {
    printf("%d ", first(l));
    print_list(rest(l));
  } else {
    putchar('\n');
  }
}

/* Search through l for e and remove it */
list *
list_remove(list *l, lval *key)
{
  if (!l) { return NULL; }
  if (strcmp(get_sym(key), get_sym(first(l))) == 0) {
    return rest(l);
  } else {
    return cons(first(l), list_remove(rest(l), key));
  }
}

lval *list_get(list *l, lval *k)
{
  if (!l) { return NULL; }
  else if (strcmp(get_sym(lval_first(first(l))), get_sym(k)) == 0) {
    return first(l);
  } else {
    return list_get(rest(l), k);
  }
}
