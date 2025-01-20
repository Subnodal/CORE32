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

void inspect(Token* token) {
    if (!token) {
        printf("\n");
        return;
    }

    switch (token->type) {
        case TOK_ERROR:
            printf("(error) ");
            break;

        case TOK_INT:
            printf("int(%d) ", token->value.asInt);
            break;

        default:
            printf("(unknown) ");
            break;;
    }

    inspect(token->next);
}