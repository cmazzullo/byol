#include <stdio.h>
#include <string.h>
#include "environment.h"
/* Search through l for e and remove it */
list *
list_remove(list *l, lval *key)
{
  if (!l) { return NULL; }
  if (strcmp(get_sym(key), get_sym(list_first(l))) == 0) {
    return list_rest(l);
  } else {
    return list_cons(list_first(l), list_remove(list_rest(l), key));
  }
}

lval *list_get(list *l, lval *k)
{
  if (!l) { return NULL; }
  else if (strcmp(get_sym(lval_first(list_first(l))), get_sym(k)) == 0) {
    return list_first(l);
  } else {
    return list_get(list_rest(l), k);
  }
}
