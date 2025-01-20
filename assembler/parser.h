#ifndef PARSER_H_
#define PARSER_H_

#include <stdbool.h>

typedef enum {
    TOK_ERROR,
    TOK_INT
} TokenType;

typedef enum {
    FMT_BYTE = 0b00,
    FMT_SHORT = 0b01,
    FMT_LONG = 0b10,
    FMT_FLOAT = 0b11
} Format;

typedef struct Token {
    TokenType type;
    union {
        int asInt;
    } value;
    Format format;
    struct Token* next;
} Token;

bool matchChars(char** codePtr, char* match);
bool matchUInt(char** codePtr, int base, unsigned int* result);
Format getFormatSuffix(char** codePtr);

Token* parse(char* code);
void inspect(Token* token);

#endif