SELECTOR_KERNEL_CODE = 0x08
SELECTOR_KERNEL_DATA = 0x10

    .global kernel_stack_of_tss, kernel_stack_of_ist1, kernel_stack_of_ist2
    .global _flush_gdt, interrupt_stack

    .text
    .type _flush_gdt, @function
_flush_gdt:
    pushq   %rax
    movq    $SELECTOR_KERNEL_CODE, %rax
    pushq   %rax
    leaq    _flush_jmp_to(%rip), %rax
    pushq   %rax
    retfq
_flush_jmp_to:
    movw    $SELECTOR_KERNEL_DATA, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    movw    %ax, %fs
    popq    %rax
    ret


    .data
    .align  16
kernel_stack_of_tss_low:
    .fill   4096*2, 1, 0     //8kb interrupt stack
kernel_stack_of_tss:
    

    .align  16
kernel_stack_of_ist1_low:
    .fill   4096*2, 1, 0     //8kb interrupt stack
kernel_stack_of_ist1:

    .align  16
kernel_stack_of_ist2_low:
    .fill   4096*2, 1, 0     //8kb interrupt stack
kernel_stack_of_ist2:
