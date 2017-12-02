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

void
test_list(void)
{
  lval *x = lval_sym("x");
  lval *y = lval_sym("y");
  lval *z = lval_sym("z");

  list *l = list_new(x, NULL);
  l = list_cons(y, l);
  l = list_cons(z, l);
  assert(list_first(l) == z);
  assert(list_first(list_rest(l)) == y);
  assert(list_count(l) == 3);
}


void
test_map(void)
{
  lval *x = lval_sym("x");
  lval *y = lval_sym("y");
  lval *z = lval_sym("z");
  lval *q = lval_sym("q");

  map *m = map_new();
  map_add(m, x, y);
  map_add(m, z, q);
  assert(lval_equal(map_get(m, z), q));
  map_add(m, z, x);
  assert(lval_equal(map_get(m, z), x));

  map *m_copy = map_copy(m);
  map_add(m_copy, z, y);
  assert(lval_equal(map_get(m_copy, z), y));
  assert(lval_equal(map_get(m, z), x));
  assert(map_contains(m_copy, y) == 0);
  assert(map_contains(m_copy, z) == 1);
  map_remove(m_copy, z);
  assert(map_contains(m_copy, z) == 0);
  map_delete(m);
}

void
test_read(void)
{
  read_initialize();
  char *line = "(+ 1 a)";
  // construct value
  lval *exp = lval_sexp();
  lval_cons(exp, lval_sym("a"));
  lval_cons(exp, lval_num(1));
  lval_cons(exp, lval_sym("+"));
  lval *result = read_line(line);
  assert(lval_equal(exp, result));
  read_cleanup();
}

void
test_lval(void)
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
test_environment(void)
{
  lval *x = lval_sym("x");
  lval *y = lval_sym("y");
  lval *x_val = lval_sym("x_val");
  lval *y_val = lval_sym("y_val");

  lenv *e1 = lenv_new(NULL); // parent env
  lenv *e2 = lenv_new(e1); // child env
  lenv_set(e1, x, x_val);
  lenv_set(e2, y, y_val);
  assert(lval_equal(lenv_get(e1, x), x_val));
  assert(lval_equal(lenv_get(e2, y), y_val));

  lenv_set(e2, x, y_val); // overwrite the parent's definition of x
  assert(lval_equal(lenv_get(e2, x), y_val));
  assert(lval_equal(lenv_get(e1, x), x_val));
  assert(get_type(lenv_get(e1, x_val)) == LVAL_ERR);

  lenv_delete(e1);
}

int
main() {
  test_list();
  test_map();
  test_lval();
  test_read();
  test_environment();
  printf("Success! All tests passed.\n");
}
