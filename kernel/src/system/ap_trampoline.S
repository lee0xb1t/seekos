.global _ap_trampoline, _ap_trampoline32
.global _gdtr, _gdt, _gdt_end
.global ap_trampoline_start, ap_trampoline_end

CODE_SEG = 0x08
DATA_SEG = 0x10

CR0_PE   = (1 << 0)
CR0_PG   = (1 << 31)

CR4_PAE  = (1 << 5)

EFER_MSR = 0xC0000080
EFER_LM_ENABLE = (1 << 8)


.code16
ap_trampoline_start:
    jmp _ap_trampoline

_long_idt_ptr:
    .quad 0
_long_stack_ptr:
    .quad 0
_long_cr3_val:
    .quad 0
_long_entry_ptr:
    .quad 0
_long_cpu_ctrl:
    .quad 0

_ap_trampoline:
    cli
    xorw %ax, %ax
    mov %ax, %es
    mov %ax, %ds
    mov %ax, %ss
    movl %cr0, %eax
    orl $CR0_PE, %eax
    movl %eax, %cr0
    lgdt _gdtr
    ljmpl $CODE_SEG, $_ap_trampoline32

_gdt:
    .long   0, 0
    .long   0x0000ffff, 0x00cf9a00    /*code segment*/
    .long   0x0000ffff, 0x00cf9300    /*data segment*/
_gdt_end:

_gdtr:
    .word   (_gdt_end - _gdt - 1)
    .long   _gdt

.code32
_ap_trampoline32:
    movw $DATA_SEG, %ax
    movw %ax, %es
    movw %ax, %ds
    movw %ax, %ss
    movw %ax, %fs

    mov %cr4, %eax
    orl $CR4_PAE, %eax
    mov %eax, %cr4

    // update cr3
    movl _long_cr3_val, %eax
    movl %eax, %cr3

    movl $EFER_MSR, %ecx
    rdmsr
    orl $EFER_LM_ENABLE, %eax
    wrmsr

    mov %cr0, %eax
    or $CR0_PG, %eax
    mov %eax, %cr0

    lgdt _gdtr_64

    ljmpl $CODE_SEG, $_ap_trampoline64

    mov $0xdead32, %eax
    hlt


_gdt64:
    .long   0, 0
    .long   0x0000ffff, 0x00af9a00    /*code segment*/
    .long   0x0000ffff, 0x008f9200    /*data segment*/
_gdt64_end:

_gdtr_64:
    .word   (_gdt64_end - _gdt64 - 1)
    .long   _gdt64

.code64
_ap_trampoline64:
    xorq %rax, %rax
    movq _long_cr3_val(%rip), %rax
    movq %rax, %cr3

    movq _long_stack_ptr(%rip), %rax
    movq %rax, %rsp

    movq _long_cpu_ctrl(%rip), %rdi

    movq _long_entry_ptr(%rip), %rax
    call *%rax

    movq $0xdead64, %rax
    hlt

ap_trampoline_end:
