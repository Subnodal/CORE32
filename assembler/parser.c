#include "parser.h"

#include <stdlib.h>
#include <stdio.h>

#define MATCH_BASE(prefix, base) if (matchChars(&code, prefix)) { \
    if (!matchUInt(&code, base, &intResult)) goto error; \
        tokenToAdd.type = TOK_INT; \
        tokenToAdd.value.asInt = intResult; \
        tokenToAdd.format = getFormatSuffix(&code); \
        goto addToken; \
    }

char* opNames[] = {
//  *000    *001    *010    *101    *110    *101    *110    *111
    "ret",  "drop", "mod",  "dup",  "swap", "over", "from", "to",   // 00*
    "sub",  "add",  "div",  "mul",  "or",   "xor",  "and",  "not",  // 01*
    "neq",  "eq",   "gtn",  "bsr",  "dec",  "ltn",  "bsl",  "inc",  // 10*
    "data", "jump", "call", "int",  "get",  "set",  "cif",  "if",   // 11*
    NULL
};

char* opSymbols[] = {
//  *000    *001    *010    *101    *110    *101    *110    *111
    "",     "",     "%",    "",     "",     "",     "",     "",     // 00*
    "-",    "+",    "/",    "*",    "|",    "^",    "&",    "!",    // 01*
    "!=",   "=",    ">",    ">>"    "--",   "<",    "<<",   "++",   // 10*
    "",     "",     "",     "",     "",     "",     "",     "",     // 11*
    NULL
};

bool matchChars(char** codePtr, char* match) {
    unsigned int i = 0;

    while (1) {
        if (!match[i]) {
            (*codePtr) += i;
            return true;
        }

        if ((*codePtr)[i] != match[i]) return false;

        i++;
    }
}

bool matchInList(char** codePtr, char* list[], unsigned int* index) {
    unsigned int i = 0;

    while (list[i]) {
        if (!list[i][0]) {
            // Ignore empty entries
            i++;
            continue;
        }

        if (matchChars(codePtr, list[i])) {
            *index = i;
            return true;
        }

        i++;
    }

    return false;
}

bool matchUInt(char** codePtr, int base, unsigned int* result) {
    unsigned int i = 0;
    char* code = *codePtr;

    *result = 0;

    while (1) {
        if (
            (code[i] == '0' || code[i] == '1') ||
            (base >= 8 && code[i] >= '2' && code[i] <= '7') ||
            (base >= 10 && code[i] >= '8' && code[i] <= '9')
        ) {
            (*result) *= base;
            (*result) += code[i] - '0';
        } else if (base >= 16 && code[i] >= 'A' && code[i] <= 'F') {
            (*result) *= base;
            (*result) += code[i] - 'A' + 0xA;
        } else if (base >= 16 && code[i] >= 'a' && code[i] <= 'f') {
            (*result) *= base;
            (*result) += code[i] - 'a' + 0xA;
        } else {
            break;
        }

        i++;
    }

    (*codePtr) += i;
    return i > 0;
}

Format getFormatSuffix(char** code) {
    if ((*code)[0] == '\'') {
        (*code)++;
        return FMT_BYTE;
    }

    if ((*code)[0] == '\"') {
        (*code)++;
        return FMT_SHORT;
    }

    if ((*code)[0] == '%') {
        (*code)++;
        return FMT_FLOAT;
    }

    return FMT_LONG;
}

Token* parse(char* code) {
    Token* firstToken = NULL;
    Token* lastToken = NULL;
    Token tokenToAdd;
    unsigned int length;
    int commentLevel = 0;
    unsigned int intResult = 0;
    Format resultFormat = FMT_BYTE;

    while (code[0]) {
        if (code[0] == '(') commentLevel++;

        if (code[0] == ')' && commentLevel > 0) {
            commentLevel--;
            code++;
            continue;
        }

        if (code[0] == ' ' || code[0] == '\t' || code[0] == '\n' || commentLevel > 0) {
            code++;
            continue;
        }

        if (code[0] == '[' || code[0] == ']') {
            tokenToAdd.type = code[0] == ']' ? TOK_RAW_CLOSE : TOK_RAW_OPEN;
            code++;

            goto addToken;
        }

        if (code[0] == '{' || code[0] == '}') {
            tokenToAdd.type = code[0] == '}' ? TOK_GROUP_CLOSE : TOK_GROUP_OPEN;
            tokenToAdd.value.asGroupType = GROUP_STD;
            code++;

            goto addToken;
        }

        if (matchChars(&code, "?{")) {
            tokenToAdd.type = TOK_GROUP_OPEN;
            tokenToAdd.value.asGroupType = GROUP_COND;

            goto addToken;
        }

        if (matchChars(&code, ":{")) {
            tokenToAdd.type = TOK_GROUP_OPEN;
            tokenToAdd.value.asGroupType = GROUP_QUOTED;

            goto addToken;
        }

        if (matchInList(&code, opNames, &intResult) || matchInList(&code, opSymbols, &intResult)) {
            tokenToAdd.type = TOK_OP;
            tokenToAdd.value.asOpcode = intResult << 3;
            tokenToAdd.format = getFormatSuffix(&code);

            goto addToken;
        }

        MATCH_BASE("0b", 2);
        MATCH_BASE("0o", 8);
        MATCH_BASE("0x", 16);

        if (matchUInt(&code, 10, &intResult)) {
            tokenToAdd.type = TOK_INT;
            tokenToAdd.value.asInt = intResult;
            tokenToAdd.format = getFormatSuffix(&code);

            goto addToken;
        }

        error:

        tokenToAdd.type = TOK_ERROR;

        addToken:

        Token* tokenPtr = malloc(sizeof(Token));

        tokenPtr->type = tokenToAdd.type;
        tokenPtr->value = tokenToAdd.value;
        tokenPtr->format = tokenToAdd.format;
        tokenPtr->next = NULL;

        if (!firstToken) firstToken = tokenPtr;
        if (lastToken) lastToken->next = tokenPtr;

        lastToken = tokenPtr;

        if (tokenToAdd.type == TOK_ERROR) {
            break;
        }
    }

    return firstToken;
}

const char* inspectFormat(Token* token) {
    if (token->format == FMT_BYTE) return "'";
    if (token->format == FMT_SHORT) return "\"";
    if (token->format == FMT_FLOAT) return "%";
    return "";
}

void inspect(Token* token) {
    if (!token) {
        printf("\n");
        return;
    }

    switch (token->type) {
        case TOK_ERROR:
            printf("(error) ");
            break;

        case TOK_OP:
            printf("op(%s%s) ", opNames[token->value.asOpcode >> 3], inspectFormat(token));
            break;

        case TOK_INT:
            printf("int(%d%s) ", token->value.asInt, inspectFormat(token));
            break;

        case TOK_RAW_OPEN:
            printf("raw ");
            break;

        case TOK_RAW_CLOSE:
            printf("endraw ");
            break;

        case TOK_GROUP_OPEN:
            printf("group(%c) ", token->value.asGroupType);
            break;

        case TOK_GROUP_CLOSE:
            printf("endgroup ");
            break;

        default:
            printf("(unknown) ");
            break;
    }

    inspect(token->next);
}