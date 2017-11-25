/*
  Simple hash table of lvals
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "map.h"
#include "list.h"
#include "environment.h"
#include "lval.h"
#define ARRAYSIZE 1024
#define MAXKEY 256 // Max key length

struct map { // hash map with chaining
  list **data; // Array of list pointers
  list *keys;
};

int
hash(lval *key)
{
  int h = 1;
  char *keystr = get_sym(key);
  int keylen = strlen(keystr);

  char c;
  for (int i = 0; i < keylen; i++) {
    c = keystr[i];
    h = h + ((i + 12) * c);
    i++;
  }
  h = abs(943287 * h) % ARRAYSIZE;
  assert(h >= 0);
  return h;
}

// List helper functions

/* Remove an lval by key from a list of key/value pairs */
list *
list_remove_pair(list *l, lval *k)
{
  if (!l) { return l; }
  lval *list_key = lval_first(list_first(l));
  // if the sym `k` and the sym in the current node are equal:
  if (lval_equal(list_key, k)) {
    return list_rest(l);
  } else {
    return list_cons(list_first(l), list_remove_pair(list_rest(l), k));
  }
}

list *
list_remove(list *l, lval *v)
{
  if (!l) { return l; }
  char *list_key = get_sym(list_first(l));
  if (strcmp(list_key, get_sym(v)) == 0) {
    return list_rest(l);
  } else {
    return list_cons(list_first(l), list_remove(list_rest(l), v));
  }
}


lval *
list_get(list *l, lval *k)
{
  if (!l) { return NULL; }
  lval *head = list_first(l);
  if (lval_equal(lval_first(head), k)) {
    return lval_first(lval_rest(list_first(l)));
  } else {
    return list_get(list_rest(l), k);
  }
}

map *
map_new(void)
{
  map *m = calloc(1, sizeof(map));
  m->data = calloc(ARRAYSIZE, sizeof(list *));
  m->keys = NULL;
  return m;
}

map *
map_copy(map *m)
{
  map *x = map_new();
  list *keys = m->keys;
  while (keys) {
    map_add(x, list_first(keys), map_get(m, list_first(keys)));
    keys = list_rest(keys);
  }
  return x;
}

void
map_delete(map *m)
{
  list *l = m->keys;
  while (l) {
    map_remove(m, list_first(l));
    l = list_rest(l);
  }
  free(m->data);
  list_delete(m->keys);
  free(m);
}

/* Remove a key:value pair from the map */
void
map_remove(map *m, lval *key)
{
  m->keys = list_remove(m->keys, key);
  m->data[hash(key)] = list_remove_pair(m->data[hash(key)], key);
}

/*
   Add a key:value pair to the map. NOTE: If `key` is already in the
   map, its value is overwritten by the new value!
*/
void
map_add(map *m, lval *key, lval *val)
{
  if (map_contains(m, key)) { map_remove(m, key); }
  int h = hash(key);
  // The pair is stored as a 2-element linked list:
  lval *pair = lval_sexp();
  lval_cons(pair, val);
  lval_cons(pair, key);
  m->data[h] = list_cons(pair, m->data[h]);
  m->keys = list_cons(key, m->keys);
}

/* Wrapper around map_get that returns a bool */
bool map_contains(map *m, lval *key) { return map_get(m, key) != NULL; }

lval *
map_get(map *m, lval *key)
{
  list *l = m->data[hash(key)];
  if (!l) { return NULL; }
  return list_get(l, key); // returns NULL if key isn't found/list is NULL
}


void
map_print(map *m)
{
  list *keys = m->keys;
  printf("{");
  while (keys) {
    print_lval(list_first(keys));
    printf(" : ");
    print_lval(map_get(m, list_first(keys)));
    keys = list_rest(keys);
    if (keys) { printf(", "); }
  }
  printf("}\n");
}
