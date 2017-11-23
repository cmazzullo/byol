#ifndef MAP_H
#define MAP_H

#include "lval.h"

typedef struct map map;

map *map_new(void);
void map_delete(map *m);
map *map_copy(map *m);
void map_print(map *m);

void map_add(map *m, lval *key, lval *val);
lval *map_get(map *m, lval *key);
bool map_contains(map *m, lval *key);
void map_remove(map *m, lval *key);

#endif
