#ifndef CORE32_H_
#define CORE32_H_

#include <stdlib.h>
#include <stdint.h>

#define C32_MALLOC malloc
#define C32_FREE free

#define C32_MEM_SIZE 0x10000

#define C32_OP_DATA     0b00000000
#define C32_OP_DROP     0b00001000
#define C32_OP_NIP      0b00010000
#define C32_OP_DUPE     0b00011000
#define C32_OP_SWAP     0b00100000
#define C32_OP_OVER     0b00101000
#define C32_OP_FROM     0b00110000
#define C32_OP_TO       0b00111000
#define C32_OP_SUB      0b01000000
#define C32_OP_ADD      0b01001000
#define C32_OP_DIV      0b01010000
#define C32_OP_MUL      0b01011000
#define C32_OP_OR       0b01100000
#define C32_OP_XOR      0b01101000
#define C32_OP_AND      0b01110000
#define C32_OP_NOT      0b01111000
#define C32_OP_NEQ      0b10000000
#define C32_OP_EQ       0b10001000
#define C32_OP_GTN      0b10010000
#define C32_OP_BSR      0b10011000
#define C32_OP_DEC      0b10100000
#define C32_OP_LTN      0b10101000
#define C32_OP_BSL      0b10110000
#define C32_OP_INC      0b10111000
#define C32_OP_JUMP     0b11000000
#define C32_OP_CALL     0b11001000
#define C32_OP_RET      0b11010000
#define C32_OP_INT      0b11011000
#define C32_OP_GET      0b11100000
#define C32_OP_SET      0b11101000
#define C32_OP_ELSE     0b11110000
#define C32_OP_IF       0b11111000

#define C32_REL_ABS     0b0
#define C32_REL_REL     0b1

#define C32_FMT_BYTE    0b00
#define C32_FMT_SHORT   0b01
#define C32_FMT_LONG    0b10
#define C32_FMT_FLOAT   0b11

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