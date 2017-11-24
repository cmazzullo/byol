#include <string.h>

#include "read.h"

lval *
read_num(mpc_ast_t *t) // Convert an AST to an LVAL containing a number
{
  long x = strtol(t->contents, NULL, 10);
  if (errno != ERANGE) return lval_num(x);
  else return lval_err("ERROR: Invalid number!");
}

lval *
read_bool(mpc_ast_t *t)
{
  if (strcmp(t->contents, "true") == 0) {
    return lval_bool(true);
  } else {
    return lval_bool(false);
  }
}

lval *
read(mpc_ast_t *t) // convert the AST into a sexp
{
  // if the tree is the root, return its first child
  if (strcmp(t->tag, ">") == 0) {
    return read(t->children[1]);
  }
  if (strstr(t->tag, "bool")) { return read_bool(t); }
  if (strstr(t->tag, "num")) { return read_num(t); }
  if (strstr(t->tag, "sym")) { return lval_sym(t->contents); }

  // if the tree a sexp then create an empty sexp
  lval *v;
  if (strstr(t->tag, "sexp")) {
    v = lval_sexp();
  }
  // fill the empty sexp with any expressions within
  // need to go through backwards for the cons to work
  for (int i = t->children_num - 1; i >= 0; i--) {
    if (strcmp(t->children[i]->tag, "regex") == 0) { continue; } // skip the noise
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    lval_cons(v, read(t->children[i]));
  }
  return v;
}
