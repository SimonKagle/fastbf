#ifndef BF_H
#define BF_H

#include "bf_utils.h"

bfprog_t *parse_bf(char* prog, long prog_size);
void opt_bf_loops(bfprog_t* prog, bfinst_t *inst, int i);
void opt_bf(bfprog_t* prog);

int run_bfprog(bfprog_t* prog);
int run_bf(char* prog, long prog_size);


#endif