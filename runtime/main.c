#include <stdio.h>

#include "core32.h"

unsigned char code[] = {
    0b00000010, 0x12, 0x34,
    0b00000010, 0xaa, 0xbb,
    0b00011001,
    0b00010001,
    0b00000001, 0xcc,
    0b00100010,
    0b00101001,
    0b00110001,
    0b00011001,
    0b00000001, 0xdd,
    0b00110010,
    0b00111001
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