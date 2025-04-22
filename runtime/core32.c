#include <stdio.h>

#include "core32.h"

const c32_Byte C32_WIDTHS[4] = {1, 2, 4, 4};

#define C32_FMT(inst) (inst & 0b11)
#define C32_REL(inst) (inst & 0b100)
#define C32_OFFSET(inst, addr) (addr + (C32_REL(inst) ? vm->ip - 1 : 0))

#define C32_BIT_OP(op) {c32_Long b = c32_pop(vm, inst); c32_Long a = c32_pop(vm, inst); c32_push(vm, inst, a op b); break;}
#define C32_LOGIC_OP(op) {c32_Long b = c32_pop(vm, inst); c32_Long a = c32_pop(vm, inst); c32_push(vm, C32_FMT_BYTE, a op b); break;}

#define C32_FLOAT_OP(op) { \
        if (C32_FMT(inst) == C32_FMT_FLOAT) { \
            c32_Float b = c32_popFloat(vm, inst); c32_Float a = c32_popFloat(vm, inst); c32_pushFloat(vm, inst, a op b); \
        } else { \
            c32_Long b = c32_pop(vm, inst); c32_Long a = c32_pop(vm, inst); c32_push(vm, inst, a op b); \
        } \
        break; \
    }

CORE32* c32_new(c32_Byte* code, c32_Long codeLength) {
    CORE32* vm = C32_MALLOC(sizeof(CORE32));

    vm->ip = 0x0;
    vm->csp = C32_CSP_BASE;
    vm->dsp = C32_DSP_BASE;
    vm->running = 1;
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
    if (width >= 2) c32_write(vm, p, (data & 0x0000FF00) >> 8);
    if (width >= 3) c32_write(vm, p, (data & 0x00FF0000) >> 16);
    if (width >= 4) c32_write(vm, p, (data & 0xFF000000) >> 24);
}

c32_Long c32_pop(CORE32* vm, c32_Byte mode) {
    vm->dsp -= C32_WIDTHS[C32_FMT(mode)];
    c32_Long result = c32_readW(vm, mode, &vm->dsp);
    vm->dsp -= C32_WIDTHS[C32_FMT(mode)];

    return result;
}

c32_Float c32_popFloat(CORE32* vm, c32_Byte mode) {
    return ((c32_LongOrFloat) {.asLong = c32_pop(vm, mode)}).asFloat;
}

void c32_push(CORE32* vm, c32_Byte mode, c32_Long data) {
    c32_writeW(vm, mode, &vm->dsp, data);
}

void c32_pushFloat(CORE32* vm, c32_Byte mode, c32_Float data) {
    c32_LongOrFloat dataLof = {.asFloat = data};

    c32_writeW(vm, mode, &vm->dsp, dataLof.asLong);
}

void c32_pushCall(CORE32* vm, c32_Long data) {
    c32_writeW(vm, C32_FMT_LONG, &vm->csp, data);
}

c32_Long c32_popCall(CORE32* vm) {
    vm->csp -= 4;
    c32_Long result = c32_readW(vm, C32_FMT_LONG, &vm->csp);
    vm->csp -= 4;

    return result;
}

void c32_step(CORE32* vm) {
    if (!vm->running) {
        return;
    }

    c32_Byte inst = c32_read(vm, &vm->ip);

    switch (inst & 0b11111000) {
        case C32_OP_RET: {
            vm->ip = c32_popCall(vm);
            if (vm->csp < C32_CSP_BASE) vm->running = 0;
            break;
        }

        case C32_OP_DROP: {
            c32_pop(vm, inst);
            break;
        }

        case C32_OP_MOD: C32_BIT_OP(%)

        case C32_OP_DUP: {
            c32_Long a = c32_pop(vm, inst);
            c32_push(vm, inst, a);
            c32_push(vm, inst, a);
            break;
        }

        case C32_OP_SWAP: {
            c32_Long b = c32_pop(vm, inst);
            c32_Long a = c32_pop(vm, inst);
            c32_push(vm, inst, b);
            c32_push(vm, inst, a);
            break;
        }

        case C32_OP_OVER: {
            c32_Long b = c32_pop(vm, inst);
            c32_Long a = c32_pop(vm, inst);
            c32_push(vm, inst, a);
            c32_push(vm, inst, b);
            c32_push(vm, inst, a);
            break;
        }

        case C32_OP_FROM: {
            c32_push(vm, C32_FMT_LONG, C32_FMT(inst) == C32_FMT_FLOAT ? ((c32_LongOrFloat) {.asLong = c32_pop(vm, inst)}).asFloat : c32_pop(vm, inst));
            break;
        }

        case C32_OP_TO: {
            c32_push(vm, inst, C32_FMT(inst) == C32_FMT_FLOAT ? ((c32_LongOrFloat) {.asFloat = c32_pop(vm, C32_FMT_LONG)}).asLong : c32_pop(vm, C32_FMT_LONG));
            break;
        }

        case C32_OP_SUB: C32_FLOAT_OP(-)
        case C32_OP_ADD: C32_FLOAT_OP(+)
        case C32_OP_DIV: C32_FLOAT_OP(/)
        case C32_OP_MUL: C32_FLOAT_OP(*)

        case C32_OP_OR: C32_BIT_OP(|)
        case C32_OP_XOR: C32_BIT_OP(^)
        case C32_OP_AND: C32_BIT_OP(&)

        case C32_OP_NOT: {
            c32_push(vm, C32_FMT_BYTE, c32_pop(vm, inst) == 0 ? 1 : 0);
            break;
        }

        case C32_OP_NEQ: C32_LOGIC_OP(!=)
        case C32_OP_EQ: C32_LOGIC_OP(==)
        case C32_OP_GTN: C32_LOGIC_OP(>)
        case C32_OP_BSR: C32_BIT_OP(>>)

        case C32_OP_DEC: {
            c32_push(vm, inst, c32_pop(vm, inst) - 1);
            break;
        }

        case C32_OP_LTN: C32_LOGIC_OP(<)
        case C32_OP_BSL: C32_BIT_OP(<<)

        case C32_OP_INC: {
            c32_push(vm, inst, c32_pop(vm, inst) + 1);
            break;
        }

        case C32_OP_PUT: {
            c32_push(vm, inst, c32_readW(vm, inst, &vm->ip));
            break;
        }

        case C32_OP_CALL: c32_pushCall(vm, vm->ip); // Fall through

        case C32_OP_JUMP: {
            vm->ip = C32_OFFSET(inst, c32_pop(vm, inst));
            break;
        }

        case C32_OP_INT: {
            printf("%c", c32_pop(vm, inst));
            break;
        }

        case C32_OP_GET: {
            c32_Long p = C32_OFFSET(inst, c32_pop(vm, C32_FMT_LONG));
            c32_push(vm, inst, c32_readW(vm, inst, &p));
            break;
        }

        case C32_OP_SET: {
            c32_Long p = C32_OFFSET(inst, c32_pop(vm, C32_FMT_LONG));
            c32_writeW(vm, inst, &p, c32_pop(vm, inst));
            break;
        }

        case C32_OP_CIF: case C32_OP_IF: {
            c32_Long p = C32_OFFSET(inst, c32_pop(vm, inst));
            c32_Byte cond = c32_pop(vm, C32_FMT_BYTE);
            if (cond) {
                if (inst & 0b11111000 == C32_OP_CIF) c32_pushCall(vm, vm->ip);
                vm->ip = p;
            }
            break;
        }
    }
}