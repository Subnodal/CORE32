#include <stdio.h>

#include "core32.h"

// unsigned char code[] = {
//     C32_OP_DATA | C32_FMT_SHORT, 0x12, 0x34,            // 0x3412
//     C32_OP_DATA | C32_FMT_SHORT, 0xAA, 0xBB,            // 0xBBAA
//     C32_OP_DUP | C32_FMT_BYTE,                          // dupe
//     C32_OP_DROP | C32_FMT_BYTE,                         // drop
//     C32_OP_DATA | C32_FMT_BYTE, 0xCC,                   // 0xCC
//     C32_OP_SWAP | C32_FMT_SHORT,                        // swap'
//     C32_OP_OVER | C32_FMT_BYTE,                         // over
//     C32_OP_DATA | C32_FMT_LONG, 0x10, 0x20, 0x30, 0x00, // 0x00302010
//     C32_OP_FROM | C32_FMT_LONG,                         // from"
//     C32_OP_TO | C32_FMT_FLOAT,                          // to%
//     C32_OP_FROM | C32_FMT_FLOAT,                        // from%
//     C32_OP_TO | C32_FMT_LONG,                           // to"
//     C32_OP_DATA | C32_FMT_LONG, 0x01, 0x02, 0x03, 0x04, // 0x04030201
//     C32_OP_DATA | C32_FMT_LONG, 0x01, 0x01, 0x01, 0x01, // 0x01010101
//     C32_OP_SUB | C32_FMT_LONG,                          // -"
//     C32_OP_RET
// };

// unsigned char code[] = {
//     C32_OP_DATA | C32_FMT_BYTE, 0x00,                   // 0x00
//     C32_OP_FROM | C32_FMT_BYTE,                         // from
//     C32_OP_INC | C32_FMT_LONG,                          // inc"
//     C32_OP_DATA | C32_FMT_BYTE, 0x03,                   // 0x03
//     C32_OP_JUMP | C32_FMT_BYTE                          // jump
// };

unsigned char code[] = {
    C32_OP_DATA | C32_FMT_BYTE, 0x00,                       // 0x00
    C32_OP_DUP | C32_FMT_BYTE,                              // dup
    C32_OP_INT | C32_FMT_BYTE,                              // int
    C32_OP_INC | C32_FMT_BYTE,                              // ++
    C32_OP_DUP | C32_FMT_BYTE,                              // dup
    C32_OP_DATA | C32_FMT_BYTE, 0xFF,                       // 0xFF
    C32_OP_NEQ | C32_FMT_BYTE,                              // !=
    C32_OP_DATA | C32_FMT_BYTE, 0x02,                       // 0x02
    C32_OP_IF | C32_FMT_BYTE,                               // if
    C32_OP_DATA | C32_FMT_BYTE, 0x0A,                       // '\n'
    C32_OP_INT | C32_FMT_BYTE,                              // int
    C32_OP_RET                                              // ret
};

CORE32* vm;

void peek() {
    for (unsigned int i = 0x200; i < vm->dsp; i++) {
        printf("%02x", vm->mem[i]);
    }

    printf("\n");
}

int main(int argc, char* argv[]) {
    vm = c32_new(code, sizeof(code));

    while (vm->running) {
        c32_step(vm);
        // peek();
    }

    return 0;
}