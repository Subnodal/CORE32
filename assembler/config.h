#ifndef C32_ASM_RECONFIG_LIBS
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <linux/limits.h>
    #include <libgen.h>
    #include <unistd.h>
    #include <stdbool.h>
#endif

#define C32_ASM_NULL NULL

#ifndef C32_ASM_RECONFIG_PRINTING
    #define C32_ASM_PRINTF(format, ...) printf(format, ##__VA_ARGS__)
    #define C32_ASM_PRINTF_STDERR(format, ...) fprintf(stderr, format, ##__VA_ARGS__)
#endif

#ifndef C32_ASM_RECONFIG_MEMORY
    #define C32_ASM_MALLOC(size) malloc(size)
    #define C32_ASM_REALLOC(ptr, size) realloc(ptr, size)
    #define C32_ASM_FREE(ptr) free(ptr)
#endif

#ifndef C32_ASM_RECONFIG_STRINGS
    #define C32_ASM_STRLEN(string) strlen(string)
    #define C32_ASM_STRCAT(destination, part) strcat(destination, part)
    #define C32_ASM_STRCPY(destination, source) strcpy(destination, source)
#endif

#ifndef C32_ASM_RECONFIG_FS
    #define C32_ASM_READ_FILE(path, dataPtr, sizePtr) readFile(path, dataPtr, sizePtr)
    #define C32_ASM_WRITE_FILE(path, data, size) writeFile(path, data, size)
    #define C32_ASM_PATH_MAX PATH_MAX
    #define C32_ASM_GET_SELF_PATH(selfPath) getSelfPath(selfPath)
    #define C32_ASM_DIRNAME(path) dirname(path)

    void getSelfPath(char selfPath[C32_ASM_PATH_MAX]);
    bool readFile(char* path, char** dataPtr, size_t* sizePtr);
    bool writeFile(char* path, char* data, size_t size);
#endif