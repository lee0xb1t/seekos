CR0_PE     = 1 << 0
CR0_PG     = 1 << 31
IA32_EFER_LME   = 1 << 8

LOSTART     = 0x7e00
PAGE_LEVEL1_ADDR     = 0x90000
PAGE_LEVEL2_ADDR     = 0x91000

.code16
.section ".lotext", "ax"
.extern loader_main
.global loader_entry, go_to_pm32_jmp
loader_entry:
    xorl %eax, %eax
    jmp loader_main

go_to_pm32_jmp:
    movl %cr0, %eax
    orl $CR0_PE, %eax     // enable PE
    movl %eax, %cr0
    
    ljmp $0x8, $loader32_entry


.code32
.extern loader32_main
.section ".lotext32", "ax"
.global loader32_entry, go_to_pm64_jmp
loader32_entry:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x0, %ax
    movw %ax, %fs
    movw %ax, %gs

    call loader32_main

go_to_pm64_jmp:
    pushl   %ebp
    movl    %esp, %ebp
    // sub     $0x100, %esp
    

    // 在 Intel 64 处理器上，系统重置时会清除 IA32_EFER MSR。在尝试初始化 IA-32e 模式之前，操作系统必须处于启用分页的保护模式下。IA-32e 模式操作还需要具备四级或五级增强分页结构的物理地址扩展（参见“4级分页和5级分页”一节）。
    // 操作系统应该按照以下顺序初始化 IA-32e 模式：
    // 1. 从保护模式开始，通过设置 CR0.PG = 0 来禁用分页。使用 MOV CR0 指令禁用分页（该指令必须位于一个身份映射的页面中）。
    // 2. 通过设置 CR4.PAE = 1 启用物理地址扩展（PAE）。如果未启用 PAE，则尝试初始化 IA-32e 模式时将导致 #GP 故障。
    // 3. 使用物理基地址加载 CR3 寄存器，加载 Level 4 页映射表（PML4）或 Level 5 页映射表（PML5）的物理基地址。
    // 4. 通过设置 IA32_EFER.LME = 1 来启用 IA-32e 模式。
    // 5. 通过设置 CR0.PG = 1 启用分页。这会导致处理器将 IA32_EFER.LMA 位设置为 1。启用分页的 MOV CR0 指令和随后的指令必须位于身份映射的页面中（直到可以实现到非身份映射页面的分支）。

    // 在激活 IA-32e 模式之前，64 位模式分页结构必须位于物理地址空间的前 4GB。这是必要的，因为在激活 IA-32e 模式之前（设置 CR0.PG = 1 启用分页），必须在传统模式下执行用于初始化页目录基址的 MOV CR3 指令。因为 MOV CR3 在保护模式下执行，所以只写入寄存器的低 32 位，将表位置限制在低 4GB 内存中。在激活 IA-32e 模式后，软件可以将页表重定位到物理内存的任何位置。
    // 当软件尝试直接修改与激活 IA-32e 模式相关的任何启用位（IA32_EFER.LME、CR0.PG 和 CR4.PAE）时，处理器会执行 64 位模式一致性检查。如果一致性检查失败，它将生成通用保护故障 (#GP)。64 位模式一致性检查确保处理器不会进入未定义的模式或状态，并具有不可预测的行为。
    // - 以下情况下会导致 64 位模式一致性检查失败：
    // - 尝试在启用分页时启用或禁用 IA-32e 模式。
    // - 在启用 IA-32e 模式并尝试启用物理地址扩展（PAE）之前，启用分页。
    // - 在 IA-32e 模式激活且尝试禁用物理地址扩展（PAE）时。
    // - 如果当前 CS 寄存器具有 L 位设置，并尝试激活 IA-32e 模式。
    // - 如果 TR 寄存器包含一个 16 位的 TSS，并尝试激活 IA-32e 模式。

    // Empty idt
    movl    $idt_pointer, %eax
    lidt    (%eax)

    // Clear TSS
    xorl    %eax, %eax
    ltr     %ax

    // Enable PAE & PGE
    movl    %cr4, %eax
    orl     $(1 << 5), %eax
    orl     $(1 << 7), %eax
    movl    %eax, %cr4


    /* Build Page Table */
    movl    $PAGE_LEVEL1_ADDR, %edi
    movl    $PAGE_LEVEL2_ADDR, %eax
    orl     $0x00000007, %eax
    movl    %eax, (%edi)

    movl    $PAGE_LEVEL2_ADDR, %edi
    movl    $0, %eax
    orl     $0x00000083, %eax
    movl    %eax, (%edi)

    /* Set 64bit page tables */
    movl    $PAGE_LEVEL1_ADDR, %eax
    movl    %eax, %cr3

    // Enable LME
    movl    $0xC0000080, %ecx  #IA32_EFER
    rdmsr
    orl     $IA32_EFER_LME, %eax
    wrmsr

    movl    0xc(%ebp), %eax
    pushl   %eax
    movl    8(%ebp), %eax

    // Enable PG
    movl    %cr0, %eax
    orl     $CR0_PG, %eax
    orl     $CR0_PE, %eax
    movl    %eax, %cr0

    movl    $gdt_pointer, %eax
    lgdt    (%eax)

    movl    0xc(%ebp), %ecx     # %ecx = kernel params
    movl    8(%ebp), %eax

    pushl   $0x8
    pushl   %eax
    lret

    // noreturn


    /* GDT */
    .align 8
gdt_arr:
    .quad   0x0000000000000000      # empty
    .quad   0x00A09B0000000000      # 64bit Kernel Code
    .quad   0x00a0930000000000      # 64bit Data Code
gdt_end:

    .align  16
gdt_pointer:
    .word   gdt_end - gdt_arr - 1
    .long   gdt_arr
    .long   0
gdt_pointer_end:


    /* IDT */
    /* empty */
    .align  16
idt_pointer:
    .word   0
    .quad   0
idt_pointer_end:
