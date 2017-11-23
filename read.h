#ifndef READ_H
#define READ_H

#include "mpc/mpc.h"
#include "core.h"

lval *read(mpc_ast_t *t);
lval *read_num(mpc_ast_t *t);
lval *read_bool(mpc_ast_t *t);

#endif
