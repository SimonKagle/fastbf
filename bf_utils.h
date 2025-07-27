#ifndef BF_UTILS_H
#define BF_UTILS_H

#include <stdlib.h>
#include <stdbool.h>

#define TAPE_SIZE 3000

typedef struct bfinst {
    enum {
        ADD,
        MOVE,
        INPUT,
        OUTPUT,
        LOOP,
        ZERO,
        IF,
        SET,
        MADD,
        SET_AT,
        SKIP,
    } kind;

    union {
        int amount;
        struct bfprog *body;
        struct {
            int dst;
            int amount;
        } args;
    } v;

} bfinst_t;

typedef struct bfprog {
    int length;
    bfinst_t* insts;
} bfprog_t;

typedef unsigned char bfword;

void* tcalloc(size_t n, size_t size);
void* trealloc(void* p, size_t size);

int find_matching(int start, char instr, char* prog, long prog_size);
void free_prog(bfprog_t** p);
bool is_bf_char(char c);
void del_insts(bfprog_t* prog, int start, int end);

#endif