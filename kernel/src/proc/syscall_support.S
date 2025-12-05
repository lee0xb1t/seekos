    #include "./syscall_macro.S"
    
    .global syscall_handler0
    .extern syscall_funs

    PERCPU_KSTACK=24
    PERCPU_USTACK=32

syscall_handler0:
    mfence
    swapgs

    movq %rsp, %gs:PERCPU_USTACK
    movq %gs:PERCPU_KSTACK, %rsp

    pushq %rcx      /*user rip*/
    pushq %r11      /*user eflags*/

    push_all

    sti
    call *syscall_funs(, %rax, 8)
    cli

    pop_all

    popq %r11
    popq %rcx

    movq %rsp, %gs:PERCPU_KSTACK
    movq %gs:PERCPU_USTACK, %rsp

    swapgs
    mfence
    sysretq

    movq $0xdead009, %rax
    cli
    hlt