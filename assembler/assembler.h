#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include "parser.h"

#define BLOCK_SIZE 512

#define OP_DUP      0b00011000
#define OP_NOT      0b01111000
#define OP_PUT      0b11000000
#define OP_JUMP     0b11001000
#define OP_CALL     0b11010000
#define OP_CIF      0b11110000
#define OP_IF       0b11111000

#define REL_ABS     0b000
#define REL_REL     0b100

#define MAX_MACRO_DEPTH 16

typedef struct Label {
    unsigned long globalIdHash;
    unsigned long localIdHash;
    unsigned long pos;
    unsigned long size;
    struct Label* next;
} Label;

typedef struct Reference {
    unsigned long pos;
    Token* token;
    unsigned long globalIdHash;
    struct Reference* next;
} Reference;

typedef struct GroupLevel {
    unsigned long pos;
    Token* token;
    struct GroupLevel* prev;
    struct GroupLevel* next;
} GroupLevel;

typedef struct Macro {
    unsigned long idHash;
    Token* firstToken;
    struct Macro* next;
} Macro;

typedef struct MacroLevel {
    Token* continueToken;
    GroupLevel* baseGroupLevel;
    struct MacroLevel* prev;
    struct MacroLevel* next;
} MacroLevel;

void addC32Header();
void addC32References();
void assemble(Token* firstToken, char** outputPtr, unsigned long* lengthPtr, bool useC32Format);

#endif