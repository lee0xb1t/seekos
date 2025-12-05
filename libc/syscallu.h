#ifndef SYSCALL_H
#define SYSCALL_H

#define syscall_0(index, ret)    \
    do {        \
        asm volatile(       \
            "syscall;"       \
            : "=a"(ret)              \
            : "a"(index)    \
            : "rcx", "r11", "memory"    \
        );          \
    } while(0)

#define syscall_1(index, arg1, ret)    \
    do {        \
        asm volatile(       \
            "syscall;"       \
            : "=a"(ret)              \
            : "a"(index), "D"(arg1)    \
            : "rcx", "r11", "memory"    \
        );          \
    } while(0)

#define syscall_2(index, arg1, arg2, ret)    \
    do {        \
        asm volatile(       \
            "syscall;"       \
            : "=a"(ret)              \
            : "a"(index), "D"(arg1), "S"(arg2)    \
            : "rcx", "r11", "memory"    \
        );          \
    } while(0)

#define syscall_3(index, arg1, arg2, arg3, ret)    \
    do {        \
        asm volatile(       \
            "syscall;"       \
            : "=a"(ret)  \
            : "a"(index), "D"(arg1), "S"(arg2), "d"(arg3)    \
            : "rcx", "r11", "memory"    \
        );          \
    } while(0)

#define syscall_4(index, arg1, arg2, arg3, arg4, ret)    \
    do {        \
        asm volatile(       \
            "syscall;"       \
            : "=a"(ret)  \
            : "a"(index), "D"(arg1), "S"(arg2), "d"(arg3), "c"(arg4)    \
            : "rcx", "r11", "memory"    \
        );          \
    } while(0)

#define syscall_5(index, arg1, arg2, arg3, arg4, arg5, ret)    \
    do {        \
        asm volatile(       \
            "syscall;"       \
            : "=a"(ret)  \
            : "a"(index), "D"(arg1), "S"(arg2), "d"(arg3), "c"(arg4), "r8"(arg5)    \
            : "rcx", "r11", "memory"    \
        );          \
    } while(0)

#define syscall_6(index, arg1, arg2, arg3, arg4, arg5, arg6, ret)    \
    do {        \
        asm volatile(       \
            "syscall;"       \
            : "=a"(ret)  \
            : "a"(index), "D"(arg1), "S"(arg2), "d"(arg3), "c"(arg4), "r8"(arg5) "r9"(arg6)    \
            : "rcx", "r11", "memory"    \
        );          \
    } while(0)

#endif
