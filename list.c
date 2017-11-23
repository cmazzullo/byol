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
list_new(lval *data, list *next)
{
  list *x = malloc(sizeof(list));
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
  free(l);
}

lval *list_first(list *l) {return l->data;}
list *list_rest(list *l) {return l->next;}
list *list_cons(lval *e, list *l) {return list_new(e, l);}

void
list_print(list *l)
{
  if (l) {
    print_lval(list_first(l));
    list_print(list_rest(l));
  } else {
    putchar('\n');
  }
}


/* void main() { */

/*   lval *x = lval_sym("x"); */
/*   lval *y = lval_sym("y"); */
/*   lval *z = lval_sym("z"); */

/*   list *l = list_new(x, list_new(y, list_new(z, NULL))); */
/*   list_print(l); */

/*   list_print(list_cons(x, l)); */

/*   printf("count = %d\n", list_count(l)); */

/*   lval *q = lval_copy(x); */
/*   list *copy = list_copy(l); */
/*   copy = list_cons(x, copy); */
/*   list_print(copy); */
/*   list_print(l); */

/*   list_delete(l); */
/* } */
