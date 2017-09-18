/*
A well-behaved REPL
*/

#include "../mpc/mpc.h"

#include <stdio.h>
#include <stdlib.h>

/* #include <readline/readline.h> */
/* #include <readline/history.h> */


void run_repl(mpc_parser_t *Input);
void parse_input(char *line, mpc_parser_t *Input);
int evaluate_tree(mpc_ast_t *ast);


int number_of_nodes(mpc_ast_t *ast) {
  if (ast->children_num == 0)
    return 1;
  int sum = 0;
  for (int i = 0; i < ast->children_num; i++) {
    sum += number_of_nodes(ast->children[i]);
  }
  return sum;
}


int eval_exp(mpc_ast_t *ast) {
  printf(ast->tag);
  if (strstr(ast->tag, "num")) {
    return atoi(ast->contents);
  }
  int sum = 0;
  for (int i = 2; i < ast->children_num - 1; i++) {
    sum += eval_exp(ast->children[i]);
  }
  return sum;
}


int main (int argc, char **argv) {

  mpc_parser_t *Num = mpc_new("num");
  mpc_parser_t *Op = mpc_new("op");
  mpc_parser_t *Exp = mpc_new("exp");
  mpc_parser_t *Input = mpc_new("input");

  // Prefix Notation
  mpca_lang(MPCA_LANG_DEFAULT," \
op: '+' ; \
num: /-?[0-9]+/ ;                    \
exp: <num> | '(' <op> <exp>* ')' ;   \
input: /^/ <op> <exp>+ /$/ ;    \
", Op, Num, Exp, Input);

  run_repl(Input);

  mpc_cleanup(4, Num, Op, Exp, Input);
  return 0;
}


void parse_input(char *line, mpc_parser_t *Input) {
  mpc_result_t r;

  if (mpc_parse("<stdin>", line, Input, &r)) {

    mpc_ast_t* a = r.output;

    // printf("%d\n", number_of_nodes(a));

    /* int i; */
    /* for (i = 0; i < a->children_num; i++) { */
    /*   mpc_ast_t *child = a->children[i]; */
    /*   printf("tag: %s\ncontents: %s\n\n", child->tag, child->contents); */
    /* } */

    /* mpc_ast_print(r.output); */
    /* mpc_ast_delete(r.output); */
    printf("evalled: %d\n", eval_exp(r.output));
  } else {
    mpc_err_print(r.error);
    mpc_err_delete(r.error);
  }
}


/* void run_repl(mpc_parser_t *Input) { */
/*   char *line; */

/*   puts( "MZLisp v0.01"); */
/*   puts( "C-c to exit"); */
/*   while ((line = readline("MZL> ")) != NULL) { */
/*     parse_input(line, Input); */
/*     add_history(line); */
/*     free(line); */
/*   } */
/*   putchar('\n'); */
/* } */


void run_repl(mpc_parser_t *Input) {
  char *line;

  puts( "MZLisp v0.01");
  puts( "C-c to exit");

  while (1) {
    printf("MZL> ");
    line = gets(line);
    parse_input(line, Input);
  }
  putchar('\n');
}
