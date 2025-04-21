#ifndef PARSER_H_
#define PARSER_H_

#include <stdbool.h>

typedef enum {
    TOK_ERROR,
    TOK_OP,
    TOK_INT,
    TOK_DEFINE,
    TOK_CALL,
    TOK_CALL_COND,
    TOK_ADDR,
    TOK_RAW_OPEN,
    TOK_RAW_CLOSE,
    TOK_GROUP_OPEN,
    TOK_GROUP_CLOSE
} TokenType;

typedef enum {
    GROUP_STD = '.',
    GROUP_COND = '?',
    GROUP_QUOTED = ':'
} GroupType;

typedef enum {
    FMT_BYTE = 0b00,
    FMT_SHORT = 0b01,
    FMT_LONG = 0b10,
    FMT_FLOAT = 0b11,
    FMT_GLOBAL = 0b00,
    FMT_LOCAL = 0b01
} Format;

typedef struct Token {
    TokenType type;
    union {
        int asInt;
        unsigned char asOpcode;
        unsigned long asIdHash;
        GroupType asGroupType;
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