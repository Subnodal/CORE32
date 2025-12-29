#include "config.h"
#include "parser.h"

#define C32_MATCH_BASE(prefix, base) if (c32_matchChars(&code, prefix, false)) { \
    if (!c32_matchUInt(&code, base, &intResult)) goto error; \
        tokenToAdd.type = numberTokenType; \
        tokenToAdd.value.asInt = intResult; \
        tokenToAdd.format = c32_getFormatSuffix(&code); \
        goto addToken; \
    }

char* opNames[] = {
//  *000    *001    *010    *101    *110    *101    *110    *111
    "ret",  "drop", "mod",  "dup",  "swap", "over", "from", "to",   // 00*
    "sub",  "add",  "div",  "mul",  "or",   "xor",  "and",  "not",  // 01*
    "neq",  "eq",   "gtn",  "bsr",  "dec",  "ltn",  "bsl",  "inc",  // 10*
    "put",  "jump", "call", "int",  "get",  "set",  "cif",  "if",   // 11*
    NULL
};

char* opSymbols[] = {
//  *000    *001    *010    *101    *110    *101    *110    *111
    "",     "",     "%",    "",     "",     "",     "",     "",     // 00*
    "-",    "+",    "/",    "*",    "|",    "^",    "&",    "!",    // 01*
    "!=",   "=",    ">",    ">>",   "--",   "<",    "<<",   "++",   // 10*
    "",     "",     "",     "",     "",     "",     "",     "",     // 11*
    NULL
};

bool c32_showDebugMessages = false;
unsigned long* includedPaths = NULL;
unsigned int includedPathsCount = 0;
c32_CachedIdentifier* firstCachedIdentifier = NULL;
c32_CachedIdentifier* lastCachedIdentifier = NULL;

char* joinPaths(const char* base, const char* relative) {
    char* result = (char*)C32_ASM_MALLOC(strlen(base) + strlen(relative) + 2);

    result[0] = '\0';

    C32_ASM_STRCAT(result, base);
    C32_ASM_STRCAT(result, "/");
    C32_ASM_STRCAT(result, relative);

    return result;
}

unsigned long hashPath(char* path) {
    // Using djb2 algorithm for hashing

    unsigned int i = 0;
    unsigned long hash = 5381;

    while (path[i]) {
        hash = (hash << 5) + hash + path[i];

        i++;
    }

    return hash;
}

char getEscapeChar(char c) {
    switch (c) {
        case 'a': return '\a';
        case 'b': return '\b';
        case 'e': return '\e';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case '\\': return '\\';
        case '\'': return '\'';
        case '"': return '"';
        default: return '\0';
    }
}

bool c32_matchChars(char** codePtr, char* match, bool whole) {
    unsigned int i = 0;

    while (true) {
        if (!match[i]) {
            char final = (*codePtr)[i];

            if (whole && !(
                final == '\0' || final == ' ' || final == '\t' || final == '\n' ||
                final == '\'' || final == '"' || final == '%' ||
                final == ':' || final == ']' || final == '}'
            )) {
                return false;
            }

            (*codePtr) += i;
            return true;
        }

        if ((*codePtr)[i] != match[i]) return false;

        i++;
    }
}

bool c32_matchInList(char** codePtr, char* list[], unsigned int* index, bool whole) {
    unsigned int i = 0;

    while (list[i]) {
        if (!list[i][0]) {
            // Ignore empty entries
            i++;
            continue;
        }

        if (c32_matchChars(codePtr, list[i], whole)) {
            *index = i;
            return true;
        }

        i++;
    }

    return false;
}

