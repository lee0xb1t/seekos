    .global _start
    .extern main

    .section .note.GNU-stack, "", @progbits

    .section .text
_start:
    popq %rdi
    popq %rsi
    popq %rdx
    call main

    movq %rax, %rdi
    movq $1, %rax
    syscall

    hlt
    jmp .
