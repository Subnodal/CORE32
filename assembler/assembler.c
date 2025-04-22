#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>

const char WIDTHS[4] = {1, 2, 4, 4};

char* output;
unsigned long pos;
unsigned long length;
Label* firstLabel;
Label* lastLabel;
Reference* firstReference;
Reference* lastReference;

void grow() {
    unsigned long oldLength = length;

    if (pos <= length) {
        return;
    }

    length = pos;

    if (oldLength / BLOCK_SIZE < length / BLOCK_SIZE) {
        unsigned long baseLength = (length / BLOCK_SIZE) * BLOCK_SIZE;

        output = realloc(output, baseLength + BLOCK_SIZE);

        for (unsigned int i = 0; i < BLOCK_SIZE; i++) {
            output[baseLength + i] = '\0';
        }
    }
}

void createLabel(unsigned long globalIdHash, unsigned long localIdHash) {
    Label* label = malloc(sizeof(Label));

    label->globalIdHash = globalIdHash;
    label->localIdHash = localIdHash;
    label->pos = pos;
    label->next = NULL;

    fprintf(stderr, "Created label (global: %ld, local: %ld)\n", globalIdHash, localIdHash);

    if (!firstLabel) firstLabel = label;
    if (lastLabel) lastLabel->next = label;

    lastLabel = label;
}

void createReference(Token* token, unsigned long globalIdHash) {
    Reference* reference = malloc(sizeof(Reference));

    reference->pos = pos;
    reference->token = token;
    reference->globalIdHash = globalIdHash;
    reference->next = NULL;

    if (!firstReference) firstReference = reference;
    if (lastReference) lastReference->next = reference;

    lastReference = reference;
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

Label* resolveLabel(unsigned long globalIdHash, unsigned long localIdHash) {
    Label* label = firstLabel;

    while (label) {
        if (label->globalIdHash == globalIdHash && label->localIdHash == localIdHash) {
            return label;
        }

        label = label->next;
    }

    fprintf(stderr, "Cannot resolve label (global: %ld, local: %ld)\n", globalIdHash, localIdHash);

    return NULL;
}

void resolveReferences() {
    Reference* reference = firstReference;

    while (reference) {
        pos = reference->pos;

        Token* token = reference->token;

        switch (token->type) {
            case TOK_CALL: case TOK_CALL_COND: case TOK_ADDR: {
                Label* label = (
                    token->format == FMT_LOCAL ?
                    resolveLabel(reference->globalIdHash, token->value.asIdHash) :
                    resolveLabel(token->value.asIdHash, 0)
                );

                if (label) outputW(label->pos, FMT_LONG);
                break;
            }

            case TOK_ADDR_EXT: {
                if (!token->next || token->next->type != TOK_CALL) {
                    fprintf(stderr, "Invalid subsequent token\n");
                    break;
                }

                Label* label = resolveLabel(token->value.asIdHash, token->next->value.asIdHash);

                if (label) outputW(label->pos, FMT_LONG);
                break;
            }

            default:
                fprintf(stderr, "Reference type not implemented\n");
                break;
        }

        reference = reference->next;
    }
}

void assemble(Token* firstToken, char** outputPtr, unsigned long* lengthPtr) {
    output = malloc(BLOCK_SIZE);
    pos = 0;
    length = 0;
    firstLabel = NULL;
    lastLabel = NULL;
    firstReference = NULL;
    lastReference = NULL;

    Token* token = firstToken;
    unsigned long currentGlobalHashId = 0;

    while (token) {
        switch (token->type) {
            case TOK_ERROR:
                break;

            case TOK_OP:
                outputB(token->value.asOpcode | token->format);
                break;

            case TOK_INT:
                outputB(OP_PUT | token->format);
                outputW(token->value.asInt, token->format);
                break;

            case TOK_DEFINE:
                if (token->format == FMT_LOCAL) createLabel(currentGlobalHashId, token->value.asIdHash);
                else {
                    currentGlobalHashId = token->value.asIdHash;
                    createLabel(currentGlobalHashId, 0);
                }
                break;

            case TOK_CALL: case TOK_CALL_COND: case TOK_ADDR: case TOK_ADDR_EXT:
                outputB(OP_PUT | FMT_LONG);
                createReference(token, currentGlobalHashId);
                outputW(0, FMT_LONG);
                if (token->type == TOK_CALL) outputB(OP_CALL | FMT_LONG);
                if (token->type == TOK_CALL_COND) outputB(OP_CIF | FMT_LONG);
                break;

            case TOK_POS_OFFSET:
                pos += token->value.asInt; grow();
                break;

            default:
                fprintf(stderr, "Token type not implemented\n");
                break;
        }

        token = token->next;
    }

    resolveReferences();

    *outputPtr = realloc(output, length);
    *lengthPtr = length;
}