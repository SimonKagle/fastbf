#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define TAPE_SIZE 3000

int find_matching(int start, char instr, char* prog, long prog_size){
    int dir = instr == '[' ? 1 : -1;
    char other = instr == '[' ? ']' : '[';
    int invar = 0;
    int pos = start;
    while (pos > 0 && pos < prog_size){
        if (prog[pos] == instr) invar++;
        if (prog[pos] == other) invar--;

        if (invar < 0) return -1;
        else if (invar == 0) return pos;

        pos += dir;
    }

    return -1;

}

int run_bf(char* prog, long prog_size){
    unsigned char tape[TAPE_SIZE] = { 0 };
    unsigned char *ptr = tape;

    for (long pc = 0; pc < prog_size; pc++){
        char instr = prog[pc];
        // printf("%ld %ld %c\n", pc, ptr - tape, instr);
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
    long prog_size = s.st_size;
    
    char* prog = (char*)calloc(prog_size, sizeof(char));
    if (!prog){
        fclose(f);
        fprintf(stderr, "Could not allocate space for program!\n");
        return EXIT_FAILURE;
    }

    fread(prog, sizeof(char), s.st_size, f);
    fclose(f);

    int ret = run_bf(prog, prog_size);

    free(prog);
    prog = NULL;

    return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}