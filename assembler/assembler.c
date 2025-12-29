#include "config.h"
#include "assembler.h"

const char WIDTHS[4] = {1, 2, 4, 4};

char* output;
unsigned long pos;
unsigned long length;
c32_Label* firstLabel;
c32_Label* lastLabel;
c32_Reference* firstReference;
c32_Reference* lastReference;
unsigned int referenceCount;
c32_GroupLevel* firstGroupLevel;
c32_GroupLevel* lastGroupLevel;
c32_Macro* firstMacro;
c32_Macro* lastMacro;
c32_MacroLevel* firstMacroLevel;
c32_MacroLevel* lastMacroLevel;
unsigned int macroLevelCount;

void grow() {
    unsigned long oldLength = length;

    if (pos <= length) {
        return;
    }

    length = pos;

    if (oldLength / C32_BLOCK_SIZE < length / C32_BLOCK_SIZE) {
        unsigned long baseLength = (length / C32_BLOCK_SIZE) * C32_BLOCK_SIZE;

        if (c32_showDebugMessages) C32_ASM_PRINTF("Growing from 0x%08x\n", baseLength);

        output = (char*)C32_ASM_REALLOC(output, baseLength + C32_BLOCK_SIZE);

        for (unsigned int i = oldLength + 1; i < baseLength + C32_BLOCK_SIZE; i++) {
            output[i] = '\0';
        }
    }
}

void outputB(char byte) {
    output[pos++] = byte; grow();
}

void outputW(unsigned long value, Format format) {
    char width = WIDTHS[format];

    if (format == C32_FMT_FLOAT) value = ((c32_LongOrFloat) {.asFloat = value}).asLong;

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
    Format format = C32_FMT_LONG;

    if (value <= 0xFFFF) format = C32_FMT_SHORT;
    if (value <= 0xFF) format = C32_FMT_BYTE;

    return format;
}

char getFormatLength(Format format) {
    switch (format) {
        case C32_FMT_BYTE: return 1;
        case C32_FMT_SHORT: return 2;
        case C32_FMT_LONG: case C32_FMT_FLOAT: return 4;
        default: return 0;
    }
}

void createLabel(unsigned long globalIdHash, unsigned long localIdHash) {
    c32_Label* label = (c32_Label*)C32_ASM_MALLOC(sizeof(c32_Label));

    label->globalIdHash = globalIdHash;
    label->localIdHash = localIdHash;
    label->pos = pos;
    label->next = C32_ASM_NULL;

    if (c32_showDebugMessages) {
        if (localIdHash) {
            C32_ASM_PRINTF_STDERR("Created label: %s.%s\n", c32_idHashToString(globalIdHash), c32_idHashToString(localIdHash));
        } else {
            C32_ASM_PRINTF_STDERR("Created label: %s\n", c32_idHashToString(globalIdHash));
        }
    }

    if (!firstLabel) firstLabel = label;
    if (lastLabel) lastLabel->next = label;

    lastLabel = label;
}

void createReference(c32_Token* token, unsigned long globalIdHash) {
    c32_Reference* reference = (c32_Reference*)C32_ASM_MALLOC(sizeof(c32_Reference));

    reference->pos = pos;
    reference->token = token;
    reference->globalIdHash = globalIdHash;
    reference->next = C32_ASM_NULL;

    if (!firstReference) firstReference = reference;
    if (lastReference) lastReference->next = reference;

    lastReference = reference;
    referenceCount++;
}

void pushGroupLevel(c32_Token* token) {
    c32_GroupLevel* groupLevel = (c32_GroupLevel*)C32_ASM_MALLOC(sizeof(c32_GroupLevel));

    if (c32_showDebugMessages) C32_ASM_PRINTF("Open group %c\n", token->value.asGroupType);
    
    groupLevel->pos = pos;
    groupLevel->token = token;
    groupLevel->prev = lastGroupLevel;
    groupLevel->next = C32_ASM_NULL;

    if (!firstGroupLevel) firstGroupLevel = groupLevel;
    if (lastGroupLevel) lastGroupLevel->next = groupLevel;

    lastGroupLevel = groupLevel;
}

