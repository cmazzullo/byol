/*
  Simple hash table of lvals
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "map.h"

#define ARRAYSIZE 1024
#define MAXKEY 256 // Max key length

struct map { // hash map with chaining
  list **data; // Array of list pointers
  list *keys;
};

int
hash(lval *key)
{
  char *keystr = get_sym(key);
  int h = 1;

  char c;
  int i = 0;
  while((c = keystr[i]) != '\0') {
    h = h + (i * c);
    i++;
  }
  return (943287 * h) % ARRAYSIZE;
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
    map_add(x, first(keys), map_get(m, first(keys)));
    keys = rest(keys);
  }
  return x;
}

void
map_del(map *m)
{
  list *l = m->keys;
  while (l) {
    map_remove(m, first(l));
    l = rest(l);
  }
  free(m->data);
  delete_list(m->keys);
  free(m);
}

/* Remove a key:value pair from the map */
void
map_remove(map *m, lval *key)
{
  m->keys = list_remove(m->keys, key);
  m->data[hash(key)] = list_remove(m->data[hash(key)], key);
}

/*
   Add a key:value pair to the map. NOTE: If `key` is already in the
   map, its value is overwritten by the new value!
*/
void
map_add(map *m, lval *key, lval *val)
{
  if (map_contains(m, key)) {map_remove(m, key);}
  int h = hash(key);
  // The pair is stored as a 2-element linked list:
  lval *pair = lval_sexp();
  lval_cons(pair, key);
  lval_cons(pair, val);
  m->data[h] = cons(pair, m->data[h]);
  m->keys = cons(key, m->keys);
}

/* Wrapper around map_get that returns a bool */
bool map_contains(map *m, lval *key) { return map_get(m, key) == NULL; }

lval *
map_get(map *m, lval *key)
{
  list *l = m->data[hash(key)];
  return list_get(l, key); // returns NULL if key isn't found/list is NULL
}


void
map_print(map *m)
{
  list *keys = m->keys;
  printf("{\n");
  while (keys) {
    print_lval(first(keys));
    putchar('\n');
    keys = rest(keys);
  }
  printf("}\n");
}
