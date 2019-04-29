#ifndef _bitwise_definitions_h_
#define _bitwise_definitions_h_

#include <sys/types.h>
#include <stdint.h>

uint64_t Bit[64];

void initBits()
{
    int i;
    uint64_t val = 0x01;
    for (i = 0; i < 64; i++)
    {
        Bit[i] = val;
        val = val << 1;
    }
}

#endif