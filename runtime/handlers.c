#include <stdio.h>

#include "handlers.h"

c32_Long c32_id(c32_Byte* name) {
    c32_Long id = 0;

    while (*name) {
        id <<= 8;
        id |= *name;
 
        name++;
    }

    return id;
}

void c32_outputByte(CORE32* vm) {
    printf("%c", c32_pop(vm, C32_FMT_BYTE));
}

void c32_registerBaseHandlers(CORE32* vm) {
    c32_registerHandler(vm, 'o', c32_outputByte);
}