bool popGroupLevel(c32_GroupLevel* groupLevel) {
    if (!lastGroupLevel) {
        return false;
    }

    *groupLevel = *lastGroupLevel;

    lastGroupLevel = lastGroupLevel->prev;

    if (lastGroupLevel) {
        lastGroupLevel->next = C32_ASM_NULL;
    } else {
        firstGroupLevel = C32_ASM_NULL;
    }

    return true;
}

void createMacro(unsigned long idHash, c32_Token* firstToken) {
    c32_Macro* macro = (c32_Macro*)C32_ASM_MALLOC(sizeof(c32_Macro));

    macro->idHash = idHash;
    macro->firstToken = firstToken;
    macro->next = C32_ASM_NULL;

    if (!firstMacro) firstMacro = macro;
    if (lastMacro) lastMacro->next = macro;

    lastMacro = macro;
}

void pushMacroLevel(c32_Token* continuec32_Token) {
    c32_MacroLevel* macroLevel = (c32_MacroLevel*)C32_ASM_MALLOC(sizeof(c32_MacroLevel));
    
    macroLevel->continuec32_Token = continuec32_Token;
    macroLevel->baseGroupLevel = lastGroupLevel;
    macroLevel->prev = lastMacroLevel;
    macroLevel->next = C32_ASM_NULL;

    if (!firstMacroLevel) firstMacroLevel = macroLevel;
    if (lastMacroLevel) lastMacroLevel->next = macroLevel;

    lastMacroLevel = macroLevel;
    macroLevelCount++;
}

bool popMacroLevel(c32_MacroLevel* macroLevel) {
    if (!lastMacroLevel) {
        return false;
    }

    *macroLevel = *lastMacroLevel;

    lastMacroLevel = lastMacroLevel->prev;
    macroLevelCount--;

    if (lastMacroLevel) {
        lastMacroLevel->next = C32_ASM_NULL;
    } else {
        firstMacroLevel = C32_ASM_NULL;
    }

    return true;
}

c32_Label* resolveLabel(unsigned long globalIdHash, unsigned long localIdHash) {
    c32_Label* label = firstLabel;

    while (label) {
        if (label->globalIdHash == globalIdHash && label->localIdHash == localIdHash) {
            return label;
        }

        label = label->next;
    }

    if (localIdHash) {
        C32_ASM_PRINTF_STDERR("Cannot resolve label: %s.%s\n", c32_idHashToString(globalIdHash), c32_idHashToString(localIdHash));
    } else {
        C32_ASM_PRINTF_STDERR("Cannot resolve label: %s\n", c32_idHashToString(globalIdHash));
    }

    return C32_ASM_NULL;
}

void setLabelSize(unsigned long globalIdHash, unsigned long localIdHash, unsigned long size) {
    c32_Label* label = resolveLabel(globalIdHash, localIdHash);

    if (label) label->size = size;
}

