#include <stdio.h>
#include <stdlib.h>

#include "assembler.h"

const char WIDTHS[4] = {1, 2, 4, 4};

char* output;
unsigned long pos;
unsigned long length;
Label* firstLabel;
Label* lastLabel;
Reference* firstReference;
Reference* lastReference;
unsigned int referenceCount;
GroupLevel* firstGroupLevel;
GroupLevel* lastGroupLevel;
Macro* firstMacro;
Macro* lastMacro;
MacroLevel* firstMacroLevel;
MacroLevel* lastMacroLevel;
unsigned int macroLevelCount;

void grow() {
    unsigned long oldLength = length;

    if (pos <= length) {
        return;
    }

    length = pos;

    if (oldLength / BLOCK_SIZE < length / BLOCK_SIZE) {
        unsigned long baseLength = (length / BLOCK_SIZE) * BLOCK_SIZE;

        printf("Growing from 0x%08x\n", baseLength);

        output = realloc(output, baseLength + BLOCK_SIZE);

        for (unsigned int i = oldLength + 1; i < baseLength + BLOCK_SIZE; i++) {
            output[i] = '\0';
        }
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

void outputChars(char* chars, unsigned int length) {
    for (unsigned int i = 0; i < length; i++) {
        outputB(chars[i]);
    }
}

Format getShortestFormat(unsigned long value) {
    Format format = FMT_LONG;

    if (value <= 0xFFFF) format = FMT_SHORT;
    if (value <= 0xFF) format = FMT_BYTE;

    return format;
}

char getFormatLength(Format format) {
    switch (format) {
        case FMT_BYTE: return 1;
        case FMT_SHORT: return 2;
        case FMT_LONG: case FMT_FLOAT: return 4;
        default: return 0;
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
    referenceCount++;
}

void pushGroupLevel(Token* token) {
    GroupLevel* groupLevel = malloc(sizeof(GroupLevel));

    printf("Open group %c\n", token->value.asGroupType);
    
    groupLevel->pos = pos;
    groupLevel->token = token;
    groupLevel->prev = lastGroupLevel;
    groupLevel->next = NULL;

    if (!firstGroupLevel) firstGroupLevel = groupLevel;
    if (lastGroupLevel) lastGroupLevel->next = groupLevel;

    lastGroupLevel = groupLevel;
}

bool popGroupLevel(GroupLevel* groupLevel) {
    if (!lastGroupLevel) {
        return false;
    }

    *groupLevel = *lastGroupLevel;

    lastGroupLevel = lastGroupLevel->prev;

    if (lastGroupLevel) {
        lastGroupLevel->next = NULL;
    } else {
        firstGroupLevel = NULL;
    }

    return true;
}

void createMacro(unsigned long idHash, Token* firstToken) {
    Macro* macro = malloc(sizeof(Macro));

    macro->idHash = idHash;
    macro->firstToken = firstToken;
    macro->next = NULL;

    if (!firstMacro) firstMacro = macro;
    if (lastMacro) lastMacro->next = macro;

    lastMacro = macro;
}

void pushMacroLevel(Token* continueToken) {
    MacroLevel* macroLevel = malloc(sizeof(MacroLevel));
    
    macroLevel->continueToken = continueToken;
    macroLevel->baseGroupLevel = lastGroupLevel;
    macroLevel->prev = lastMacroLevel;
    macroLevel->next = NULL;

    if (!firstMacroLevel) firstMacroLevel = macroLevel;
    if (lastMacroLevel) lastMacroLevel->next = macroLevel;

    lastMacroLevel = macroLevel;
    macroLevelCount++;
}

bool popMacroLevel(MacroLevel* macroLevel) {
    if (!lastMacroLevel) {
        return false;
    }

    *macroLevel = *lastMacroLevel;

    lastMacroLevel = lastMacroLevel->prev;
    macroLevelCount--;

    if (lastMacroLevel) {
        lastMacroLevel->next = NULL;
    } else {
        firstMacroLevel = NULL;
    }

    return true;
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

void setLabelSize(unsigned long globalIdHash, unsigned long localIdHash, unsigned long size) {
    Label* label = resolveLabel(globalIdHash, localIdHash);

    if (label) label->size = size;
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

                if (!label) goto undefinedReference;

                outputW(label->pos, FMT_LONG);
                break;
            }

            case TOK_LOCAL_OFFSET: {
                Label* label = resolveLabel(reference->globalIdHash, token->value.asIdHash);
                Label* globalLabel = resolveLabel(reference->globalIdHash, 0);

                if (!label || !globalLabel) goto undefinedReference;

                outputW(label->pos - globalLabel->pos, FMT_LONG);
            }

            case TOK_ADDR_EXT: case TOK_LOCAL_OFFSET_EXT: {
                if (!token->next || token->next->type != TOK_CALL) {
                    fprintf(stderr, "Invalid subsequent token\n");
                    break;
                }

                Label* label = resolveLabel(token->value.asIdHash, token->next->value.asIdHash);

                if (!label) goto undefinedReference;

                unsigned long resolvedPos = label->pos;

                if (token->type == TOK_LOCAL_OFFSET_EXT) {
                    Label* globalLabel = resolveLabel(token->value.asIdHash, 0);

                    if (!globalLabel) goto undefinedReference;

                    resolvedPos -= globalLabel->pos;
                }

                outputW(resolvedPos, FMT_LONG);
                break;
            }

            case TOK_SIZE_OF: case TOK_SIZE_OF_EXT: {
                Label* label = NULL;

                if (token->format == FMT_LOCAL) {
                    label = resolveLabel(reference->globalIdHash, token->value.asIdHash);
                } else if (token->type == TOK_SIZE_OF_EXT) {
                    if (!token->next || token->next->type != TOK_CALL) {
                        fprintf(stderr, "Invalid subsequent token\n");
                        break;
                    }

                    label = resolveLabel(token->value.asIdHash, token->next->value.asIdHash);
                } else {
                    label = resolveLabel(token->value.asIdHash, 0);
                }

                if (!label) goto undefinedReference;

                outputW(label->size, FMT_LONG);
                break;
            }

            default:
                fprintf(stderr, "Reference type not implemented\n");
                break;

            undefinedReference:
                fprintf(stderr, "Undefined reference\n");
                break;
        }

        reference = reference->next;
    }
}

void resolveGroupLevel() {
    GroupLevel groupLevel;

    if (!popGroupLevel(&groupLevel)) {
        fprintf(stderr, "Mismatched group bracket\n");
        return;
    }

    GroupType groupType = groupLevel.token->value.asGroupType;

    if (groupType == GROUP_STD) {
        return;
    }

    unsigned long oldPos = pos;

    pos = groupLevel.pos + 1; // Offset by one byte due to `put` op

    if (groupType == GROUP_COND) {
        pos++; // Offset by another byte due to `not` op
    }

    printf("Resolve group close to address 0x%08x\n", pos);

    if (groupType == GROUP_QUOTED) {
        // Offset pos by 11 to skip over `put` + long + `put` + long + `jump`
        outputW(groupLevel.pos + 11, FMT_LONG);
        pos++; // Skip over second `put` op
    }

    outputW(oldPos, FMT_LONG);

    pos = oldPos;
}

Macro* reoslveMacro(unsigned long idHash) {
    Macro* macro = firstMacro;

    while (macro) {
        if (macro->idHash == idHash) return macro;

        macro = macro->next;
    }

    return NULL;
}

void addC32Header() {
    outputB(0);
    outputChars("C32", 3);
    outputW(0, FMT_LONG);

    outputChars("CODE", 4);
    outputW(0, FMT_LONG); // Code length will be stored here
}

void addC32References() {
    unsigned long savedPos = pos;

    pos = 0x400 + 12;
    outputW(savedPos - pos, FMT_LONG); // Store code length

    pos = savedPos;

    outputChars("REFS", 4);
    outputW(referenceCount, FMT_LONG);

    Reference* currentReference = firstReference;

    while (currentReference) {
        outputW(currentReference->pos, FMT_LONG);

        currentReference = currentReference-> next;
    }
}

void assemble(Token* firstToken, char** outputPtr, unsigned long* lengthPtr, bool useC32Format) {
    output = malloc(BLOCK_SIZE);
    pos = 0x400;
    length = 0;
    firstLabel = NULL;
    lastLabel = NULL;
    firstReference = NULL;
    lastReference = NULL;
    firstGroupLevel = NULL;
    lastGroupLevel = NULL;
    firstMacro = NULL;
    lastMacro = NULL;
    firstMacroLevel = NULL;
    lastMacroLevel = NULL;
    macroLevelCount = 0;
    referenceCount = 0;

    grow();

    if (useC32Format) {
        addC32Header();
    }

    Token* token = firstToken;
    unsigned long currentGlobalHashId = 0;
    unsigned long currentLocalHashId = 0;
    unsigned long currentGlobalStartPos = 0;
    unsigned long currentLocalStartPos = 0;
    unsigned int rawLevel = 0;
    MacroLevel macroLevel;

    while (token) {
        bool skipNextToken = false;
        bool skipMacroExit = false;
    
        switch (token->type) {
            case TOK_ERROR:
                break;

            case TOK_OP:
                outputB(token->value.asOpcode | token->format);
                break;

            case TOK_INT:
                if (rawLevel == 0) outputB(OP_PUT | token->format);
                outputW(token->value.asInt, token->format);
                break;

            case TOK_STRING: {
                unsigned int length = 0;
                for (; token->value.asString[length]; length++);
                if (rawLevel == 0) {
                    Format format = getShortestFormat(length + 1);
                    outputB(OP_PUT | FMT_LONG);
                    outputW(pos + 6 + getFormatLength(format), FMT_LONG);
                    outputB(OP_PUT | format);
                    outputW(length + 2, format);
                    outputB(OP_JUMP | format | REL_REL);
                }
                for (unsigned int i = 0; token->value.asString[i]; i++) {
                    outputB(token->value.asString[i]);
                }
                if (rawLevel == 0) outputB('\0');
                break;
            }

            case TOK_DEFINE:
                if (token->format == FMT_LOCAL) {
                    setLabelSize(currentGlobalHashId, currentLocalHashId, pos - currentLocalStartPos);
                    currentLocalHashId = token->value.asIdHash;
                    currentLocalStartPos = pos;
                    createLabel(currentGlobalHashId, currentLocalHashId);
                } else {
                    if (currentGlobalHashId) setLabelSize(currentGlobalHashId, 0, pos - currentGlobalStartPos);
                    if (currentLocalHashId) setLabelSize(currentGlobalHashId, currentLocalHashId, pos - currentLocalStartPos);
                    currentGlobalHashId = token->value.asIdHash;
                    currentGlobalStartPos = pos;
                    createLabel(currentGlobalHashId, 0);
                }
                break;

            case TOK_CALL: case TOK_CALL_COND: case TOK_ADDR: case TOK_ADDR_EXT:
            case TOK_LOCAL_OFFSET: case TOK_LOCAL_OFFSET_EXT:
            case TOK_SIZE_OF: case TOK_SIZE_OF_EXT:
                if (rawLevel == 0) outputB(OP_PUT | FMT_LONG);
                createReference(token, currentGlobalHashId);
                outputW(0, FMT_LONG);
                if (rawLevel == 0 && token->type == TOK_CALL) outputB(OP_CALL | FMT_LONG);
                if (rawLevel == 0 && token->type == TOK_CALL_COND) outputB(OP_CIF | FMT_LONG);
                if (
                    token->type == TOK_ADDR_EXT ||
                    token->type == TOK_LOCAL_OFFSET_EXT ||
                    token->type == TOK_SIZE_OF_EXT
                ) skipNextToken = true;
                break;

            case TOK_RAW_OPEN:
                rawLevel++;
                break;

            case TOK_RAW_CLOSE:
                if (rawLevel == 0) {
                    fprintf(stderr, "Mismatched raw bracket\n");
                    break;
                }
                rawLevel--;
                break;

            case TOK_GROUP_OPEN:
                pushGroupLevel(token);
                if (token->value.asGroupType == GROUP_STD) break;
                if (token->value.asGroupType == GROUP_COND) outputB(OP_NOT);
                outputB(OP_PUT | FMT_LONG);
                outputW(0, FMT_LONG);
                if (token->value.asGroupType == GROUP_QUOTED) {
                    outputB(OP_PUT | FMT_LONG);
                    outputW(0, FMT_LONG);
                }
                outputB((token->value.asGroupType == GROUP_COND ? OP_IF : OP_JUMP) | FMT_LONG);
                break;

            case TOK_GROUP_CLOSE:
                resolveGroupLevel();
                break;

            case TOK_POS_ABS:
                pos = token->value.asInt; grow();
                break;

            case TOK_POS_OFFSET:
                pos += token->value.asInt; grow();
                break;

            case TOK_SIZE_OF_OFFSET: case TOK_SIZE_OF_OFFSET_EXT: {
                Label* label = NULL;

                if (token->format == FMT_LOCAL) {
                    label = resolveLabel(currentGlobalHashId, token->value.asIdHash);
                } else if (token->type == TOK_SIZE_OF_EXT) {
                    if (!token->next || token->next->type != TOK_CALL) {
                        fprintf(stderr, "Invalid subsequent token\n");
                        break;
                    }

                    label = resolveLabel(token->value.asIdHash, token->next->value.asIdHash);
                    skipNextToken = true;
                } else {
                    label = resolveLabel(token->value.asIdHash, 0);
                }

                if (!label) {
                    fprintf(stderr, "Undefined reference (note: future references not allowed)\n");
                    break;
                }

                pos += label->size; grow();
                break;
            }

            case TOK_MACRO: {
                Macro* macro = reoslveMacro(token->value.asIdHash);

                if (!macro) {
                    fprintf(stderr, "Undefined macro\n");
                    break;
                }

                if (macroLevelCount >= MAX_MACRO_DEPTH - 1) {
                    fprintf(stderr, "Maximum macro depth limit reached\n");
                    break;
                }

                pushMacroLevel(token->next);

                token = macro->firstToken;
                skipMacroExit = true;
                break;
            }

            case TOK_MACRO_DEFINE: {
                unsigned int groupLevel = 0;

                createMacro(token->value.asIdHash, token);

                token = token->next;

                while (token) {
                    if (token->type == TOK_GROUP_OPEN) groupLevel++;
                    if (token->type == TOK_GROUP_CLOSE && groupLevel > 0) groupLevel--;

                    if (groupLevel == 0) {
                        break;
                    }

                    token = token->next;
                }

                break;
            }

            default:
                fprintf(stderr, "Token type not implemented\n");
                break;
        }

        if (token) {
            token = token->next;
        }

        if (skipNextToken && token) {
            token = token->next;
        }

        if (!skipMacroExit && lastMacroLevel && lastMacroLevel->baseGroupLevel == lastGroupLevel && popMacroLevel(&macroLevel)) {
            token = macroLevel.continueToken;
        }
    }

    setLabelSize(currentGlobalHashId, 0, pos - currentGlobalStartPos);
    setLabelSize(currentGlobalHashId, currentLocalHashId, pos - currentLocalStartPos);

    resolveReferences();

    if (useC32Format) {
        addC32References();
    }

    *outputPtr = realloc(output, length);
    *lengthPtr = length;
}