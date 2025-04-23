#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "assembler.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "No input file specified\n");

        return 1;
    }

    FILE* fp = fopen(argv[1], "r");

    if (!fp) {
        fprintf(stderr, "Error when reading file\n");

        return 1;
    }

    fseek(fp, 0, SEEK_END);

    size_t size = ftell(fp);
    char* data = malloc(size + 1);

    fseek(fp, 0, SEEK_SET);

    if (fread(data, sizeof(char), size, fp) != size) {
        fprintf(stderr, "Error reading file contents\n");

        return 1;
    }

    data[size] = '\0';

    Token* firstToken = parse(data);

    fclose(fp);

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

    fp = fopen(argv[3], "w");

    if (!fp) {
        fprintf(stderr, "Error when writing file\n");

        return 1;
    }

    fwrite(output + offset, 1, length - offset, fp);

    return 0;
}