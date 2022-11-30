//
// Created by Yuan on 2022/11/2.
//
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gen.c"

// op codes
enum
{
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
    if (*endptr != NULL)
    {
        if (**endptr == split)
        {
            *endptr += 1;
        }
    }

    return ret;
}

int switch_interpreter(char *instructions, int a, int l)
{
    int pc = 0;
    while (1)
    {
        switch (instructions[pc])
        {
        case HALT:
            return a;
        case CLRA:
            a = 0;
            break;
        case INC3A:
            a += 3;
            break;
        case DECA:
            a--;
            break;
        case SELA:
            l = a;
            break;
        case BACK7:
            l--;
            if (l > 0)
                pc -= 7;
            break;
        }
        pc++;
    }
}

int indirect_threaded_interpreter(char *instructions, int a, int l)
{
    static void *dispatch_table[] = {&&DO_HALT, &&DO_CLRA, &&DO_INC3A, &&DO_DECA, &&DO_SELA, &&DO_BACK7};
#define DISPATCH() goto *dispatch_table[instructions[pc++]]

    int pc = 0;

    // fire
    DISPATCH();
    while (1)
    {
    DO_HALT:
        return a;
    DO_CLRA:
        a = 0;
        DISPATCH();
    DO_INC3A:
        a += 3;
        DISPATCH();
    DO_DECA:
        a--;
        DISPATCH();
    DO_SELA:
        l = a;
        DISPATCH();
    DO_BACK7:
        l--;
        if (l > 0)
            pc -= 7;
        DISPATCH();
    }
}

int direct_threaded_interpreter(char *instructions, int size, int a, int l)
{
    static void *dispatch_table[] = {&&DO_HALT, &&DO_CLRA, &&DO_INC3A, &&DO_DECA, &&DO_SELA, &&DO_BACK7};

#define DISPATCH_D() goto *(void *)intermediate_instructions[pc++]
    // precoding
    int64_t *intermediate_instructions = malloc(sizeof(int64_t) * size);
    for (int i = 0; i < size; i += 4)
    {
        intermediate_instructions[i] = (int64_t)dispatch_table[instructions[i]];
        intermediate_instructions[i+1] = (int64_t)dispatch_table[instructions[i+1]];
        intermediate_instructions[i+2] = (int64_t)dispatch_table[instructions[i+2]];
        intermediate_instructions[i+3] = (int64_t)dispatch_table[instructions[i+3]];
    }

    int pc = 0;

    // fire
    DISPATCH_D();
    while (1)
    {
    DO_HALT:
        return a;
    DO_CLRA:
        a = 0;
        DISPATCH_D();
    DO_INC3A:
        a += 3;
        DISPATCH_D();
    DO_DECA:
        a--;
        DISPATCH_D();
    DO_SELA:
        l = a;
        DISPATCH_D();
    DO_BACK7:
        l--;
        if (l > 0)
            pc -= 7;
        DISPATCH_D();
    }
}

