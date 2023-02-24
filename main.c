//
// Created by Yuan on 2022/11/2.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include "gen.c"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"

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

uint64_t rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

void put_head(FILE* stream, uint8_t a_init) {
    fputc(0x55, stream);
    fputc(0x48, stream);
    fputc(0x89, stream);
    fputc(0xe5, stream);
    fputc(0xc7, stream);
    fputc(0x45, stream);
    fputc(0xfc, stream);
    fputc(a_init, stream);
    fputc(0x00, stream);
    fputc(0x00, stream);
    fputc(0x00, stream);
}

void put_CLRA(FILE* stream) {
    fputc(0xc7, stream);
    fputc(0x45, stream);
    fputc(0xfc, stream);
    fputc(0x00, stream);
    fputc(0x00, stream);
    fputc(0x00, stream);
    fputc(0x00, stream);
}

void put_INC3A(FILE* stream) {
    fputc(0x83, stream);
    fputc(0x45, stream);
    fputc(0xfc, stream);
    fputc(0x03, stream);
}

void put_DECA(FILE* stream) {
    fputc(0x83, stream);
    fputc(0x6d, stream);
    fputc(0xfc, stream);
    fputc(0x01, stream);
}

void put_tail(FILE* stream) {
    fputc(0x8b, stream);
    fputc(0x45, stream);
    fputc(0xfc, stream);
    fputc(0x5d, stream);
    fputc(0xc3, stream);
}

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

int indirect_threaded_interpreter_precoding(char *instructions, int size, int a, int l)
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

void gen_x86stream(char *instructions, int size, FILE* stream, uint8_t a_init) {
    fflush(stream);
    put_head(stream, a_init);
    for (int i = 0; i < size - 1; ++i)
    {
        switch (instructions[i]) 
        {
            case CLRA:
                put_CLRA(stream);
                break;
            case INC3A:
                put_INC3A(stream);
                break;
            case DECA:
                put_DECA(stream);
                break;
        }
    }
    put_tail(stream);
    fclose(stream);
}

void indirect_x86gen(char *instructions, FILE* stream, uint8_t a_init) {
    static void *dispatch_table[] = {&&x86_HALT, &&x86_CLRA, &&x86_INC3A, &&x86_DECA};
#define x86_DISPATCH() goto *dispatch_table[instructions[pc++]]
    int pc = 0;
    put_head(stream, a_init);
    x86_DISPATCH();
    while (1) {
        x86_HALT:
            put_tail(stream);
            break;
        x86_CLRA:
            put_CLRA(stream);
            x86_DISPATCH();
        x86_INC3A:
            put_INC3A(stream);
            x86_DISPATCH();
        x86_DECA:
            put_DECA(stream);
            x86_DISPATCH();
    }
    fclose(stream);
}

int cal_average(int *cycles, int size) {
    int sum = 0;
    for (int i = 0; i < size; i++)
    {
        sum += cycles[i];
    }
    return sum / size;
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

    int *cycles = (int *)malloc(sizeof(int) * iteration);
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
        start_t = rdtsc();
        a = switch_interpreter(instructions, a, l);
        end_t = rdtsc();
        cycles[i] = (end_t - start_t);
        printf("switch interpreter took %d cpu clocks, final value of a: %d\n", cycles[i], a);
        a = a_init;
        l = l_init;
    }
    printf("On average, switch interpreter took %s%d%s cpu clocks\n", KRED, cal_average(cycles, iteration), KNRM);
    printf("/*---------------------------------------------------------------*/\n");
    for (int i = 0; i < iteration; i++)
    {
        start_t = rdtsc();
        a = indirect_threaded_interpreter(instructions, a, l);
        end_t = rdtsc();
        cycles[i] = (end_t - start_t);
        printf("indirect threaded interpreter took %d cpu clocks, final value of a: %d\n", cycles[i], a);
        a = a_init;
        l = l_init;
    }
    printf("On average, indirect threaded interpreter took %s%d%s cpu clocks\n", KRED, cal_average(cycles, iteration), KNRM);
    printf("/*---------------------------------------------------------------*/\n");
    for (int i = 0; i < iteration; i++)
    {
        start_t = rdtsc();
        a = indirect_threaded_interpreter_precoding(instructions, size, a, l);
        end_t = rdtsc();
        cycles[i] = (end_t - start_t);
        printf("direct threaded interpreter took %d cpu clocks, final value of a: %d\n", cycles[i], a);
        a = a_init;
        l = l_init;
    }
    printf("On average, direct threaded interpreter took %s%d%s cpu clocks\n", KRED, cal_average(cycles, iteration), KNRM);

    // Bad implementation of super-event interpreter
    // printf("/*---------------------------------------------------------------*/\n");
    // for (int i = 0; i < iteration; i++)
    // {
    //     start_t = clock();
    //     a = superevent_interpreter(instructions, a, l);
    //     end_t = clock();
    //     cycles[i] = (end_t - start_t);
    //     printf("super event interpreter took %d cpu clocks, final value of a: %d\n",cycles[i], a);
    //     a = a_init;
    //     l = l_init;
    // }
    // printf("On average, super event interpreter took %d cpu clocks\n", cal_average(cycles, iteration));

    printf("/*---------------------------------------------------------------*/\n");
    
    char * buf;
    size_t len;
    FILE* stream = open_memstream(&buf, &len);
    if (stream == NULL)
        printf("open_memstream failed\n");
    /* generate x86 instruction stream */
    gen_x86stream(instructions, size, stream, a_init);
    /* mmap a region for our code */
    void *p = mmap(NULL, len, PROT_READ|PROT_WRITE,  /* No PROT_EXEC */
            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0); 
    if (p==MAP_FAILED) {
        fprintf(stderr, "mmap() failed\n");
        return 2;
    }
    memcpy(p, buf, len);

    /* Now make it execute-only */
    if (mprotect(p, len, PROT_EXEC) < 0) {
        fprintf(stderr, "mprotect failed to mark exec-only\n");
        return 2;
    } 

    int (*func)(void) = p;
    for (int i = 0; i < iteration; i++)
    {
        start_t = rdtsc();
        a = func();
        end_t = rdtsc();
        cycles[i] = (end_t - start_t);
        printf("x86 stream took %d cpu clocks, final value of a: %d\n", cycles[i], a);
    }
    free(buf);
    printf("On average, x86 stream took %s%d%s cpu clocks\n", KRED, cal_average(cycles, iteration), KNRM);
    return 0;
}