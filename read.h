#ifndef READ_H
#define READ_H

#include "mpc/mpc.h"
#include "structs.h"
#include "lval.h"

void read_initialize(void);
void read_cleanup(void);
lval *read(mpc_ast_t *t);
lval *read_line(char *line);
lval *read_num(mpc_ast_t *t);
lval *read_bool(mpc_ast_t *t);

#endif
