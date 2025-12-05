
.macro push_all
    pushq   %rax
    pushq   %rcx
    pushq   %rdx
    pushq   %rbx
    pushq   %rbp
    pushq   %rsi
    pushq   %rdi
    pushq   %r8
    pushq   %r9
    pushq   %r10
    pushq   %r11
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15
.endm


.macro pop_all
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %r11
    popq    %r10
    popq    %r9
    popq    %r8
    popq    %rdi
    popq    %rsi
    popq    %rbp
    popq    %rbx
    popq    %rdx
    popq    %rcx
    popq    %rax
.endm


.macro EXCEPTION_HANDLER error_code=0, trapno, function
    // swapgs
    .if \error_code == 0
    pushq   $0
    .endif

    push_all

    pushq   $\trapno

    movq    %rsp, %rdi
    call    \function

    addq    $8, %rsp

    pop_all

    /* Remove Error Code */
    addq    $8, %rsp

    iretq
.endm


.macro INTERRUPT_HANDLER trapno, function
    pushq   $0

    push_all

    pushq   $\trapno

    movq    %rsp, %rdi
    call    \function

    addq    $8, %rsp

    pop_all

    /* Remove Error Code */
    addq    $8, %rsp

    iretq
.endm