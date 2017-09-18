/*
A well-behaved prompt that we'll use for our REPL
*/

#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

int
main (int argc, char **argv) {
  // just get line and then echo it back

  puts( "MZLisp v0.01");
  puts( "C-c to exit");

  char *input;
  while ((input = readline("MZL> ")) != NULL) {
    add_history(input);
    printf("%s\n", input);
    free(input);
  }

  putchar('\n');
  return 0;
}
