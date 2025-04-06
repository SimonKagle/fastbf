#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>

#define TAPE_SIZE 3000

struct bfinst;
struct bfprog;

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
    } kind;

    union {
        int amount;
        struct bfprog *body;
    } v;

} bfinst_t;

typedef struct bfprog {
    int length;
    bfinst_t* insts;
} bfprog_t;

typedef unsigned char bfword;

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

bfprog_t *parse_bf(char* prog, long prog_size){

    // Remove non-bf characters
    // int offset = 0;
    // for (int i = 0; i < prog_size; i++){
    //     if (!is_bf_char(prog[i + offset])){
    //         offset++;
    //         prog_size--;
    //     }
    //     if (offset){
    //         prog[i] = prog[i + offset];
    //     }
    // }

    int insts_len = 64;
    bfinst_t* insts = (bfinst_t*)calloc(insts_len, sizeof(bfinst_t));
    if (!insts){
        fprintf(stderr, "Could not create buffer for instructions!");
        return NULL;
    }

    int inst_on = 0;

    for (int i = 0; i < prog_size; i++){
        char c = prog[i];
        switch (c){
            case '+':
            case '-': {
                    int counter = 0;
                    for (; i < prog_size && (prog[i] == '+' || prog[i] == '-'); i++){
                        counter += prog[i] == '+' ? 1 : -1;
                    }

                    if (counter == 0){
                        continue;
                    }

                    insts[inst_on] = (bfinst_t){
                        .kind = ADD,
                        .v.amount = counter
                    };

                    i--;
                    //printf("inst: ADD %d\n", counter);
                }
                break;
            
            case '<':
            case '>': {
                    int counter = 0;
                    for (; i < prog_size && (prog[i] == '>' || prog[i] == '<'); i++){
                        counter += prog[i] == '>' ? 1 : -1;
                    }

                    if (counter == 0){
                        continue;
                    }

                    insts[inst_on] = (bfinst_t){
                        .kind = MOVE,
                        .v.amount = counter
                    };

                    i--;
                    //printf("inst: MOVE %d\n", counter);
                }
                break;

            case ',': 
                insts[inst_on] = (bfinst_t){.kind = INPUT};
                //printf("inst: INPUT\n");
                break;

            case '.': 
                insts[inst_on] = (bfinst_t){.kind = OUTPUT};
                //printf("inst: OUTPUT\n");
                break;

            case '[': {
                    int end = find_matching(i, '[', prog, prog_size);
                    if (end < 0){
                        fprintf(stderr, "Mismatched left bracket at pc=%d\n", i);
                        free(insts);
                        return NULL;
                    }

                    if (end - i - 1 == 1 && (prog[i + 1] == '-' || prog[i + 1] == '+')){
                        insts[inst_on] = (bfinst_t){
                            .kind = ZERO,
                        };
                    } else {
                        bfprog_t *body = parse_bf(prog + i + 1, end - i - 1);
                        if (!body){
                            free(insts);
                            return NULL;
                        }
                        insts[inst_on] = (bfinst_t){
                            .kind = LOOP,
                            .v.body = body,
                        };
                    }

                    i = end;
                }
                break;
            
            case ']':
                fprintf(stderr, "Mismatched right bracket at pc=%d\n", i);
                free(insts);
                return NULL;
            
            default:
                inst_on--;
                break;
                
        }

        inst_on++;
        if (inst_on >= insts_len){
            insts_len *= 2;
            bfinst_t* new_insts = (bfinst_t*)realloc(insts, sizeof(bfinst_t) * insts_len);
            if (!new_insts){
                fprintf(stderr, "Could not allocate %d instructions!", insts_len);
                free(insts);
                return NULL;
            }
            insts = new_insts;
        }
    }

    insts_len = inst_on;
    bfinst_t* new_insts = (bfinst_t*)realloc(insts, sizeof(bfinst_t) * insts_len);
    if (!new_insts){
        fprintf(stderr, "Could not allocate %d instructions!", insts_len);
        free(insts);
        return NULL;
    }
    insts = new_insts;

    bfprog_t* res = (bfprog_t*)calloc(1, sizeof(bfprog_t));
    if (!res){
        fprintf(stderr, "Could not allocate program!\n");
        free(insts);
        return NULL;
    }

    *res = (bfprog_t){
        .length = insts_len,
        .insts = insts,
    };

    return res;
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

void opt_bf(bfprog_t* prog){
    for (int i = 0; i < prog->length; i++){
        bfinst_t *inst = &prog->insts[i];
        switch(inst->kind){
            case LOOP: {
                    while (inst->v.body->length == 1 && inst->v.body->insts[0].kind == LOOP){
                        prog->insts[i] = inst->v.body->insts[0];
                    }

                    if (inst->v.body->insts[inst->v.body->length - 1].kind == LOOP){
                        inst->kind = IF;
                    }

                    opt_bf(inst->v.body);
                }
                break;
            
            case ZERO: {
                    if (i + 1 < prog->length){
                        if (prog->insts[i + 1].kind == ZERO){
                            del_insts(prog, i, i + 1);
                            i--;
                        } else if (prog->insts[i + 1].kind == ADD){
                            prog->insts[i].kind = SET;
                            prog->insts[i].v.amount = prog->insts[i + 1].v.amount;
                            del_insts(prog, i + 1, i + 2);
                            i--;
                        } else {
                            break;
                        }
                    }
                }
                break;

            default: break;
        }
    }
}

bfword tape[TAPE_SIZE] = {0};
bfword *ptr = tape;

int run_bfprog(bfprog_t* prog){
    for (int pc = 0; pc < prog->length; pc++){
        bfinst_t inst = prog->insts[pc];
        switch(inst.kind){
            case ADD:
                *ptr += inst.v.amount;
                break;

            case MOVE:
                ptr += inst.v.amount;
                if (ptr < tape || ptr >= tape + TAPE_SIZE){
                    fprintf(stderr, "Pointer out of bounds at pc=%d\n", pc);
                    return -1;
                }
                break;

            case OUTPUT:
                putchar(*ptr);
                break;

            case INPUT:
                *ptr = getchar();
                break;
            
            case LOOP:
                while (*ptr){
                    int exit = run_bfprog(inst.v.body);
                    if (exit < 0){
                        return exit;
                    }
                }
                break;
            
            case IF:
                if (*ptr){
                    int exit = run_bfprog(inst.v.body);
                    if (exit < 0){
                        return exit;
                    }
                }
                break;
            
            case ZERO:
                *ptr = 0;
                break;
                
            case SET:
                *ptr = inst.v.amount;
                break;
            
        }
    }

    return 0;
}

int run_bf(char* prog, long prog_size){
    bfword tape[TAPE_SIZE] = { 0 };
    bfword *ptr = tape;

    for (long pc = 0; pc < prog_size; pc++){
        char instr = prog[pc];
        // //printf("%ld %ld %c\n", pc, ptr - tape, instr);
        switch(instr){
            case '+':
            case '-':
                *ptr += instr == '+' ? 1 : -1;
                break;
            case '<':
            case '>':
                ptr += instr == '>' ? 1 : -1;
                if (ptr < tape || ptr >= tape + TAPE_SIZE){
                    fprintf(stderr, "Pointer out of bounds at pc=%ld\n", pc);
                    return -1;
                }
                break;
            case '.':
                putchar(*ptr);
                break;
            case ',':
                *ptr = getchar();
                break;
            case '[':
            case ']': {
                    int matching = find_matching(pc, instr, prog, prog_size) - 1;
                    if (matching < 0){
                        fprintf(stderr, "Brackets unbalanced!\n");
                        return -1;
                    }

                    if ((instr == '[' && !*ptr) || (instr == ']' && *ptr)) pc = matching;
                }
                break;
        }
    }

    return 0;
}

int main(int argc, char** argv){
    FILE* f = fopen(argv[1], "r");
    if (!f){
        fprintf(stderr, "Could not open file!\n");
        return EXIT_FAILURE;
    }

    struct stat s = {0};
    stat(argv[1], &s);
    
    char* prog = (char*)calloc(s.st_size, sizeof(char));
    if (!prog){
        fclose(f);
        fprintf(stderr, "Could not allocate space for program!\n");
        return EXIT_FAILURE;
    }

    // fread(prog, sizeof(char), s.st_size, f);
    clock_t start = clock();
    int prog_size = 0;
    while (!feof(f)){
        int c = getc(f);
        if (is_bf_char(c)){
            prog[prog_size++] = c;
        }
    }
    fclose(f);

    // int ret = run_bf(prog, prog_size);
    bfprog_t* pinst = parse_bf(prog, prog_size);
    opt_bf(pinst);
    printf("%lf\n", ((double)(clock() - start))/CLOCKS_PER_SEC);
    int ret = -1;
    if (pinst){
        ret = run_bfprog(pinst);
    }

    free_prog(&pinst);

    free(prog);
    prog = NULL;

    return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}