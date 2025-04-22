#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include "parser.h"

#define BLOCK_SIZE 512

#define OP_PUT      0b11000000

void assemble(Token* firstToken, char** outputPtr, unsigned long* lengthPtr);

#endif