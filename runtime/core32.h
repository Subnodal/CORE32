#ifndef CORE32_H_
#define CORE32_H_

#include <stdlib.h>
#include <stdint.h>

#define C32_MALLOC malloc
#define C32_FREE free

#define C32_MEM_SIZE 0x10000

#define C32_FMT_BYTE 0b00
#define C32_FMT_SHORT 0b01
#define C32_FMT_LONG 0b10
#define C32_FMT_FLOAT 0b11

typedef uint8_t c32_Byte;
typedef uint16_t c32_Short;
typedef uint32_t c32_Long;
typedef float c32_Float;

typedef union c32_LongOrFloat {
    unsigned long asLong;
    float asFloat;
} c32_LongOrFloat;

typedef struct CORE32 {
    c32_Long ip; // Instruction pointer
    c32_Long csp; // Core stack pointer
    c32_Long dsp; // Data stack pointer
    c32_Byte* mem;
} CORE32;

CORE32* c32_new(c32_Byte* code, c32_Long codeLength);
void c32_step(CORE32* vm);

#endif