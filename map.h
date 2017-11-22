#ifndef MAP_H
#define MAP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core.h"

#define ARRAYSIZE 1024

typedef struct list list;
typedef struct map map;

int hash(char *key);
list *new_list(char *key, lval *val);
list *cdr(list *l);
list *cons(char *key, lval *val, list *l);
lval *list_get(list *l, char *key);
map *map_new(void);
map *map_copy(map *m);
void map_add(map *m, char *key, lval *val);
lval *map_get(map *m, char *key);
void map_print(map *m);
void map_del(map *m);

#endif