void resolveReferences() {
    c32_Reference* reference = firstReference;

    while (reference) {
        pos = reference->pos;

        c32_Token* token = reference->token;

        switch (token->type) {
            case C32_TOK_CALL: case C32_TOK_CALL_COND: case C32_TOK_ADDR: {
                c32_Label* label = (
                    token->format == C32_FMT_LOCAL ?
                    resolveLabel(reference->globalIdHash, token->value.asIdHash) :
                    resolveLabel(token->value.asIdHash, 0)
                );

                if (!label) goto undefinedReference;

                outputW(label->pos, C32_FMT_LONG);
                break;
            }

            case C32_TOK_LOCAL_OFFSET: {
                c32_Label* label = resolveLabel(reference->globalIdHash, token->value.asIdHash);
                c32_Label* globalLabel = resolveLabel(reference->globalIdHash, 0);

                if (!label || !globalLabel) goto undefinedReference;

                outputW(label->pos - globalLabel->pos, C32_FMT_LONG);
            }

            case C32_TOK_ADDR_EXT: case C32_TOK_LOCAL_OFFSET_EXT: {
                if (!token->next || token->next->type != C32_TOK_CALL) {
                    C32_ASM_PRINTF_STDERR("Invalid subsequent token\n");
                    break;
                }

                c32_Label* label = resolveLabel(token->value.asIdHash, token->next->value.asIdHash);

                if (!label) goto undefinedReference;

                unsigned long resolvedPos = label->pos;

                if (token->type == C32_TOK_LOCAL_OFFSET_EXT) {
                    c32_Label* globalLabel = resolveLabel(token->value.asIdHash, 0);

                    if (!globalLabel) goto undefinedReference;

                    resolvedPos -= globalLabel->pos;
                }

                outputW(resolvedPos, C32_FMT_LONG);
                break;
            }

            case C32_TOK_SIZE_OF: case C32_TOK_SIZE_OF_EXT: {
                c32_Label* label = C32_ASM_NULL;

                if (token->format == C32_FMT_LOCAL) {
                    label = resolveLabel(reference->globalIdHash, token->value.asIdHash);
                } else if (token->type == C32_TOK_SIZE_OF_EXT) {
                    if (!token->next || token->next->type != C32_TOK_CALL) {
                        C32_ASM_PRINTF_STDERR("Invalid subsequent token\n");
                        break;
                    }

                    label = resolveLabel(token->value.asIdHash, token->next->value.asIdHash);
                } else {
                    label = resolveLabel(token->value.asIdHash, 0);
                }

                if (!label) goto undefinedReference;

                outputW(label->size, C32_FMT_LONG);
                break;
            }

            default:
                C32_ASM_PRINTF_STDERR("Reference type not implemented\n");
                break;

            undefinedReference:
                C32_ASM_PRINTF_STDERR("Undefined reference\n");
                break;
        }

        reference = reference->next;
    }
}

void resolveGroupLevel() {
    c32_GroupLevel groupLevel;

    if (!popGroupLevel(&groupLevel)) {
        C32_ASM_PRINTF_STDERR("Mismatched group bracket\n");
        return;
    }

    GroupType groupType = groupLevel.token->value.asGroupType;

    if (groupType == C32_GROUP_STD) {
        return;
    }

    unsigned long oldPos = pos;

    pos = groupLevel.pos + 1; // Offset by one byte due to `put` op

    if (groupType == C32_GROUP_COND) {
        pos++; // Offset by another byte due to `not` op
    }

    if (c32_showDebugMessages) C32_ASM_PRINTF("Resolve group close to address 0x%08x\n", pos);

    if (groupType == C32_GROUP_QUOTED) {
        // Offset pos by 11 to skip over `put` + long + `put` + long + `jump`
        outputW(groupLevel.pos + 11, C32_FMT_LONG);
        pos++; // Skip over second `put` op
    }

    outputW(oldPos, C32_FMT_LONG);

    pos = oldPos;
}

c32_Macro* reoslveMacro(unsigned long idHash) {
    c32_Macro* macro = firstMacro;

    while (macro) {
        if (macro->idHash == idHash) return macro;

        macro = macro->next;
    }

    return C32_ASM_NULL;
}

void c32_addC32Header() {
    outputB(0);
    outputChars("C32", 3);
    outputW(0, C32_FMT_LONG);

    outputChars("CODE", 4);
    outputW(0, C32_FMT_LONG); // Code length will be stored here
}

void c32_addC32References() {
    outputB(0);

    unsigned long savedPos = pos;

    pos = 0x400 + 12;
    outputW(savedPos - pos, C32_FMT_LONG); // Store code length

    pos = savedPos;

    outputChars("REFS", 4);
    outputW(referenceCount, C32_FMT_LONG);

    c32_Reference* currentReference = firstReference;

    while (currentReference) {
        outputW(currentReference->pos, C32_FMT_LONG);

        currentReference = currentReference-> next;
    }
}

