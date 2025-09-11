#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <linux/limits.h>

#include "parser.h"
#include "files.h"

#define MATCH_BASE(prefix, base) if (matchChars(&code, prefix, false)) { \
    if (!matchUInt(&code, base, &intResult)) goto error; \
        tokenToAdd.type = numberTokenType; \
        tokenToAdd.value.asInt = intResult; \
        tokenToAdd.format = getFormatSuffix(&code); \
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

unsigned long* includedPaths = NULL;
unsigned int includedPathsCount = 0;

char* joinPaths(const char* base, const char* relative) {
    char* result = malloc(strlen(base) + strlen(relative) + 2);

    result[0] = '\0';

    strcat(result, base);
    strcat(result, "/");
    strcat(result, relative);

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

bool matchChars(char** codePtr, char* match, bool whole) {
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

bool matchInList(char** codePtr, char* list[], unsigned int* index, bool whole) {
    unsigned int i = 0;

    while (list[i]) {
        if (!list[i][0]) {
            // Ignore empty entries
            i++;
            continue;
        }

        if (matchChars(codePtr, list[i], whole)) {
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

bool matchIdentifier(char** codePtr, unsigned long* result) {
    // Using djb2 algorithm for hashing

    unsigned int i = 0;
    unsigned char* code = *codePtr;
    unsigned long hash = 5381;

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

        i++;
    }

    if (i == 0) {
        return false;
    }

    *codePtr += i;
    *result = hash;

    return true;
}

bool matchString(char** codePtr, char** resultPtr) {
    unsigned int i = 0;
    unsigned char* code = *codePtr;

    if (code[i++] != '"') {
        return false;
    }

    char* result = malloc(1);
    unsigned int resultLength = 0;

    while (code[i] != '"') {
        if (code[i] == '\0') {
            free(result);

            return false;
        }

        char c = code[i++];

        if (c == '\\') {
            c = getEscapeChar(code[i++]);
        }

        result = realloc(result, resultLength + 1);
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
    char* result = malloc(1);
    unsigned int resultLength = 0;
    bool corelib = false;
    bool escaping = false;

    if (code[i] != '.') {
        char selfPath[PATH_MAX];

        readlink("/proc/self/exe", selfPath, PATH_MAX);

        char* prefix = joinPaths(dirname(selfPath), "../../corelib/");

        corelib = true;
        resultLength = strlen(prefix);
        result = realloc(result, resultLength + 1);

        strcpy(result, prefix);

        free(prefix);
    }

    while (code[i] != '\0' && (escaping || code[i] != ' ' && code[i] != '\t' && code[i] != '\n')) {
        char c = code[i++];

        escaping = !escaping && c == '\\';

        if (escaping) continue;

        result = realloc(result, resultLength + 1);
        result[resultLength++] = c;
    }

    result[resultLength] = '\0';

    *codePtr += i;
    *resultPtr = result;

    return !corelib;
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

Token* parse(char* code, char* path) {
    Token* firstToken = NULL;
    Token* lastToken = NULL;
    Token tokenToAdd;
    unsigned int length;
    int commentLevel = 0;
    unsigned int intResult = 0;
    unsigned long hashResult = 0;
    Format resultFormat = FMT_BYTE;
    bool expectSizeOfLocalNext = false;

    if (!includedPaths) includedPaths = malloc(0);

    while (code[0]) {
        TokenType numberTokenType = TOK_INT;

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

            tokenToAdd.type = TOK_INT;
            tokenToAdd.value.asInt = c;
            tokenToAdd.format = FMT_BYTE;

            goto addToken;
        }

        if (code[0] == '"') {
            char* result;

            if (!matchString(&code, &result)) {
                goto error;
            }

            tokenToAdd.type = TOK_STRING;
            tokenToAdd.value.asString = result;

            goto addToken;
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

        if (matchChars(&code, "?{", false)) {
            tokenToAdd.type = TOK_GROUP_OPEN;
            tokenToAdd.value.asGroupType = GROUP_COND;

            goto addToken;
        }

        if (matchChars(&code, ":{", false)) {
            tokenToAdd.type = TOK_GROUP_OPEN;
            tokenToAdd.value.asGroupType = GROUP_QUOTED;

            goto addToken;
        }

        if (matchChars(&code, "${", false)) {
            tokenToAdd.type = TOK_GROUP_OPEN;
            tokenToAdd.value.asGroupType = GROUP_SKIPPED;

            goto addToken;
        }

        if (matchInList(&code, opNames, &intResult, true) || matchInList(&code, opSymbols, &intResult, true)) {
            tokenToAdd.type = TOK_OP;
            tokenToAdd.value.asOpcode = intResult << 3;
            tokenToAdd.format = getFormatSuffix(&code);

            goto addToken;
        }

        if (matchIdentifier(&code, &hashResult)) {
            tokenToAdd.type = TOK_CALL;
            tokenToAdd.value.asIdHash = hashResult;
            tokenToAdd.format = FMT_GLOBAL;

            if (code[0] == ':' || code[0] == '~') {
                tokenToAdd.type = code[0] == ':' ? TOK_DEFINE : TOK_SIZE_OF;
                code++;
            }

            if (code[0] == '.') {
                tokenToAdd.type = TOK_SIZE_OF_EXT;
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

            if (!matchIdentifier(&code, &hashResult)) {
                goto error;
            }

            tokenToAdd.type = TOK_CALL;
            tokenToAdd.value.asIdHash = hashResult;
            tokenToAdd.format = local ? FMT_LOCAL : FMT_GLOBAL;

            if (expectSizeOfLocalNext) {
                if (code[0] != '~') goto error;
                expectSizeOfLocalNext = false;
                code++;
                goto addToken;
            }

            if (prefix == '.' && code[0] == ':') {
                tokenToAdd.type = TOK_DEFINE;
                code++;
            } else if (prefix == '?') {
                tokenToAdd.type = TOK_CALL_COND;
            } else if (prefix == '$') {
                tokenToAdd.type = code[0] == '.' ? TOK_ADDR_EXT : TOK_ADDR;
            } else if (code[0] == '~') {
                tokenToAdd.type = TOK_SIZE_OF;
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

            if (!matchIdentifier(&code, &hashResult)) {
                goto error;
            }

            tokenToAdd.type = TOK_MACRO;
            tokenToAdd.value.asIdHash = hashResult;

            if (isLocalOffset) {
                tokenToAdd.type = TOK_LOCAL_OFFSET;
                goto addToken;
            }

            if (code[0] == '.') {
                tokenToAdd.type = TOK_LOCAL_OFFSET_EXT;
            } else if (code[0] == ':') {
                tokenToAdd.type = TOK_MACRO_DEFINE;
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

                free(pathDir);

                if (!includedPath) {
                    fprintf(stderr, "Invalid path: %s\n", relativePath);

                    goto error;
                }

                free(relativePath);
            } else {
                includedPath = relativePath;
            }

            unsigned long hashedPath = hashPath(includedPath);

            for (unsigned int i = 0; i < includedPathsCount; i++) {
                if (includedPaths[i] == hashedPath) goto skipInclusion;
            }

            printf("Including: %s\n", includedPath);

            includedPaths = realloc(includedPaths, sizeof(unsigned long) * (++includedPathsCount));
            includedPaths[includedPathsCount - 1] = hashedPath;

            if (!readFile(includedPath, &includedCode, NULL)) goto error;

            Token* includedToken = parse(includedCode, includedPath);

            if (!includedToken) goto skipInclusion;
            if (!firstToken) firstToken = includedToken;
            if (lastToken) lastToken->next = includedToken;

            lastToken = includedToken;

            while (lastToken->next) lastToken = lastToken->next;

            skipInclusion:

            free(includedPath);

            continue;
        }

        if (code[0] == '@') {
            numberTokenType = TOK_POS_ABS;
            code++;
        } else if (code[0] == '~') {
            bool local = code[1] == '.';

            code += local ? 2 : 1;

            if (matchIdentifier(&code, &hashResult)) {
                tokenToAdd.type = code[0] == '.' ? TOK_SIZE_OF_OFFSET_EXT : TOK_SIZE_OF_OFFSET;
                tokenToAdd.value.asIdHash = hashResult;
                tokenToAdd.format = local ? FMT_LOCAL : FMT_GLOBAL;

                goto addToken;
            }

            numberTokenType = TOK_POS_OFFSET;
        }

        MATCH_BASE("0b", 2);
        MATCH_BASE("0o", 8);
        MATCH_BASE("0x", 16);

        if (matchUInt(&code, 10, &intResult)) {
            tokenToAdd.type = numberTokenType;
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

        case TOK_STRING:
            printf("string(%s) ", token->value.asString);
            break;

        case TOK_DEFINE:
            printf("def(%s%ld) ", token->format == FMT_LOCAL ? "." : "", token->value.asIdHash);
            break;

        case TOK_CALL:
            printf("call(%s%ld) ", token->format == FMT_LOCAL ? "." : "", token->value.asIdHash);
            break;

        case TOK_CALL_COND:
            printf("callif(%s%ld) ", token->format == FMT_LOCAL ? "." : "", token->value.asIdHash);
            break;

        case TOK_ADDR:
            printf("addr(%s%ld) ", token->format == FMT_LOCAL ? "." : "", token->value.asIdHash);
            break;

        case TOK_ADDR_EXT:
            printf("addrext(%s%ld) ", token->format == FMT_LOCAL ? "." : "", token->value.asIdHash);
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

        case TOK_POS_ABS:
            printf("posabs(%ld%s) ", token->value.asInt, inspectFormat(token));
            break;

        case TOK_POS_OFFSET:
            printf("posoffset(%ld%s) ", token->value.asInt, inspectFormat(token));
            break;

        case TOK_SIZE_OF:
            printf("sizeof(%s%ld) ", token->format == FMT_LOCAL ? "." : "", token->value.asInt);
            break;

        case TOK_SIZE_OF_EXT:
            printf("sizeofext(%ld) ", token->value.asInt);
            break;

        case TOK_LOCAL_OFFSET:
            printf("offset(.%ld) ", token->value.asIdHash);
            break;

        case TOK_LOCAL_OFFSET_EXT:
            printf("offsetext(%ld) ", token->value.asIdHash);
            break;

        case TOK_MACRO:
            printf("macro(%ld) ", token->value.asIdHash);
            break;

        case TOK_MACRO_DEFINE:
            printf("defmacro(%ld) ", token->value.asIdHash);
            break;

        default:
            printf("(unknown) ");
            break;
    }

    inspect(token->next);
}