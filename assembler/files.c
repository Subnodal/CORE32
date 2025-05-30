#include <stdio.h>

#include "files.h"

bool readFile(char* path, char** dataPtr, size_t* sizePtr) {
    FILE* fp = fopen(path, "r");

    if (!fp) {
        return false;
    }

    fseek(fp, 0, SEEK_END);

    size_t size = ftell(fp);
    char* data = malloc(size + 1);

    fseek(fp, 0, SEEK_SET);

    if (fread(data, sizeof(char), size, fp) != size) {
        return false;
    }

    data[size] = '\0';

    if (dataPtr) *dataPtr = data;
    if (sizePtr) *sizePtr = size;

    fclose(fp);

    return true;
}