void c32_assemble(c32_Token* firstToken, char** outputPtr, unsigned long* lengthPtr, c32_BinaryFormat binaryFormat) {
    output = (char*)C32_ASM_MALLOC(C32_BLOCK_SIZE);
    pos = 0x400;
    length = 0;
    firstLabel = C32_ASM_NULL;
    lastLabel = C32_ASM_NULL;
    firstReference = C32_ASM_NULL;
    lastReference = C32_ASM_NULL;
    firstGroupLevel = C32_ASM_NULL;
    lastGroupLevel = C32_ASM_NULL;
    firstMacro = C32_ASM_NULL;
    lastMacro = C32_ASM_NULL;
    firstMacroLevel = C32_ASM_NULL;
    lastMacroLevel = C32_ASM_NULL;
    macroLevelCount = 0;
    referenceCount = 0;

    grow();

    if (binaryFormat) {
        c32_addC32Header();
    }

    c32_Token* token = firstToken;
    unsigned long currentGlobalHashId = 0;
    unsigned long currentLocalHashId = 0;
    unsigned long currentGlobalStartPos = 0;
    unsigned long currentLocalStartPos = 0;
    unsigned int rawLevel = 0;
    c32_MacroLevel macroLevel;

    while (token) {
        bool skipNextToken = false;
        bool skipMacroExit = false;
    
        switch (token->type) {
            case C32_TOK_ERROR:
                break;

            case C32_TOK_OP:
                outputB(token->value.asOpcode | token->format);
                break;

            case C32_TOK_INT:
                if (rawLevel == 0) outputB(C32_OP_PUT | token->format);
                outputW(token->value.asInt, token->format);
                break;

            case C32_TOK_STRING: {
                unsigned int length = 0;
                for (; token->value.asString[length]; length++);
                if (rawLevel == 0) {
                    Format format = getShortestFormat(length + 1);
                    outputB(C32_OP_PUT | C32_FMT_LONG);
                    outputW(pos + 6 + getFormatLength(format), C32_FMT_LONG);
                    outputB(C32_OP_PUT | format);
                    outputW(length + 2, format);
                    outputB(C32_OP_JUMP | format | C32_REL_REL);
                }
                for (unsigned int i = 0; token->value.asString[i]; i++) {
                    outputB(token->value.asString[i]);
                }
                if (rawLevel == 0) outputB('\0');
                break;
            }

            case C32_TOK_DEFINE:
                if (token->format == C32_FMT_LOCAL) {
                    setLabelSize(currentGlobalHashId, currentLocalHashId, pos - currentLocalStartPos);
                    currentLocalHashId = token->value.asIdHash;
                    currentLocalStartPos = pos;
                    createLabel(currentGlobalHashId, currentLocalHashId);
                } else {
                    if (currentGlobalHashId) setLabelSize(currentGlobalHashId, 0, pos - currentGlobalStartPos);
                    if (currentLocalHashId) setLabelSize(currentGlobalHashId, currentLocalHashId, pos - currentLocalStartPos);
                    currentGlobalHashId = token->value.asIdHash;
                    currentLocalHashId = 0;
                    currentGlobalStartPos = pos;
                    createLabel(currentGlobalHashId, 0);
                }
                break;

            case C32_TOK_CALL: case C32_TOK_CALL_COND: case C32_TOK_ADDR: case C32_TOK_ADDR_EXT:
            case C32_TOK_LOCAL_OFFSET: case C32_TOK_LOCAL_OFFSET_EXT:
            case C32_TOK_SIZE_OF: case C32_TOK_SIZE_OF_EXT:
                if (rawLevel == 0) outputB(C32_OP_PUT | C32_FMT_LONG);
                createReference(token, currentGlobalHashId);
                outputW(0, C32_FMT_LONG);
                if (rawLevel == 0 && token->type == C32_TOK_CALL) outputB(C32_OP_CALL | C32_FMT_LONG);
                if (rawLevel == 0 && token->type == C32_TOK_CALL_COND) outputB(C32_OP_CIF | C32_FMT_LONG);
                if (
                    token->type == C32_TOK_ADDR_EXT ||
                    token->type == C32_TOK_LOCAL_OFFSET_EXT ||
                    token->type == C32_TOK_SIZE_OF_EXT
                ) skipNextToken = true;
                break;

            case C32_TOK_RAW_OPEN:
                rawLevel++;
                break;

            case C32_TOK_RAW_CLOSE:
                if (rawLevel == 0) {
                    C32_ASM_PRINTF_STDERR("Mismatched raw bracket\n");
                    break;
                }
                rawLevel--;
                break;

            case C32_TOK_C32_GROUP_OPEN:
                pushGroupLevel(token);
                if (token->value.asGroupType == C32_GROUP_STD) break;
                if (token->value.asGroupType == C32_GROUP_COND) outputB(C32_OP_NOT);
                outputB(C32_OP_PUT | C32_FMT_LONG);
                outputW(0, C32_FMT_LONG);
                if (token->value.asGroupType == C32_GROUP_QUOTED) {
                    outputB(C32_OP_PUT | C32_FMT_LONG);
                    outputW(0, C32_FMT_LONG);
                }
                outputB((token->value.asGroupType == C32_GROUP_COND ? C32_OP_IF : C32_OP_JUMP) | C32_FMT_LONG);
                break;

            case C32_TOK_C32_GROUP_CLOSE:
                resolveGroupLevel();
                break;

            case C32_TOK_POS_ABS:
                pos = token->value.asInt; grow();
                break;

            case C32_TOK_POS_OFFSET:
                pos += token->value.asInt; grow();
                break;

            case C32_TOK_SIZE_OF_OFFSET: case C32_TOK_SIZE_OF_OFFSET_EXT: {
                c32_Label* label = C32_ASM_NULL;

                if (token->format == C32_FMT_LOCAL) {
                    label = resolveLabel(currentGlobalHashId, token->value.asIdHash);
                } else if (token->type == C32_TOK_SIZE_OF_OFFSET_EXT) {
                    if (!token->next || token->next->type != C32_TOK_CALL) {
                        C32_ASM_PRINTF_STDERR("Invalid subsequent token\n");
                        break;
                    }

                    label = resolveLabel(token->value.asIdHash, token->next->value.asIdHash);
                    skipNextToken = true;
                } else {
                    label = resolveLabel(token->value.asIdHash, 0);
                }

                if (!label) {
                    C32_ASM_PRINTF_STDERR("Undefined reference (note: future references not allowed)\n");
                    break;
                }

                pos += label->size; grow();
                break;
            }

            case C32_TOK_MACRO: {
                c32_Macro* macro = reoslveMacro(token->value.asIdHash);

                if (!macro) {
                    C32_ASM_PRINTF_STDERR("Undefined macro\n");
                    break;
                }

                if (macroLevelCount >= C32_MAX_MACRO_DEPTH - 1) {
                    C32_ASM_PRINTF_STDERR("Maximum macro depth limit reached\n");
                    break;
                }

                pushMacroLevel(token->next);

                token = macro->firstToken;
                skipMacroExit = true;
                break;
            }

            case C32_TOK_MACRO_DEFINE: {
                unsigned int groupLevel = 0;

                createMacro(token->value.asIdHash, token);

                token = token->next;

                while (token) {
                    if (token->type == C32_TOK_C32_GROUP_OPEN) groupLevel++;
                    if (token->type == C32_TOK_C32_GROUP_CLOSE && groupLevel > 0) groupLevel--;

                    if (groupLevel == 0) {
                        break;
                    }

                    token = token->next;
                }

                break;
            }

            default:
                C32_ASM_PRINTF_STDERR("Token type not implemented\n");
                break;
        }

        if (token) {
            token = token->next;
        }

        if (skipNextToken && token) {
            token = token->next;
        }

        if (!skipMacroExit && lastMacroLevel && lastMacroLevel->baseGroupLevel == lastGroupLevel && popMacroLevel(&macroLevel)) {
            token = macroLevel.continuec32_Token;
        }
    }

    setLabelSize(currentGlobalHashId, 0, pos - currentGlobalStartPos);
    setLabelSize(currentGlobalHashId, currentLocalHashId, pos - currentLocalStartPos);

    unsigned long savedPos = pos;

    resolveReferences();

    pos = savedPos;

    if (binaryFormat & C32_BINFMT_REFS) {
        c32_addC32References();
    }

    *outputPtr = (char*)C32_ASM_REALLOC(output, length);
    *lengthPtr = length;
}