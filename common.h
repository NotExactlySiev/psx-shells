#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "registers.h"

//#define NO_GTE

#define PREC    12
#define ONE     (1 << PREC)

void k_printf(char* fmt, ...);

static int fixed_div(int a, int b)
{
    return (a * ONE) / b;
}

static int fixed_mul(int a, int b)
{
    return a * b / ONE;
}

typedef unsigned short  ushort;
typedef unsigned int    uint;