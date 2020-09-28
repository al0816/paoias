#ifndef paoias_h
#define paoias_h

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

const char * const opts = "i:r:c:hv";
static struct option long_options[] = {
    {"interpret", required_argument, NULL, 'i'},
    {"run", required_argument, NULL, 'r'},
    {"compile", required_argument, NULL, 'c'},
    {"help", no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'v'},
    {0, 0, 0, 0}
};


enum registers {
    eax,
    ebx,
    ecx,
    edx,
    eip
};

enum flags {
    zf,
    sf
};

enum instructions {
    mov_mem,
    mov_to,
    mov_l,
    mov_reg,
    mov_t,
    test_r,
    jz_l,
    add_to,
    sub_to,
    halt
};

void print_state();

#endif
