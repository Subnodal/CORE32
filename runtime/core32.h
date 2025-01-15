#ifndef CORE32_H_
#define CORE32_H_

#include <stdlib.h>

#define C32_MALLOC malloc
#define C32_FREE free

#define C32_MEM_SIZE 0x10000

typedef unsigned char c32_Byte;
typedef unsigned short c32_Short;
typedef unsigned long c32_Long;
typedef float c32_Float;

typedef struct CORE32 {
    c32_Long ip; // Instruction pointer
    c32_Long csp; // Core stack pointer
    c32_Long dsp; // Data stack pointer
    c32_Byte* mem;
} CORE32;

CORE32* c32_new(c32_Byte* code, c32_Long codeLength);
void c32_step(CORE32* vm);

#endif