#include <stdio.h>

#include "core32.h"

// unsigned char code[] = {
//     0b00000001, 0x12, 0x34,             // 0x12 0x34
//     0b00000001, 0xAA, 0xBB,             // 0xAA 0xBB
//     0b00011000,                         // dupe
//     0b00001000,                         // pop
//     0b00000000, 0xCC,                   // 0xCC
//     0b00100001,                         // swap'
//     0b00010000,                         // nip
//     0b00101000,                         // over
//     0b00000010, 0x10, 0x20, 0x30, 0x00, // 0x10 0x20 0x30 0x00
//     0b00110010,                         // from
//     0b00111011,                         // to%
//     0b00110011,                         // from%
//     0b00110010                          // to
// };

unsigned char code[] = {
    0b00000000, 0x00,
    0b00110000,
    0b10111010,
    0b00000000, 0x03,
    0b11000000
};

CORE32* vm;

void peek() {
    for (unsigned int i = 0x200; i < 0x210; i++) {
        printf("%02x", vm->mem[i]);
    }

    printf("\n");
}

int main(int argc, char* argv[]) {
    vm = c32_new(code, sizeof(code));

    while (vm->ip < sizeof(code)) {
        c32_step(vm);
        peek();
    }

    return 0;
}