int superevent_interpreter(char *instructions, int a, int l)
{
    int pc = 0;
    while (1)
    {
        if (instructions[pc] == HALT)
            return a;
        else if (instructions[pc] == BACK7)
        {
            l--;
            if (l > 0)
                pc -= 7;
            pc++;
        }
        else
        {
            if ((instructions[pc] == CLRA || instructions[pc] == INC3A || instructions[pc] == DECA) && instructions[pc + 1] == CLRA)
                a = 0;
            else if (instructions[pc] == SELA)
            {
                l = a;
                switch (instructions[pc + 1])
                {
                case HALT:
                    return a;
                case CLRA:
                    a = 0;
                    break;
                case INC3A:
                    a += 3;
                    break;
                case DECA:
                    a--;
                    break;
                case SELA:
                    l = a;
                    break;
                case BACK7:
                    l--;
                    if (l > 0)
                        pc -= 6;
                    break;
                }
            }
            else if (instructions[pc] == CLRA)
            {
                switch (instructions[pc + 1])
                {
                case HALT:
                    return 0;
                case INC3A:
                    a = 3;
                    break;
                case DECA:
                    a = -1;
                    break;
                case SELA:
                    l = a = 0;
                    break;
                case BACK7:
                    l--;
                    if (l > 0)
                        pc -= 6;
                    break;
                }
            }
            else if (instructions[pc] == INC3A)
            {
                switch (instructions[pc + 1])
                {
                case HALT:
                    return a + 3;
                case INC3A:
                    a += 6;
                    break;
                case DECA:
                    a += 2;
                    break;
                case SELA:
                    a += 3;
                    l = a;
                    break;
                case BACK7:
                    l--;
                    if (l > 0)
                        pc -= 6;
                    break;
                }
            }
            else if (instructions[pc] == DECA)
            {
                switch (instructions[pc + 1])
                {
                case HALT:
                    return a - 1;
                case INC3A:
                    a += 2;
                    break;
                case DECA:
                    a -= 2;
                    break;
                case SELA:
                    a--;
                    l = a;
                    break;
                case BACK7:
                    l--;
                    if (l > 0)
                        pc -= 6;
                    break;
                }
            }
            pc += 2;
        }
    }
}

int main(int argc, char **argv)
{
    // time
    clock_t start_t, end_t;
    // registers
    u_int32_t ip;
    int32_t a, l;

    int size = 50000, iteration = 5;
    char *p_shift = "1-9-1-5-5";

    if (argc == 4)
    {
        size = atoi(argv[1]);
        iteration = atoi(argv[2]);
        p_shift = argv[3];
        printf("\nProgram continues with %d instruction(s) distributed in %s, each interpreter will run %d times\n\n", size, p_shift, iteration);
    }
    else
    {
        printf("\nUsage: ./main size iteration probability\n");
        printf("Program continues with default set: %d instruction(s) distributed in %s, each interpreter will run %d times\n\n", size, p_shift, iteration);
    }

    char *instructions = (char *)malloc(sizeof(char) * size);
    int seed = 1;
    int prob[5];

    // decode command line parameters
    char *end = NULL;
    for (int i = 0; i < 5; i++)
    {
        prob[i] = str2i(p_shift, '-', &end);
        p_shift = end;
    }
    // init instructions
    init(instructions, size, prob, seed, &a, &l);
    int a_init = a, l_init = l;

    // run different interpreters
    for (int i = 0; i < iteration; i++)
    {
        start_t = clock();
        a = switch_interpreter(instructions, a, l);
        end_t = clock();
        printf("switch interpreter took %lu cpu clocks, final value of a: %d\n", (end_t - start_t), a);
        a = a_init;
        l = l_init;
    }
    printf("/*---------------------------------------------------------------*/\n");
    for (int i = 0; i < iteration; i++)
    {
        start_t = clock();
        a = indirect_threaded_interpreter(instructions, a, l);
        end_t = clock();
        printf("indirect threaded interpreter took %lu cpu clocks, final value of a: %d\n", (end_t - start_t), a);
        a = a_init;
        l = l_init;
    }
    printf("/*---------------------------------------------------------------*/\n");
    for (int i = 0; i < iteration; i++)
    {
        start_t = clock();
        a = direct_threaded_interpreter(instructions, size, a, l);
        end_t = clock();
        printf("direct threaded interpreter took %lu cpu clocks, final value of a: %d\n", (end_t - start_t), a);
        a = a_init;
        l = l_init;
    }
    printf("/*---------------------------------------------------------------*/\n");
    for (int i = 0; i < iteration; i++)
    {
        start_t = clock();
        a = superevent_interpreter(instructions, a, l);
        end_t = clock();
        printf("super event interpreter took %lu cpu clocks, final value of a: %d\n", (end_t - start_t), a);
        a = a_init;
        l = l_init;
    }

    return 0;
}