    #include "./sched_context.S"
    #include "../system/idt_macro.S"

    .global __switch_to, __sched_task_start, __sched_task_start_user
    .extern sched_end, _sched_do_execve


    #define TASK_TSTACK(x)  48(x)


__switch_to:
    testq %rcx, %rcx
    jnz .skip_save_gr
    save_gr
.skip_save_gr:
    mfence
    testq %rdx, %rdx
    jz .skip_switch_cr3
    movq %rdx, %cr3
    mfence
.skip_switch_cr3:
    testq %rcx, %rcx
    jnz .skip_save_rsp
    testq %rsi, %rsi
    jz .skip_save_rsp
    movq %rsp, TASK_TSTACK(%rsi)
.skip_save_rsp:
    movq TASK_TSTACK(%rdi), %rsp
    restore_gr
    ret


__sched_task_start:
    /* unlock timer */
    sti

    popq %rax
    popq %rdi
    call %rax

    /* lock timer */
    cli

    movq $0, %rdi /* exitcode */
    call __sched_kernel_end

__sched_task_start_user:
    popq %rax
    popq %rdi

    call _sched_do_execve

    addq $8, %rsp  /*trapno*/
    pop_all
    addq $8, %rsp  /*errorcode*/
    swapgs
    iretq
    hlt
