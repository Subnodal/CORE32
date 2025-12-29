#include <stdio.h>

#include "handlers.h"

#define C32_HANDLER_GETTER(name, value, format) void name(CORE32* vm) { \
        c32_push(vm, format, value); \
    }

#define C32_HANDLER_SETTER(name, value, format) void name(CORE32* vm) { \
        value = c32_pop(vm, format); \
    }

c32_Long c32_id(c32_Byte* name) {
    c32_Long id = 0;
    c32_Byte length = 0;

    while (*name) {
        id >>= 8;
        id |= (*name << 24);
 
        name++;
        length++;
    }

    if (length < 4) {
        id >>= (4 - length) * 8;
    }

    return id;
}

c32_Byte* c32_safeGetBytes(CORE32* vm, c32_Long address) {
    if (address >= vm->memSize) {
        return NULL;
    }

    return (c32_Byte*)(vm->mem + address);
}

void c32_outputByte(CORE32* vm) {
    printf("%c", c32_pop(vm, C32_FMT_BYTE));
}

C32_HANDLER_GETTER(c32_getIp, vm->ip, C32_FMT_LONG);
C32_HANDLER_SETTER(c32_setIp, vm->ip, C32_FMT_LONG);

C32_HANDLER_GETTER(c32_getCsp, vm->csp, C32_FMT_LONG);
C32_HANDLER_SETTER(c32_setCsp, vm->csp, C32_FMT_LONG);

C32_HANDLER_GETTER(c32_getDsp, vm->dsp, C32_FMT_LONG);
C32_HANDLER_SETTER(c32_setDsp, vm->dsp, C32_FMT_LONG);

C32_HANDLER_GETTER(c32_getSsr, vm->ssr, C32_FMT_LONG);
C32_HANDLER_SETTER(c32_setSsr, vm->ssr, C32_FMT_LONG);

C32_HANDLER_GETTER(c32_getMemLimit, vm->memLimit, C32_FMT_LONG);
C32_HANDLER_SETTER(c32_setMemLimit, vm->memLimit, C32_FMT_LONG);

C32_HANDLER_GETTER(c32_getCodeEnd, vm->codeEnd, C32_FMT_LONG);
C32_HANDLER_SETTER(c32_setCodeEnd, vm->codeEnd, C32_FMT_LONG);

void c32_registerBaseHandlers(CORE32* vm) {
    c32_registerHandler(vm, 'o', c32_outputByte);
    c32_registerHandler(vm, 'c', c32_getCsp);
    c32_registerHandler(vm, 'C', c32_setCsp);
    c32_registerHandler(vm, 'd', c32_getDsp);
    c32_registerHandler(vm, 'D', c32_setDsp);
    c32_registerHandler(vm, 'r', c32_getSsr);
    c32_registerHandler(vm, 'R', c32_setSsr);
    c32_registerHandler(vm, 'e', c32_getCodeEnd);
    c32_registerHandler(vm, 'E', c32_setCodeEnd);
    c32_registerHandler(vm, 'l', c32_getMemLimit);
    c32_registerHandler(vm, 'L', c32_setMemLimit);
}