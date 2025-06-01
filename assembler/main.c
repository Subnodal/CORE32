#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "files.h"
#include "parser.h"
#include "assembler.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "No input file specified\n");

        return 1;
    }

    char* data;

    if (!readFile(argv[1], &data, NULL)) {
        fprintf(stderr, "Error reading file contents\n");

        return 1;
    }

    Token* firstToken = parse(data, argv[1]);

    free(data);

    inspect(firstToken);

    char* output;
    unsigned long length;
    unsigned long offset = 0x400;

    assemble(firstToken, &output, &length);

    if (argc < 4 || strcmp(argv[2], "-o") != 0) {
        fprintf(stderr, "No output file specified\n");

        return 1;
    }

    FILE* fp = fopen(argv[3], "w");

    if (!fp) {
        fprintf(stderr, "Error when writing file\n");

        return 1;
    }

    fwrite(output + offset, 1, length - offset, fp);

    return 0;
}