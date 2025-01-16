#include <stdio.h>

#include "core32.h"

const c32_Byte C32_WIDTHS[4] = {1, 2, 4, 4};

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

c32_Long c32_readW(CORE32* vm, c32_Byte mode, c32_Long* p) {
    c32_Byte width = C32_WIDTHS[C32_FMT(mode)];

    return (
        c32_read(vm, p) |
        (width >= 2 ? c32_read(vm, p) << 8 : 0) |
        (width >= 3 ? c32_read(vm, p) << 16 : 0) |
        (width >= 4 ? c32_read(vm, p) << 24 : 0)
    );
}

void c32_writeW(CORE32* vm, c32_Byte mode, c32_Long* p, c32_Long data) {
    c32_Byte width = C32_WIDTHS[C32_FMT(mode)];

    c32_write(vm, p, data & 0x000000FF);
    width >= 2 ? c32_write(vm, p, (data & 0x0000FF00) >> 8) : 0;
    width >= 3 ? c32_write(vm, p, (data & 0x00FF0000) >> 16) : 0;
    width >= 4 ? c32_write(vm, p, (data & 0xFF000000) >> 24) : 0;
}

c32_Long c32_pop(CORE32* vm, c32_Byte mode) {
    vm->dsp -= C32_WIDTHS[C32_FMT(mode)];

    c32_Long result = c32_readW(vm, mode, &vm->dsp);

    vm->dsp -= C32_WIDTHS[C32_FMT(mode)];

    return result;
}

void c32_push(CORE32* vm, c32_Byte mode, c32_Long data) {
    c32_writeW(vm, mode, &vm->dsp, data);
}

void c32_step(CORE32* vm) {
    c32_Byte inst = c32_read(vm, &vm->ip);

    switch (inst & 0b11111000) {
        case 0b00000000: { // data
            c32_push(vm, inst, c32_readW(vm, inst, &vm->ip));
            break;
        }

        case 0b00001000: { // pop
            c32_pop(vm, inst);
            break;
        }

        case 0b00010000: { // nip
            c32_Long b = c32_pop(vm, inst);
            c32_Long a = c32_pop(vm, inst);
            c32_push(vm, inst, b);
            break;
        }

        case 0b00011000: { // dupe
            c32_Long a = c32_pop(vm, inst);
            c32_push(vm, inst, a);
            c32_push(vm, inst, a);
            break;
        }

        case 0b00100000: { // swap
            c32_Long b = c32_pop(vm, inst);
            c32_Long a = c32_pop(vm, inst);
            c32_push(vm, inst, b);
            c32_push(vm, inst, a);
            break;
        }

        case 0b00101000: { // over
            c32_Long b = c32_pop(vm, inst);
            c32_Long a = c32_pop(vm, inst);
            c32_push(vm, inst, a);
            c32_push(vm, inst, b);
            c32_push(vm, inst, a);
            break;
        }

        case 0b00110000: { // from
            c32_LongOrFloat a = {.asLong = c32_pop(vm, inst)};
            c32_push(vm, C32_FMT_LONG, C32_FMT(inst) == C32_FMT_FLOAT ? a.asFloat : a.asLong);
            break;
        }

        case 0b00111000: { // to
            c32_LongOrFloat a = C32_FMT(inst) == C32_FMT_FLOAT ? (c32_LongOrFloat) {.asFloat = c32_pop(vm, C32_FMT_LONG)} : (c32_LongOrFloat) {.asLong = c32_pop(vm, C32_FMT_LONG)};
            c32_push(vm, inst, a.asLong);
            break;
        }

        // TODO: Implement other instructions
    }
}