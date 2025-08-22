#include <stdlib.h>
#include <stdbool.h>

bool readFile(char* path, char** dataPtr, size_t* sizePtr);
bool writeFile(char* path, char* data, size_t size);