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

void c32_systemCall(CORE32* vm) {
    c32_pushCall(vm, vm->ip);
    vm->ip = vm->ssr;
}

C32_HANDLER_GETTER(c32_getIp, vm->ip, C32_FMT_LONG);
C32_HANDLER_SETTER(c32_setIp, vm->ip, C32_FMT_LONG);

C32_HANDLER_GETTER(c32_getCsp, vm->csp, C32_FMT_LONG);
C32_HANDLER_SETTER(c32_setCsp, vm->csp, C32_FMT_LONG);

C32_HANDLER_GETTER(c32_getDsp, vm->dsp, C32_FMT_LONG);
C32_HANDLER_SETTER(c32_setDsp, vm->dsp, C32_FMT_LONG);

C32_HANDLER_GETTER(c32_getSsr, vm->ssr, C32_FMT_LONG);
C32_HANDLER_SETTER(c32_setSsr, vm->ssr, C32_FMT_LONG);

void c32_registerBaseHandlers(CORE32* vm) {
    c32_registerHandler(vm, 'o', c32_outputByte);
    c32_registerHandler(vm, 's', c32_systemCall);
    c32_registerHandler(vm, 'c', c32_getCsp);
    c32_registerHandler(vm, 'C', c32_setCsp);
    c32_registerHandler(vm, 'd', c32_getDsp);
    c32_registerHandler(vm, 'D', c32_setDsp);
    c32_registerHandler(vm, 'r', c32_getSsr);
    c32_registerHandler(vm, 'R', c32_setSsr);
}