    .global _start
    .extern main

    .section .note.GNU-stack, "", @progbits

    .section .text
_start:
    popq %rdi
    popq %rsi
    call main

    // exit
    movq %rax, %rdi
    movq $1, %rax
    syscall

    // noreturn
    hlt
    jmp .