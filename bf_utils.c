#include "bf_utils.h"

#include <stdio.h>

void* tcalloc(size_t n, size_t size){
    void* res = calloc(n, size);
    if (!res){
        fprintf(stderr, "Could not allocate %zu elements of %zu!\n", n, size);
        exit(42);
    }

    return res;
}

void* trealloc(void* p, size_t size){
    void* res = realloc(p, size);
    if (!res){
        fprintf(stderr, "Could not reallocate %zu bytes for pointer %p!\n", size, p);
        exit(42);
    }

    return res;
}

/**
 * Finds the index in the program text of the matching '[' or ']'
 * @param start Index to start search
 * @param instr Which square bracket we currently have, and are looking for the pair of
 * @param prog Program text
 * @param prog_size Length of program text
 * @returns Index of the corresponding square bracket, or -1 if not found
 */
int find_matching(int start, char instr, char* prog, long prog_size){
    int dir = instr == '[' ? 1 : -1;
    char other = instr == '[' ? ']' : '[';
    int invar = 0;
    int pos = start;
    while (pos >= 0 && pos < prog_size){
        if (prog[pos] == instr) invar++;
        if (prog[pos] == other) invar--;

        if (invar < 0) return -1;
        else if (invar == 0) return pos;

        pos += dir;
    }

    return -1;

}

void free_prog(bfprog_t** p){
    if (!p || !(*p)){
        return;
    }

    if ((*p)->insts){
        for (int i = 0; i < (*p)->length; i++){
            if ((*p)->insts[i].kind == LOOP || (*p)->insts[i].kind == IF){
                free_prog(&(*p)->insts[i].v.body);
                (*p)->insts[i].v.body = NULL;
            }
            (*p)->insts[i] = (bfinst_t){ 0 };
        }
        free((*p)->insts);
        (*p)->insts = NULL;
    }

    free(*p);
    *p = NULL;
}

bool is_bf_char(char c){
    return c == '[' || c == ']' || c == '+' || c == '-' || c == '>' || c == '<' || c == '.' || c == ',';
}


// end exclusive
void del_insts(bfprog_t* prog, int start, int end){
    if (end > prog->length){
        end = prog->length;
    }
    int del_len = end - start;
    for (int i = start; i < prog->length; i++){
        if (i < end && (prog->insts[i].kind == LOOP || prog->insts[i].kind == IF)){
            free_prog(&prog->insts[i].v.body);
        }

        if (i + del_len >= prog->length){
            break;
        }

        prog->insts[i] = prog->insts[i + del_len];
    }
    prog->length -= del_len;

}