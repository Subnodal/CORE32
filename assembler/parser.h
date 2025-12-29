#ifndef PARSER_H_
#define PARSER_H_

#include <stdbool.h>

typedef enum {
    C32_TOK_ERROR,
    C32_TOK_OP,
    C32_TOK_INT,
    C32_TOK_STRING,
    C32_TOK_DEFINE,
    C32_TOK_CALL,
    C32_TOK_CALL_COND,
    C32_TOK_ADDR,
    C32_TOK_ADDR_EXT,
    C32_TOK_SIZE_OF,
    C32_TOK_SIZE_OF_EXT,
    C32_TOK_RAW_OPEN,
    C32_TOK_RAW_CLOSE,
    C32_TOK_C32_GROUP_OPEN,
    C32_TOK_C32_GROUP_CLOSE,
    C32_TOK_POS_ABS,
    C32_TOK_POS_OFFSET,
    C32_TOK_SIZE_OF_OFFSET,
    C32_TOK_SIZE_OF_OFFSET_EXT,
    C32_TOK_LOCAL_OFFSET,
    C32_TOK_LOCAL_OFFSET_EXT,
    C32_TOK_MACRO,
    C32_TOK_MACRO_DEFINE
} c32_TokenType;

typedef enum {
    C32_GROUP_STD = '.',
    C32_GROUP_COND = '?',
    C32_GROUP_QUOTED = ':',
    C32_GROUP_SKIPPED = '$'
} GroupType;

typedef enum {
    C32_FMT_BYTE = 0b00,
    C32_FMT_SHORT = 0b01,
    C32_FMT_LONG = 0b10,
    C32_FMT_FLOAT = 0b11,
    C32_FMT_GLOBAL = 0b00,
    C32_FMT_LOCAL = 0b01
} Format;

typedef struct c32_Token {
    c32_TokenType type;
    union {
        unsigned long asInt;
        char* asString;
        unsigned char asOpcode;
        unsigned long asIdHash;
        GroupType asGroupType;
    } value;
    Format format;
    struct c32_Token* next;
} c32_Token;

typedef struct c32_CachedIdentifier {
    unsigned long idHash;
    char* string;
    struct c32_CachedIdentifier* next;
} c32_CachedIdentifier;

extern bool c32_showDebugMessages;

bool c32_matchChars(char** codePtr, char* match, bool whole);
bool c32_matchInList(char** codePtr, char* list[], unsigned int* index, bool whole);
bool c32_matchUInt(char** codePtr, int base, unsigned int* result);
bool c32_matchIdentifier(char** codePtr, unsigned long* result);
char* c32_idHashToString(unsigned long idHash);
Format c32_getFormatSuffix(char** codePtr);

c32_Token* c32_parse(char* code, char* path);
void c32_inspect(c32_Token* token);

#endif