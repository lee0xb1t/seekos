//
// 进入保护模式后，tty模块无效
//

#include "code16gcc.h"
#include "tty.h"

void bios_putchar(char c) {
    __asm__ __volatile__(
        "movb $0x0e, %%ah;"
        "movb %0, %%al   ;"
        "movw $0x0007, %%bx;"
        "int $0x10       ;"
        :
        : "r" (c)
    );
}

void putchar(char c) {
    if (c == '\n') {
        putchar('\r');
    }
    
    bios_putchar(c);
}

void put_byte_number(char c) {
    if (c >= 0 && c < 10) {
        bios_putchar(c | 0x30);
    }
}

void put_dword_number(int n) {
    int i = 1000000000;

    if (n == 0) {
        bios_putchar('0');
    }

    while (1) {
        char v = (n / i) % 10;
        
        put_byte_number(v);

        i /= 10;

        if (i == 0)
            break;
    }
}

void puts(const char* str) {
    while (*str)
        putchar(*str++);
}
