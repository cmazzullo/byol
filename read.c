#include "read.h"
#include <string.h>

// Global definition for the parser
mpc_parser_t *String, *Bool, *Num, *Symbol, *Exp, *Sexp, *Input;

void
read_initialize(void)
{
  extern mpc_parser_t *String;
  extern mpc_parser_t *Bool;
  extern mpc_parser_t *Num;
  extern mpc_parser_t *Symbol;
  extern mpc_parser_t *Exp;
  extern mpc_parser_t *Sexp;
  extern mpc_parser_t *Input;
  Input = mpc_new("input");
  String = mpc_new("string");
  Bool = mpc_new("bool");
  Num = mpc_new("num");
  Symbol = mpc_new("symbol");
  Exp = mpc_new("exp");
  Sexp = mpc_new("sexp");

  mpca_lang(MPCA_LANG_DEFAULT,"\
  string : /\"(\\\\.|[^\"])*\"/ ;					\
  bool : \"true\" | \"false\" ;						\
  num : /-?[0-9]+/ ;							\
  symbol : /[a-zA-Z0-9*+\\-\\/\\\\_=<>!&]+/ ;				\
  sexp : '(' <exp>* ')' ;						\
  exp : <string> | <bool> | <num> | <symbol> | <sexp> ; \
  input : /^/ <exp>? /$/ ;", String, Bool, Num, Symbol, Sexp, Exp, Input);
}

void
read_cleanup(void)
{
  extern mpc_parser_t *String;
  extern mpc_parser_t *Bool;
  extern mpc_parser_t *Num;
  extern mpc_parser_t *Symbol;
  extern mpc_parser_t *Exp;
  extern mpc_parser_t *Sexp;
  extern mpc_parser_t *Input;
  mpc_cleanup(7, String, Bool, Num, Symbol, Sexp, Exp, Input);
}

lval *
read_num(mpc_ast_t *t) // Convert an AST to an LVAL containing a number
{
  long x = strtol(t->contents, NULL, 10);
  if (errno != ERANGE) return lval_num(x);
  else return lval_err("ERROR: Invalid number!");
}


lval *
read_string(mpc_ast_t *t) // Convert an AST to an LVAL containing a number
{
  t->contents[strlen(t->contents) - 1] = '\0'; // Cut off last quote
  char *str = malloc(strlen(t->contents + 1) + 1);
  strcpy(str, t->contents + 1);
  lval *s = lval_string(str);
  free(str);
  return s;
}

lval *
read_line(char *line)
{
  mpc_result_t r;
  if (mpc_parse("<stdin>", line, Input, &r)) {
    lval *lval_input = read(r.output);
    mpc_ast_delete(r.output);
    return lval_input;
  } else {
    return lval_err("ERROR: Syntax error!");
  }
}

lval *
read(mpc_ast_t *t) // convert the AST into a sexp
{
  // if the tree is the root, return its first child
  if (strcmp(t->tag, ">") == 0) {
    return read(t->children[1]);
  }
  if (strstr(t->tag, "bool")) { return lval_bool(strcmp(t->contents, "true") == 0); }
  if (strstr(t->tag, "string")) { return read_string(t); }
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
