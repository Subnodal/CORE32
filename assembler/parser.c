#include "parser.h"

#include <stdlib.h>
#include <stdio.h>

unsigned int matchChars(char* code, char* match) {
    unsigned int i = 0;

    while (1) {
        if (!match[i]) return i;
        if (code[i] != match[i]) return 0;
        i++;
    }
}

unsigned int matchUInt(char* code, int base, unsigned int* result, Format* format) {
    unsigned int i = 0;

    *result = 0;
    *format = 0;

    while (1) {
        if (
            (code[i] == '0' || code[i] == '1') ||
            (base >= 8 && code[i] >= '2' && code[i] <= '7') ||
            (base >= 10 && code[i] >= '8' && code[i] <= '9') ||
            (base >= 16 && code[i] >= 'A' && code[i] <= 'F') ||
            (base >= 16 && code[i] >= 'a' && code[i] <= 'f')
        ) {
            (*result) *= base;
            (*result) += code[i] - '0';
        } else {
            break;
        }

        i++;
    }

    return i;
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
        if (code[0] == ')') commentLevel--;

        if (code[0] == ' ' || code[0] == '\t' || code[0] == '\n' || commentLevel > 0) {
            code++;
            continue;
        }

        if ((length = matchUInt(code, 10, &intResult, &resultFormat))) {
            tokenToAdd.type = TOK_INT;
            tokenToAdd.value.asInt = intResult;
            code += length;
            goto addToken;
        }

        tokenToAdd.type = TOK_ERROR;

        addToken:

        Token* tokenPtr = malloc(sizeof(Token));

        tokenPtr->type = tokenToAdd.type;
        tokenPtr->value = tokenToAdd.value;
        tokenPtr->next = NULL;

        if (!firstToken) firstToken = tokenPtr;
        if (lastToken) lastToken->next = tokenPtr;

        lastToken = tokenPtr;
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