#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include "parser.h"

#define BLOCK_SIZE 512

#define OP_PUT      0b11000000
#define OP_CALL     0b11010000
#define OP_CIF      0b11110000

typedef struct Label {
    unsigned long globalIdHash;
    unsigned long localIdHash;
    unsigned long pos;
    struct Label* next;
} Label;

typedef struct Reference {
    unsigned long pos;
    Token* token;
    unsigned long globalIdHash;
    struct Reference* next;
} Reference;

void assemble(Token* firstToken, char** outputPtr, unsigned long* lengthPtr);

#endif