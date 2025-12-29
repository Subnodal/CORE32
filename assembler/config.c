#include "config.h"

#ifndef C32_ASM_RECONFIG_FS
    void getSelfPath(char selfPath[C32_ASM_PATH_MAX]) {
        readlink("/proc/self/exe", selfPath, C32_ASM_PATH_MAX);
    }

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

    bool writeFile(char* path, char* data, size_t size) {
        FILE* fp = fopen(path, "w");

        if (!fp) {
            return false;
        }

        fwrite(data, 1, size, fp);
        fclose(fp);

        return true;
    }
#endif