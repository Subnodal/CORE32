#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "parser.h"
#include "assembler.h"

enum {
    ARG_STATE_NONE,
    ARG_STATE_INFILE,
    ARG_STATE_OUTFILE
};

enum {
    ARG_FORMAT_C32,
    ARG_FORMAT_RAW
};

int main(int argc, char* argv[]) {
    int argState = ARG_STATE_INFILE;
    c32_BinaryFormat binaryFormat = C32_BINFMT_CODE;
    char* infile = NULL;
    char* outfile = NULL;

    for (unsigned int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) {
            argState = ARG_STATE_INFILE;
            continue;
        }

        if (strcmp(argv[i], "-o") == 0) {
            argState = ARG_STATE_OUTFILE;
            continue;
        }
        
        if (strcmp(argv[i], "--raw") == 0) {
            binaryFormat = 0;
            continue;
        }

        if (strcmp(argv[i], "--refs") == 0) {
            binaryFormat = C32_BINFMT_CODE | C32_BINFMT_REFS;
            continue;
        }

        if (strcmp(argv[i], "--debug") == 0) {
            c32_showDebugMessages = true;
            continue;
        }

        if (argState == ARG_STATE_INFILE) {
            infile = argv[i];
            argState = ARG_STATE_NONE;
        } else if (argState == ARG_STATE_OUTFILE) {
            outfile = argv[i];
            argState = ARG_STATE_NONE;
        } else {
            fprintf(stderr, "Invalid argument\n");

            return 1;
        }
    }

    if (!infile) {
        fprintf(stderr, "No input file specified\n");

        return 1;
    }

    char* data;

    if (!c32_readFile(infile, &data, NULL)) {
        fprintf(stderr, "Error reading file contents\n");

        return 1;
    }

    c32_Token* firstToken = c32_parse(data, infile);

    free(data);

    if (c32_showDebugMessages) c32_inspect(firstToken);

    char* output;
    unsigned long length;
    unsigned long offset = 0x400;

    c32_assemble(firstToken, &output, &length, binaryFormat);

    if (!outfile) {
        fprintf(stderr, "No output file specified\n");

        return 1;
    }

    FILE* fp = fopen(outfile, "w");

    if (!c32_writeFile(outfile, output + offset, length - offset)) {
        fprintf(stderr, "Error when writing file\n");

        return 1;
    }

    free(output);

    return 0;
}