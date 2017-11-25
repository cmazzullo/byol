#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct list list;
typedef list lenv;
typedef struct lval lval;
typedef lval* (*lbuiltin)(lenv *, lval *);
typedef struct map map;

#endif
