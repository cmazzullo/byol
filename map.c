/*
  Simple hash table of lvals
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "map.h"

#define ARRAYSIZE 1024

struct list {
  lval *val;
  char *key;
  list *next;
};

struct map { // hash map with chaining
  list **data; // Array of list pointers
  char **keys;
  int nkeys; // number of keys
};


list *
new_list(char *key, lval *val)
{
  list *n = calloc(1, sizeof(list));
  n->key = key;
  n->val = val;
  return n;
}

list *cdr(list *l) {return l->next;}

list *
cons(char *key, lval *val, list *l)
{
  list *head = new_list(key, val);
  head->next = l;
  return head;
}

lval */* Search through a list for the val associated w/ `key` */
list_get(list *l, char *key)
{
  if (l) { // if l is not NULL
    if (strcmp(l->key, key) == 0) {
      return l->val;
    } else {
      return list_get(cdr(l), key);
    }
  } else {
    return -999; // return filler value
  }
}

map *
new_map(void)
{
  map *m = calloc(1, sizeof(map));
  m->data = calloc(ARRAYSIZE, sizeof(list *));
  m->keys = calloc(ARRAYSIZE, sizeof(char *));
  m->nkeys = 0;
  return m;
}

void
map_add(map *m, char *key, lval *val)
{
  int h = hash(key);
  if (m->data[h]) { // if the list at `h` isn't null
    m->data[h] = cons(key, val, m->data[h]);
  } else {
    m->data[h] = new_list(key, val);
  }
  m->keys[m->nkeys] = key;
  m->nkeys++;
}

int
map_get(map *m, char *key)
{
  list *l = m->data[hash(key)];
  return list_get(l, key);
}

int
hash(char *key)
{
  int h = 1;

  char c;
  int i = 0;
  while((c = key[i]) != '\0') {
    h = h + (i * c);
    i++;
  }
  return (943287 * h) % ARRAYSIZE;
}

void
map_print(map *m)
{
  printf("{\n");
  for(int i = 0; i < m->nkeys; i++) {
    printf("  %s: %d", m->keys[i], map_get(m, m->keys[i]));
    putchar('\n');
  }
  printf("}\n");
}

int
main(int argc, char **argv)
{
  map *m = new_map();
  map_add(m, "thing", 1);
  map_add(m, "other_thing", 2);
  map_print(m);
  printf("%d, %d, %d\n", map_get(m, "thing"), map_get(m, "other_thing"), map_get(m, "fake"));
  printf("Hash \"thing\" = %d\n", hash("thing"));
  printf("Hash \"hting\" = %d\n", hash("hting"));
  printf("Hash \"other_thing\" = %d\n", hash("other_thing"));
  printf("Get \"other thing\" = %d\n", map_get(m, "other thing"));
  return 0;
}
