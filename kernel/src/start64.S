
//     KERNEL_BASE = 0xffffff8000000000

//     .extern kernel_main, __kernel_start

//     .code64
//     .section ".stext", "ax"
//     .global _start64, boot_params, pgtable
// _start64:
//     jmp     _start_skip

//     .align  8
// boot_params:
// _kernel_pa:
//     .quad   0
//     /* screen_info_t */
//     .quad   0
//     .long   0
//     .long   0
//     .long   0
//     .long   0
//     .long   0
//     .long   0
//     .long   0
//     .long   0
//     /* memory_desc_t */
//     .quad   0   /* mapcount */
//     .quad   0   /* available_memory */
//     .quad   0   /* total_memory */
//     .quad   0   /* max_phys_addr */
//     .fill   16*256, 1, 0
//     /*madt*/
//     .long   0   /* madt addr */
//     .long   0   /* madt size */
//     /*hpet*/
//     .long   0
//     .long   0
//     .quad   0
//     .byte   0
//     .short  0
//     .byte   0

// _start_skip:
//     // leaq    pgtable(%rip), %rax
//     // movq    %rax, kernel_pml4t(%rip)
    
//     // Initialize page tables
//     /* Build Level 4 */
//     leaq    0 + pgtable(%rip), %rdi
//     leaq    0x1007(%rdi), %rax
//     movq    %rax, (%rdi)

//     leaq    0 + pgtable(%rip), %rdi
//     leaq    0x1007(%rdi), %rax
//     leaq    0xff8(%rdi), %rdi
//     movq    %rax, (%rdi)

//     // Self reference
//     leaq    0 + pgtable(%rip), %rdi
//     leaq    0x0007(%rdi), %rax
//     leaq    0xff0(%rdi), %rdi
//     movq    %rax, (%rdi)

//     /* Build Level 3 */
//     leaq    0x1000 + pgtable(%rip), %rdi
//     movq    $0x00000183, %rax
//     movl    $511, %ecx
// 1:  movq    %rax, (%rdi)
//     addq    $(1 << 30), %rax
//     addq    $0x08, %rdi
//     decl    %ecx
//     jnz     1b

// //     /* Build Level 2 */
// //     leaq    0x2000 + pgtable(%rip), %rdi
// //     movq    $0x00000183, %rax
// //     movl    $1024, %ecx
// // 1:  movq    %rax, 0x00(%rdi)
// //     addq    $(1 << 21), %rax
// //     addq    $0x08, %rdi
// //     decl    %ecx
// //     jnz     1b

//     /* Set 64bit page tables */
//     leaq    pgtable(%rip), %rax
//     movq    %rax, %cr3

//     // Jmp to prepare_pgtable
//     movq    $KERNEL_BASE, %rax
//     leaq    prepare_pgtable(%rip), %rbx
//     addq    %rbx, %rax
    
//     jmpq    *%rax

// prepare_pgtable:
//     /* Prepare Level 4 */
//     leaq    pgtable(%rip), %rdi
//     movq    $0, %rax
//     movq    %rax, (%rdi)

//     /* flush tlb */
//     movq    %cr3, %rax
//     movq    %rax, %cr3

//     leaq    stack_top(%rip), %rax
//     movq    %rax, %rsp
    
//     // Boot params
//     leaq    boot_params(%rip), %rdi
    
//     // Kernel main
//     leaq    kernel_main(%rip), %rax
//     call    *%rax


//     .data
//     .balign 4096, 0
// stack_bottom:
//     .fill   16*4096, 1, 0   /* 64kb stack */
// stack_top:

//     .balign 4096, 0
// pgtable:
//     .fill   2*4096, 1, 0    /* 512g paging */

