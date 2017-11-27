#ifndef READ_H
#define READ_H

#include "structs.h"

void read_initialize(void);
void read_cleanup(void);

lval *read_line(char *line);
lval *read_file(char *fname);

#endif
