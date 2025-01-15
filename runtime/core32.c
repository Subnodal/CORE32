#include <stdio.h>

#include "core32.h"

const c32_Byte C32_WIDTHS[4] = {4, 1, 2, 4};

#define C32_FMT(inst) (inst & 0b11)
#define C32_REL(inst) (inst & 0b100)

CORE32* c32_new(c32_Byte* code, c32_Long codeLength) {
    CORE32* vm = C32_MALLOC(sizeof(CORE32));

    vm->ip = 0x0;
    vm->csp = 0x100;
    vm->dsp = 0x200;
    vm->mem = C32_MALLOC(C32_MEM_SIZE);

    for (c32_Long i = 0; i < C32_MEM_SIZE; i++) {
        vm->mem[i] = 0;
    }

    for (c32_Long i = 0; i < codeLength; i++) {
        vm->mem[i] = code[i];
    }

    return vm;
}

c32_Byte c32_read(CORE32* vm, c32_Long* p) {
    c32_Byte result = *p <= C32_MEM_SIZE ? vm->mem[*p] : 0;

    (*p)++;

    return result;
}

void c32_write(CORE32* vm, c32_Long* p, c32_Byte data) {
    *p <= C32_MEM_SIZE && (vm->mem[*p] = data);

    (*p)++;
}

c32_Long c32_readW(CORE32* vm, c32_Byte inst, c32_Long* p) {
    c32_Byte width = C32_WIDTHS[C32_FMT(inst)];

    return (
        c32_read(vm, p) |
        (width >= 2 ? c32_read(vm, p) << 8 : 0) |
        (width >= 3 ? c32_read(vm, p) << 16 : 0) |
        (width >= 4 ? c32_read(vm, p) << 24 : 0)
    );
}

void c32_writeW(CORE32* vm, c32_Byte inst, c32_Long* p, c32_Long data) {
    c32_Byte width = C32_WIDTHS[C32_FMT(inst)];

    c32_write(vm, p, data & 0x000000FF);
    width >= 2 ? c32_write(vm, p, (data & 0x0000FF00) >> 8) : 0;
    width >= 3 ? c32_write(vm, p, (data & 0x00FF0000) >> 16) : 0;
    width >= 4 ? c32_write(vm, p, (data & 0xFF000000) >> 24) : 0;
}

c32_Long c32_pop(CORE32* vm, c32_Byte inst) {
    vm->dsp -= C32_WIDTHS[C32_FMT(inst)];

    c32_Long result = c32_readW(vm, inst, &vm->dsp);

    vm->dsp -= C32_WIDTHS[C32_FMT(inst)];

    return result;
}

void c32_push(CORE32* vm, c32_Byte inst, c32_Long data) {
    c32_writeW(vm, inst, &vm->dsp, data);
}

void c32_step(CORE32* vm) {
    c32_Byte inst = c32_read(vm, &vm->ip);

    switch (inst & 0b11111000) {
        case 0b00000000: {c32_push(vm, inst, c32_readW(vm, inst, &vm->ip)); break;} // data
        case 0b00001000: break; // conv TODO: Implement
        case 0b00010000: {c32_pop(vm, inst); break;} // pop
        case 0b00011000: {c32_Long a = c32_pop(vm, inst); c32_push(vm, inst, a); c32_push(vm, inst, a); break;} // dupe
        case 0b00100000: {c32_Long b = c32_pop(vm, inst); c32_Long a = c32_pop(vm, inst); c32_push(vm, inst, b); c32_push(vm, inst, a); break;} // swap
        case 0b00101000: {c32_Long c = c32_pop(vm, inst); c32_pop(vm, inst); c32_Long a = c32_pop(vm, inst); c32_push(vm, inst, a); c32_push(vm, inst, c); break;} // nip
        case 0b00110000: {c32_Long c = c32_pop(vm, inst); c32_Long b = c32_pop(vm, inst); c32_Long a = c32_pop(vm, inst); c32_push(vm, inst, b); c32_push(vm, inst, c); c32_push(vm, inst, a); break;} // rot
        case 0b00111000: {c32_Long b = c32_pop(vm, inst); c32_Long a = c32_pop(vm, inst); c32_push(vm, inst, a); c32_push(vm, inst, b); c32_push(vm, inst, a); break;} // over
        // TODO: Implement other instructions
    }
}