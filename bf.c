#include "bf.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

/**
 * Parses BF program into tree of instructions
 * @param prog Program text
 * @param prog_size Program length
 * @returns Pointer to tree of instruction nodes
 */
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
    bfinst_t* insts = (bfinst_t*)tcalloc(insts_len, sizeof(bfinst_t));

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
            insts = (bfinst_t*)trealloc(insts, sizeof(bfinst_t) * insts_len);
        }
    }

    insts_len = inst_on;
    insts = (bfinst_t*)trealloc(insts, sizeof(bfinst_t) * insts_len);

    bfprog_t* res = (bfprog_t*)tcalloc(1, sizeof(bfprog_t));

    *res = (bfprog_t){
        .length = insts_len,
        .insts = insts,
    };

    return res;
}

int get_num_dsts(const bfinst_t* lbody, long lblen){
    bool ismvadd = true;
    int disp = 0;
    int num_dsts = 0;
    for (int j = 0; j < lblen; j++){
        int kind = lbody[j].kind;
        if (kind != MOVE && kind != ADD && kind != SET && kind != ZERO){
            ismvadd = false;
            break;
        }

        if (kind == MOVE){
            disp += lbody[j].v.amount;
        } else {
            num_dsts++;
        }
    }

    return !ismvadd || disp ? -1 : num_dsts;
}

void convert_to_mvadd(const bfinst_t* lbody, long lblen, bfinst_t *inst){
    int num_dsts = get_num_dsts(lbody, lblen);
        if (num_dsts <= 0){
            return;
        }

        int table_size = 0;
        struct {
            int ptr_offset;
            bool is_set;
            int amnt;
        } madd_dsts[num_dsts];
        int ptr_offset = 0;
        int center_entry = -1;
        for (int pc = 0; pc < lblen; pc++){
            bfinst_t li = lbody[pc];

            bool is_set = false;
            int amnt = 0;
            if (li.kind == MOVE){
                ptr_offset += li.v.amount;
                continue;
            } else if (li.kind == SET || li.kind == ZERO){
                is_set = true;
                amnt = li.kind == ZERO ? 0 : li.v.amount;
            } else if (li.kind == ADD){
                amnt = li.v.amount;
            }

            // Look-up in table
            for (int entry = 0; entry < num_dsts; entry++){
                if (entry >= table_size){
                    madd_dsts[entry].ptr_offset = ptr_offset;
                    madd_dsts[entry].is_set = false;
                    madd_dsts[entry].amnt = 0;
                    table_size++;
                }
                if (madd_dsts[entry].ptr_offset == ptr_offset){
                    if (is_set){
                        madd_dsts[entry].is_set = true;
                        madd_dsts[entry].amnt = amnt;
                    } else {
                        madd_dsts[entry].amnt = amnt;
                    }

                    if (ptr_offset == 0) center_entry = entry;
                    break;
                }
            }
        }

        if (madd_dsts[center_entry].amnt != -1 || madd_dsts[center_entry].is_set) return;

        inst->kind = IF;
        free_prog(&inst->v.body);
        inst->v.body = (bfprog_t*)tcalloc(1, sizeof(bfprog_t));
        inst->v.body->length = table_size;
        inst->v.body->insts = (bfinst_t*)tcalloc(table_size, sizeof(bfinst_t));
        bfinst_t* newbod = inst->v.body->insts;
        for (int entry = 0, ins_pos = 0; entry < table_size; entry++, ins_pos++){
            if (entry == center_entry){
                entry++;
            } 

            newbod[ins_pos] = (bfinst_t){
                .kind = madd_dsts[entry].is_set ? SET_AT : MADD,
                .v.args.amount = madd_dsts[entry].amnt,
                .v.args.dst = madd_dsts[entry].ptr_offset,
            };
            
        }

        newbod[table_size - 1] = (bfinst_t){
            .kind = SET,
            .v.amount = 0
        };
}

void opt_bf_loops(bfprog_t* prog, bfinst_t *inst, int i){
    const bfinst_t* lbody = inst->v.body->insts;
    long lblen = inst->v.body->length;

    while (lblen == 1 && lbody[0].kind == LOOP){
        prog->insts[i] = lbody[0];
    }

    if (lbody[lblen - 1].kind == LOOP){
        inst->kind = IF;
    }

    opt_bf(inst->v.body);


    if (inst->kind == LOOP){
        if (inst->v.body->length == 1 && inst->v.body[0].insts[0].kind == MOVE){
            inst->kind = SKIP;
            int amnt = inst->v.body[0].insts[0].v.amount;
            free_prog(&inst->v.body);
            inst->v.amount = amnt;
        } else {
            convert_to_mvadd(lbody, lblen, inst);
        }
    }
}

void opt_bf(bfprog_t* prog){
    for (int i = 0; i < prog->length; i++){
        bfinst_t *inst = &prog->insts[i];
        switch(inst->kind){
            case LOOP:
                opt_bf_loops(prog, inst, i);
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
            
            case SET_AT:
                *(ptr + inst.v.args.dst) = inst.v.args.amount;
                break;

            case MADD:
                *(ptr + inst.v.args.dst) += *ptr * inst.v.args.amount;
                break;
            
            case SKIP:
                if (!*ptr) break;
                for (; *ptr && ptr >= tape && ptr < tape + TAPE_SIZE; ptr += inst.v.amount);
                if (*ptr || !(ptr >= tape && ptr < tape + TAPE_SIZE)) return 1;
                break;
            
            default: break;
            
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
    
    char* prog = (char*)tcalloc(s.st_size, sizeof(char));

    // fread(prog, sizeof(char), s.st_size, f);
    // clock_t start = clock();
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
    // printf("%lf\n", ((double)(clock() - start))/CLOCKS_PER_SEC);
    int ret = -1;
    if (pinst){
        ret = run_bfprog(pinst);
    }

    free_prog(&pinst);

    free(prog);
    prog = NULL;

    return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}