bool c32_matchUInt(char** codePtr, int base, unsigned int* result) {
    unsigned int i = 0;
    char* code = *codePtr;

    *result = 0;

    while (true) {
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

    *codePtr += i;

    return i > 0;
}

bool c32_matchIdentifier(char** codePtr, unsigned long* result) {
    // Using djb2 algorithm for hashing

    unsigned int i = 0;
    unsigned char* code = *codePtr;
    char* string = (char*)C32_ASM_MALLOC(1);
    unsigned long hash = 5381;

    string[0] = '\0';

    while (true) {
        if (!(
            (code[i] >= 'a' && code[i] <= 'z') ||
            (code[i] >= 'A' && code[i] <= 'Z') ||
            code[i] == '_' ||
            (i > 0 && code[i] >= '0' && code[i] <= '9')
        )) {
            break;
        }

        hash = (hash << 5) + hash + code[i];

        string = (char*)C32_ASM_REALLOC(string, i + 2);
        string[i] = code[i];
        string[i + 1] = '\0';

        i++;
    }

    if (i == 0) {
        C32_ASM_FREE(string);

        return false;
    }

    *codePtr += i;
    *result = hash;

    if (!c32_idHashToString(hash)) {
        c32_CachedIdentifier* cachedIdentifier = (c32_CachedIdentifier*)C32_ASM_MALLOC(sizeof(c32_CachedIdentifier));

        cachedIdentifier->idHash = hash;
        cachedIdentifier->string = string;
        cachedIdentifier->next = NULL;

        if (!firstCachedIdentifier) firstCachedIdentifier = cachedIdentifier;
        if (lastCachedIdentifier) lastCachedIdentifier->next = cachedIdentifier;

        lastCachedIdentifier = cachedIdentifier;
    }

    return true;
}

char* c32_idHashToString(unsigned long idHash) {
    c32_CachedIdentifier* currentCachedIdentifier = firstCachedIdentifier;

    while (currentCachedIdentifier) {
        if (currentCachedIdentifier->idHash == idHash) {
            return currentCachedIdentifier->string;
        }

        currentCachedIdentifier = currentCachedIdentifier->next;
    }

    return NULL;
}

bool matchString(char** codePtr, char** resultPtr) {
    unsigned int i = 0;
    unsigned char* code = *codePtr;

    if (code[i++] != '"') {
        return false;
    }

    char* result = (char*)C32_ASM_MALLOC(1);
    unsigned int resultLength = 0;

    while (code[i] != '"') {
        if (code[i] == '\0') {
            C32_ASM_FREE(result);

            return false;
        }

        char c = code[i++];

        if (c == '\\') {
            c = getEscapeChar(code[i++]);
        }

        result = (char*)C32_ASM_REALLOC(result, resultLength + 1);
        result[resultLength++] = c;
    }

    i++; // To match string's closing quote

    result[resultLength] = '\0';

    *codePtr += i;
    *resultPtr = result;

    return true;
}

bool matchPath(char** codePtr, char** resultPtr) {
    unsigned int i = 0;
    unsigned char* code = *codePtr;
    char* result = (char*)C32_ASM_MALLOC(1);
    unsigned int resultLength = 0;
    bool corelib = false;
    bool escaping = false;

    if (code[i] != '.') {
        char selfPath[C32_ASM_PATH_MAX];

        C32_ASM_GET_SELF_PATH(selfPath);

        char* prefix = joinPaths(C32_ASM_DIRNAME(selfPath), "../../corelib/");

        corelib = true;
        resultLength = C32_ASM_STRLEN(prefix);
        result = (char*)C32_ASM_REALLOC(result, resultLength + 1);

        C32_ASM_STRCPY(result, prefix);

        C32_ASM_FREE(prefix);
    }

    while (code[i] != '\0' && (escaping || code[i] != ' ' && code[i] != '\t' && code[i] != '\n')) {
        char c = code[i++];

        escaping = !escaping && c == '\\';

        if (escaping) continue;

        result = (char*)C32_ASM_REALLOC(result, resultLength + 1);
        result[resultLength++] = c;
    }

    result[resultLength] = '\0';

    *codePtr += i;
    *resultPtr = result;

    return !corelib;
}

Format c32_getFormatSuffix(char** code) {
    if ((*code)[0] == '\'') {
        (*code)++;
        return C32_FMT_BYTE;
    }

    if ((*code)[0] == '\"') {
        (*code)++;
        return C32_FMT_SHORT;
    }

    if ((*code)[0] == '%') {
        (*code)++;
        return C32_FMT_FLOAT;
    }

    return C32_FMT_LONG;
}

c32_Token* c32_parse(char* code, char* path) {
    c32_Token* firstToken = NULL;
    c32_Token* lastToken = NULL;
    c32_Token tokenToAdd;
    unsigned int length;
    int commentLevel = 0;
    unsigned int intResult = 0;
    unsigned long hashResult = 0;
    Format resultFormat = C32_FMT_BYTE;
    bool expectSizeOfLocalNext = false;

    if (!includedPaths) includedPaths = (unsigned long*)C32_ASM_MALLOC(0);

    while (code[0]) {
        c32_TokenType numberTokenType = C32_TOK_INT;

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

        if (code[0] == '\'') {
            char c = code[1];

            code += 2;

            if (c =='\\') {
                c = getEscapeChar(code[0]);

                code++;
            }

            if (code[0] != '\'') {
                goto error;
            }

            code++;

            tokenToAdd.type = C32_TOK_INT;
            tokenToAdd.value.asInt = c;
            tokenToAdd.format = C32_FMT_BYTE;

            goto addToken;
        }

        if (code[0] == '"') {
            char* result;

            if (!matchString(&code, &result)) {
                goto error;
            }

            tokenToAdd.type = C32_TOK_STRING;
            tokenToAdd.value.asString = result;

            goto addToken;
        }

        if (code[0] == '[' || code[0] == ']') {
            tokenToAdd.type = code[0] == ']' ? C32_TOK_RAW_CLOSE : C32_TOK_RAW_OPEN;
            code++;

            goto addToken;
        }

        if (code[0] == '{' || code[0] == '}') {
            tokenToAdd.type = code[0] == '}' ? C32_TOK_C32_GROUP_CLOSE : C32_TOK_C32_GROUP_OPEN;
            tokenToAdd.value.asGroupType = C32_GROUP_STD;
            code++;

            goto addToken;
        }

        if (c32_matchChars(&code, "?{", false)) {
            tokenToAdd.type = C32_TOK_C32_GROUP_OPEN;
            tokenToAdd.value.asGroupType = C32_GROUP_COND;

            goto addToken;
        }

        if (c32_matchChars(&code, ":{", false)) {
            tokenToAdd.type = C32_TOK_C32_GROUP_OPEN;
            tokenToAdd.value.asGroupType = C32_GROUP_QUOTED;

            goto addToken;
        }

        if (c32_matchChars(&code, "${", false)) {
            tokenToAdd.type = C32_TOK_C32_GROUP_OPEN;
            tokenToAdd.value.asGroupType = C32_GROUP_SKIPPED;

            goto addToken;
        }

        if (c32_matchInList(&code, opNames, &intResult, true) || c32_matchInList(&code, opSymbols, &intResult, true)) {
            tokenToAdd.type = C32_TOK_OP;
            tokenToAdd.value.asOpcode = intResult << 3;
            tokenToAdd.format = c32_getFormatSuffix(&code);

            goto addToken;
        }

        if (c32_matchIdentifier(&code, &hashResult)) {
            tokenToAdd.type = C32_TOK_CALL;
            tokenToAdd.value.asIdHash = hashResult;
            tokenToAdd.format = C32_FMT_GLOBAL;

            if (code[0] == ':' || code[0] == '~') {
                tokenToAdd.type = code[0] == ':' ? C32_TOK_DEFINE : C32_TOK_SIZE_OF;
                code++;
            }

            if (code[0] == '.') {
                tokenToAdd.type = C32_TOK_SIZE_OF_EXT;
                expectSizeOfLocalNext = true;
            }

            goto addToken;
        }

        if (code[0] == '?' || code[0] == '$' || code[0] == '.') {
            char prefix = code[0];
            bool local = prefix == '.';

            code++;

            if (!local && code[0] == '.') {
                local = true;
                code++;
            }

            if (!c32_matchIdentifier(&code, &hashResult)) {
                goto error;
            }

            tokenToAdd.type = C32_TOK_CALL;
            tokenToAdd.value.asIdHash = hashResult;
            tokenToAdd.format = local ? C32_FMT_LOCAL : C32_FMT_GLOBAL;

            if (expectSizeOfLocalNext) {
                if (code[0] != '~') goto error;
                expectSizeOfLocalNext = false;
                code++;
                goto addToken;
            }

            if (prefix == '.' && code[0] == ':') {
                tokenToAdd.type = C32_TOK_DEFINE;
                code++;
            } else if (prefix == '?') {
                tokenToAdd.type = C32_TOK_CALL_COND;
            } else if (prefix == '$') {
                tokenToAdd.type = code[0] == '.' ? C32_TOK_ADDR_EXT : C32_TOK_ADDR;
            } else if (code[0] == '~') {
                tokenToAdd.type = C32_TOK_SIZE_OF;
                code++;
            }

            goto addToken;
        }

        if (code[0] == '#') {
            bool isLocalOffset = false;

            code++;

            if (code[0] == '.') {
                isLocalOffset = true;
                code++;
            }

            if (!c32_matchIdentifier(&code, &hashResult)) {
                goto error;
            }

            tokenToAdd.type = C32_TOK_MACRO;
            tokenToAdd.value.asIdHash = hashResult;

            if (isLocalOffset) {
                tokenToAdd.type = C32_TOK_LOCAL_OFFSET;
                goto addToken;
            }

            if (code[0] == '.') {
                tokenToAdd.type = C32_TOK_LOCAL_OFFSET_EXT;
            } else if (code[0] == ':') {
                tokenToAdd.type = C32_TOK_MACRO_DEFINE;
                code++;
            }

            goto addToken;
        }

        if (code[0] == '+') {
            char* includedPath;
            char* relativePath;
            char* includedCode = NULL;

            code++;

            if (matchPath(&code, &relativePath)) {
                char* pathDir = dirname(strdup(path));

                includedPath = realpath(joinPaths(pathDir, relativePath), NULL);

                C32_ASM_FREE(pathDir);

                if (!includedPath) {
                    C32_ASM_PRINTF_STDERR("Invalid path: %s\n", relativePath);

                    goto error;
                }

                C32_ASM_FREE(relativePath);
            } else {
                includedPath = relativePath;
            }

            unsigned long hashedPath = hashPath(includedPath);

            for (unsigned int i = 0; i < includedPathsCount; i++) {
                if (includedPaths[i] == hashedPath) goto skipInclusion;
            }

            C32_ASM_PRINTF("Including: %s\n", includedPath);

            includedPaths = (unsigned long*)C32_ASM_REALLOC(includedPaths, sizeof(unsigned long) * (++includedPathsCount));
            includedPaths[includedPathsCount - 1] = hashedPath;

            if (!c32_readFile(includedPath, &includedCode, NULL)) goto error;

            c32_Token* includedToken = c32_parse(includedCode, includedPath);

            if (!includedToken) goto skipInclusion;
            if (!firstToken) firstToken = includedToken;
            if (lastToken) lastToken->next = includedToken;

            lastToken = includedToken;

            while (lastToken->next) lastToken = lastToken->next;

            skipInclusion:

            C32_ASM_FREE(includedPath);

            continue;
        }

        if (code[0] == '@') {
            numberTokenType = C32_TOK_POS_ABS;
            code++;
        } else if (code[0] == '~') {
            bool local = code[1] == '.';

            code += local ? 2 : 1;

            if (c32_matchIdentifier(&code, &hashResult)) {
                tokenToAdd.type = code[0] == '.' ? C32_TOK_SIZE_OF_OFFSET_EXT : C32_TOK_SIZE_OF_OFFSET;
                tokenToAdd.value.asIdHash = hashResult;
                tokenToAdd.format = local ? C32_FMT_LOCAL : C32_FMT_GLOBAL;

                goto addToken;
            }

            numberTokenType = C32_TOK_POS_OFFSET;
        }

        C32_MATCH_BASE("0b", 2);
        C32_MATCH_BASE("0o", 8);
        C32_MATCH_BASE("0x", 16);

        if (c32_matchUInt(&code, 10, &intResult)) {
            tokenToAdd.type = numberTokenType;
            tokenToAdd.value.asInt = intResult;
            tokenToAdd.format = c32_getFormatSuffix(&code);

            goto addToken;
        }

        error:

        tokenToAdd.type = C32_TOK_ERROR;

        addToken:

        c32_Token* tokenPtr = (c32_Token*)C32_ASM_MALLOC(sizeof(c32_Token));

        tokenPtr->type = tokenToAdd.type;
        tokenPtr->value = tokenToAdd.value;
        tokenPtr->format = tokenToAdd.format;
        tokenPtr->next = NULL;

        if (!firstToken) firstToken = tokenPtr;
        if (lastToken) lastToken->next = tokenPtr;

        lastToken = tokenPtr;

        if (tokenToAdd.type == C32_TOK_ERROR) {
            break;
        }
    }

    return firstToken;
}

const char* c32_inspectFormat(c32_Token* token) {
    if (token->format == C32_FMT_BYTE) return "'";
    if (token->format == C32_FMT_SHORT) return "\"";
    if (token->format == C32_FMT_FLOAT) return "%";
    return "";
}

void c32_inspect(c32_Token* token) {
    if (!token) {
        printf("\n");
        return;
    }

    switch (token->type) {
        case C32_TOK_ERROR:
            printf("(error) ");
            break;

        case C32_TOK_OP:
            printf("op(%s%s) ", opNames[token->value.asOpcode >> 3], c32_inspectFormat(token));
            break;

        case C32_TOK_INT:
            printf("int(%d%s) ", token->value.asInt, c32_inspectFormat(token));
            break;

        case C32_TOK_STRING:
            printf("string(%s) ", token->value.asString);
            break;

        case C32_TOK_DEFINE:
            printf("def(%s%s) ", token->format == C32_FMT_LOCAL ? "." : "", c32_idHashToString(token->value.asIdHash));
            break;

        case C32_TOK_CALL:
            printf("call(%s%s) ", token->format == C32_FMT_LOCAL ? "." : "", c32_idHashToString(token->value.asIdHash));
            break;

        case C32_TOK_CALL_COND:
            printf("callif(%s%s) ", token->format == C32_FMT_LOCAL ? "." : "", c32_idHashToString(token->value.asIdHash));
            break;

        case C32_TOK_ADDR:
            printf("addr(%s%s) ", token->format == C32_FMT_LOCAL ? "." : "", c32_idHashToString(token->value.asIdHash));
            break;

        case C32_TOK_ADDR_EXT:
            printf("addrext(%s%s) ", token->format == C32_FMT_LOCAL ? "." : "", c32_idHashToString(token->value.asIdHash));
            break;

        case C32_TOK_RAW_OPEN:
            printf("raw ");
            break;

        case C32_TOK_RAW_CLOSE:
            printf("endraw ");
            break;

        case C32_TOK_C32_GROUP_OPEN:
            printf("group(%c) ", token->value.asGroupType);
            break;

        case C32_TOK_C32_GROUP_CLOSE:
            printf("endgroup ");
            break;

        case C32_TOK_POS_ABS:
            printf("posabs(%ld%s) ", token->value.asInt, c32_inspectFormat(token));
            break;

        case C32_TOK_POS_OFFSET:
            printf("posoffset(%ld%s) ", token->value.asInt, c32_inspectFormat(token));
            break;

        case C32_TOK_SIZE_OF:
            printf("sizeof(%s%s) ", token->format == C32_FMT_LOCAL ? "." : "", c32_idHashToString(token->value.asInt));
            break;

        case C32_TOK_SIZE_OF_EXT:
            printf("sizeofext(%ld) ", token->value.asInt);
            break;

        case C32_TOK_LOCAL_OFFSET:
            printf("offset(.%s) ", c32_idHashToString(token->value.asIdHash));
            break;

        case C32_TOK_LOCAL_OFFSET_EXT:
            printf("offsetext(%s) ", c32_idHashToString(token->value.asIdHash));
            break;

        case C32_TOK_MACRO:
            printf("macro(%s) ", c32_idHashToString(token->value.asIdHash));
            break;

        case C32_TOK_MACRO_DEFINE:
            printf("defmacro(%s) ", c32_idHashToString(token->value.asIdHash));
            break;

        default:
            printf("(unknown) ");
            break;
    }

    c32_inspect(token->next);
}