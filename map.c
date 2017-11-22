/*
  Simple hash table of lvals
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "map.h"
#include "core.h"

#define ARRAYSIZE 1024
#define MAXKEY 256 // Max key length

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
  n->key = malloc(sizeof(char) * MAXKEY);
  strncpy(n->key, key, MAXKEY);
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
    return NULL; // return filler value
  }
}

map *
map_new(void)
{
  map *m = calloc(1, sizeof(map));
  m->data = calloc(ARRAYSIZE, sizeof(list *));
  m->keys = calloc(ARRAYSIZE, sizeof(char *));
  m->nkeys = 0;
  return m;
}

map *
map_copy(map *m)
{
  map *x = map_new();
  for (int i = 0; i < m->nkeys; i++) { // for each key in m
    map_add(x, m->keys[i], map_get(m, m->keys[i])); // add the val to x
  }
  return x;
}

void
map_del(map *m)
{
  free(m->data);
  free(m->keys);
  free(m);
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

  m->keys = realloc(m->keys, sizeof(char *) * (m->nkeys + 1));
  m->keys[m->nkeys] = malloc(sizeof(char) * MAXKEY);
  strncpy(m->keys[m->nkeys], key, MAXKEY);
  m->nkeys++;
}

lval *
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

    printf("  %s: ", m->keys[i]);
    print_lval(map_get(m, m->keys[i]));
    putchar('\n');
  }
  printf("}\n");
}

/* int */
/* main(int argc, char **argv) { */
/*   map *m = map_new(); */
/*   lval *l1 = lval_num(1); */
/*   lval *l2 = lval_err("ERROR: I'm an error"); */
/*   lval *l3 = lval_sym("mysym"); */
/*   map_add(m, "first", l1); */
/*   map_add(m, "second", l2); */
/*   map_add(m, "third", l3); */
/*   map_print(m); */
/*   map *m2 = map_copy(m); */
/*   map_add(m2, "fourth", lval_num(22)); */
/*   map_print(m2); */
/*   return 0; */
/* } */
