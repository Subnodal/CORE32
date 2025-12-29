#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include "parser.h"

#define C32_BLOCK_SIZE 512

#define C32_OP_DUP      0b00011000
#define C32_OP_NOT      0b01111000
#define C32_OP_PUT      0b11000000
#define C32_OP_JUMP     0b11001000
#define C32_OP_CALL     0b11010000
#define C32_OP_CIF      0b11110000
#define C32_OP_IF       0b11111000

#define C32_REL_ABS     0b000
#define C32_REL_REL     0b100

#define C32_MAX_MACRO_DEPTH 16

typedef enum {
    C32_BINFMT_CODE = 1,
    C32_BINFMT_REFS = 2
} c32_BinaryFormat;

typedef union c32_LongOrFloat {
    unsigned long asLong;
    float asFloat;
} c32_LongOrFloat;

typedef struct c32_Label {
    unsigned long globalIdHash;
    unsigned long localIdHash;
    unsigned long pos;
    unsigned long size;
    struct c32_Label* next;
} c32_Label;

typedef struct c32_Reference {
    unsigned long pos;
    c32_Token* token;
    unsigned long globalIdHash;
    struct c32_Reference* next;
} c32_Reference;

typedef struct c32_GroupLevel {
    unsigned long pos;
    c32_Token* token;
    struct c32_GroupLevel* prev;
    struct c32_GroupLevel* next;
} c32_GroupLevel;

typedef struct c32_Macro {
    unsigned long idHash;
    c32_Token* firstToken;
    struct c32_Macro* next;
} c32_Macro;

typedef struct c32_MacroLevel {
    c32_Token* continuec32_Token;
    c32_GroupLevel* baseGroupLevel;
    struct c32_MacroLevel* prev;
    struct c32_MacroLevel* next;
} c32_MacroLevel;

void c32_addC32Header();
void c32_addC32References();
void c32_assemble(c32_Token* firstToken, char** outputPtr, unsigned long* lengthPtr, c32_BinaryFormat binaryFormat);

#endif