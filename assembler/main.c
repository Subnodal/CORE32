#include <stdlib.h>
#include <stdio.h>

#include "parser.h"

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
        fclose(fp);

        free(data);

        return 1;
    }

    data[size] = '\0';

    Token* firstToken = parse(data);

    inspect(firstToken);

    return 0;
}