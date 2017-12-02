#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "environment.h"
#include "map.h"
#include "list.h"
#include "lval.h"


lenv *lenv_new(lenv *parent) { return list_new(lval_dict(), parent); }

lenv *lenv_copy(lenv *e) { return list_copy(e); }

void lenv_delete(lenv *e) { list_delete(e); }

void
lenv_print(lenv *e)
{
  printf("<Environment>:\n");
  list_print(e);
}

/* Iterates through every map in the list to find `k` */
lval *
lenv_get(lenv *e, lval *k)
{
  if (!e) { return lval_err("ERROR: Variable `%s` not found!", get_sym(k)); }
  lval *v = lval_get(list_first(e), k);
  if (get_type(v) != LVAL_ERR) {
    return v;
  } else {
    return lenv_get(list_rest(e), k);
  }
}

/* Set K equal to V in E */
void lenv_set(lenv *e, lval *k, lval *v) { lval_put(list_first(e), k, v); }
