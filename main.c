//
// Created by Yuan on 2022/11/2.
//
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gen.c"

// op codes
enum {
    HALT,
    CLRA,
    INC3A,
    DECA,
    SELA,
    BACK7
};

int str2i(const char *str, char split, char **endptr)
{
    int ret;
    ret = strtol(str, endptr, 10);
    if (*endptr != NULL) {
        if (**endptr == split) {
            *endptr += 1;
        }
    }

    return ret;
}

int interpreter(char *buf, int size, int a, int l) {
    for (int i = 0; i < size; ++i) {
        switch (buf[i]) {
            case HALT:  return a;
            case CLRA:  a = 0;  break;
            case INC3A: a += 3; break;
            case DECA:  a--;    break;
            case SELA:  l = a;  break;
            case BACK7: l--;    if (l > 0)  i -= 7; break;
        }
    }
    return a;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: ./main + probability(e.g. 00100)\n");
        exit(1);
    }
    // time
    clock_t start_t, end_t;
    // registers
    u_int32_t ip;
    int32_t a, l;

    int size = 50000;
    char *buf = (char *) malloc(sizeof(char) * size);
    int seed = 1;
    int prob[5];
    char *end = NULL;
    char *p_shift = argv[1];

    // decode command line parameters
    for (int i = 0; i < 5; i++) {
        prob[i] = str2i(p_shift, '-', &end);
        p_shift = end;
    }
    start_t = clock();
    init(buf, size, prob, seed, &a, &l);
    end_t = clock();
    a = interpreter(buf, size, a, l);
    printf("%d instructions distributed in %s took %lu cpu clocks\n", size, argv[1], (end_t - start_t));
    return 0;
}