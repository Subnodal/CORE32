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
    int argFormat = ARG_FORMAT_C32;
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
            argFormat = ARG_FORMAT_RAW;
            continue;
        }

        if (strcmp(argv[i], "--debug") == 0) {
            showDebugMessages = true;
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

    if (!readFile(infile, &data, NULL)) {
        fprintf(stderr, "Error reading file contents\n");

        return 1;
    }

    Token* firstToken = parse(data, infile);

    free(data);

    if (showDebugMessages) inspect(firstToken);

    char* output;
    unsigned long length;
    unsigned long offset = 0x400;

    assemble(firstToken, &output, &length, argFormat == ARG_FORMAT_C32);

    if (!outfile) {
        fprintf(stderr, "No output file specified\n");

        return 1;
    }

    FILE* fp = fopen(outfile, "w");

    if (!writeFile(outfile, output + offset, length - offset)) {
        fprintf(stderr, "Error when writing file\n");

        return 1;
    }

    return 0;
}