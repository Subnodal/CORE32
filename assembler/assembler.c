#include "assembler.h"

#include <stdlib.h>

const char WIDTHS[4] = {1, 2, 4, 4};

char* output;
unsigned long pos;
unsigned long length;

void grow() {
    unsigned long oldLength = length;

    if (pos <= length) {
        return;
    }

    length = pos;

    if (oldLength / BLOCK_SIZE < length / BLOCK_SIZE) {
        output = realloc(output, ((length / BLOCK_SIZE) + 1) * BLOCK_SIZE);
    }
}

void outputB(char byte) {
    output[pos++] = byte; grow();
}

void outputW(unsigned long value, Format format) {
    char width = WIDTHS[format];

    outputB(value & 0x000000FF);
    if (width >= 2) outputB((value & 0x0000FF00) >> 8);
    if (width >= 3) outputB((value & 0x00FF0000) >> 16);
    if (width >= 4) outputB((value & 0xFF000000) >> 24);
}

void assemble(Token* firstToken, char** outputPtr, unsigned long* lengthPtr) {
    output = malloc(BLOCK_SIZE);
    pos = 0;
    length = 0;

    Token* token = firstToken;

    while (token) {
        switch (token->type) {
            case TOK_ERROR:
                break;

            case TOK_OP:
                outputB(token->value.asOpcode);
                break;

            case TOK_INT:
                outputB(OP_PUT | token->format);
                outputW(token->value.asInt, token->format);
                break;
        }

        token = token->next;
    }

    *outputPtr = realloc(output, length);
    *lengthPtr = length;
}