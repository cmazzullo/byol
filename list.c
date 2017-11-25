/*
   Linked list implementation for byol
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "lval.h"
typedef struct list list;

struct list {
  lval *data;
  list *next;
};

list *
list_new(lval *data, list *next)
{
  list *x = calloc(1, sizeof(list));
  x->data = data;
  x->next = next;
  return x;
}

list *
list_copy(list *l)
{
  if (!l) return NULL;
  return list_new(lval_copy(list_first(l)), list_copy(list_rest(l)));
}

int
list_count(list *l)
{
  if (!l) return 0;
  return 1 + list_count(list_rest(l));
}

void
list_delete(list *l)
{
  if (!l) { return; }
  list_delete(list_rest(l));
  lval_del(list_first(l));
  free(l);
}

lval *list_first(list *l) {return l->data;}
list *list_rest(list *l) {return l->next;}
list *list_cons(lval *e, list *l) {return list_new(e, l);}

void
print_helper(list *l)
{
  if (l) {
    print_lval(list_first(l));
    if (list_rest(l)) { putchar(' '); }
    print_helper(list_rest(l));
  }
}

void
list_print(list *l)
{
  printf("[");
  print_helper(l);
  printf("]");
}
