#include <stdio.h>

#include "core32.h"
#include "handlers.h"

CORE32* vm;

void peek() {
    for (unsigned int i = 0x200; i < vm->dsp; i++) {
        printf("%02x", vm->mem[i]);
    }

    printf("\n");
}

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
    char* data = calloc(size + C32_ENTRY_POINT + 1, 1);

    fseek(fp, 0, SEEK_SET);

    if (fread(data + C32_ENTRY_POINT, sizeof(char), size, fp) != size) {
        fprintf(stderr, "Error reading file contents\n");
        fclose(fp);

        free(data);

        return 1;
    }

    data[size + C32_ENTRY_POINT] = '\0';

    vm = c32_new(data, size + C32_ENTRY_POINT);

    if (
        size > 16 &&
        data[C32_ENTRY_POINT + 0] == '\0' &&
        data[C32_ENTRY_POINT + 1] == 'C' &&
        data[C32_ENTRY_POINT + 2] == '3' &&
        data[C32_ENTRY_POINT + 3] == '2'
    ) {
        vm->ip += 16;
    }

    c32_registerBaseHandlers(vm);

    while (vm->running) {
        c32_step(vm);
    }

    return 0;
}