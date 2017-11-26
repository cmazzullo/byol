#include "list.h"
#include "lval.h"
#include "map.h"
#include "environment.h"
#include "builtin.h"
#include "read.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

list *list_remove_pair(list *l, lval *k);

void
test_list_h(void)
{
  lval *x = lval_sym("x");
  lval *y = lval_sym("y");
  lval *z = lval_sym("z");
  lval *q = lval_sym("q");

  lval *pair1 = lval_cons(lval_cons(lval_sexp(), x), y);
  lval *pair2 = lval_cons(lval_cons(lval_sexp(), z), q);
  list *l = list_new(pair1, NULL);
  l = list_cons(pair2, l);
}


void
test_map_h(void)
{
  lval *x = lval_sym("x");
  lval *y = lval_sym("y");
  lval *z = lval_sym("z");
  lval *q = lval_sym("q");

  lval *pair1 = lval_cons(lval_cons(lval_sexp(), x), y);
  lval *pair2 = lval_cons(lval_cons(lval_sexp(), z), q);
  list *l = list_new(pair1, NULL);
  list_print(l);
   l = list_cons(pair2, l);
  list_print(l);
  assert(list_count(l) == 2);
   l = list_remove_pair(l, q);
  list_print(l);
  assert(list_count(l) == 1);

  l = list_remove_pair(l, y);
  assert(list_count(l) == 0);

  map *m = map_new();
  map_add(m, x, y);
  map_add(m, z, q);
  assert(lval_equal(map_get(m, z), q));
  map_add(m, z, x);
  assert(lval_equal(map_get(m, z), x));
  map_delete(m);
}


void
test_lval_h(void)
{
  lval *s = lval_sexp();
  s = lval_cons(s, lval_num(3));
  s = lval_cons(s, lval_num(5));
  s = lval_cons(s, lval_sym("+"));
  // s = `(+ 5 3)`
  assert(strcmp("+", get_sym(lval_first(s))) == 0);
  lenv* e = lenv_new(NULL); // create the environment
  env_add_builtins(e);
  lval *result = lval_eval(e, s);
  assert(get_num(result) == 8);
}

void
test_environment_h(void)
{
  lval *x = lval_sym("x");
  lval *y = lval_sym("y");
  lval *z = lval_sym("z");
  lval *q = lval_sym("q");

  lenv *e1 = lenv_new(NULL);
  lenv *e2 = lenv_new(e1);
  lenv_set(e1, x, y);
  lenv_set(e2, z, q);

  assert(lval_equal(lenv_get(e1, x), y));
  assert(lval_equal(lenv_get(e2, z), q));
  assert(lval_equal(lenv_get(e2, x), y));

  lenv_delete(e2);
  lenv_delete(e1);
}

void
test_integration(void)
{
lval *read(mpc_ast_t *t);
}

int
main() {
  test_list_h();
  test_map_h();
  test_lval_h();
  test_environment_h();
  test_integration();
  printf("Success! All tests passed.\n");
}
