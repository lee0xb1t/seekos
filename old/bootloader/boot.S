.code16

BOOTSEG     = 0
STACKSEG    = 0
BOOTSTART   = 0x7c00
STACKSTART  = BOOTSTART
LOSTART     = 0x7e00

NUMFORLOADER = 0x80

.extern loader_entry

.section ".boottext", "ax"
.global boot_start

boot_start:
    ljmp $BOOTSEG, $boot_start2
boot_start2:
    movw	%cs, %ax
    movw	%ax, %ds
    movw	%ax, %es
    movw	%cs, %ax
    mov     $STACKSEG, %ax
    mov     %ax, %ss
    movw    $STACKSTART, %sp

    movw    $boot_msg, %cx
    call    tty_puts

    # read loader in sector 2
    mov     $BOOTSEG, %ax
    mov     %ax, %es
    mov     $LOSTART, %bx

    mov     $0x0200, %ax    /* al = NumberForRead */
    movb    $NUMFORLOADER, %al
    mov     $0x0002, %cx    /* ch = cylinder number  cl = sector number */
    mov     $0x0080, %dx    /* dh = head number  dl = drive number */
    int     $0x13
    jc      load_disk_error
    jmp     loader_entry


load_disk_error:
    mov $load_disk_error_msg, %cx
    call tty_puts
    jmp .

    
tty_puts:
    movw %cx, %si
tty_puts_putchar:
    lodsb
    test %al, %al
    jz tty_puts_ret
    movb $0x0e, %ah  
    movw $0x0007, %bx
    int $0x10
    jmp tty_puts_putchar
tty_puts_ret:
    ret


.section ".bootdata", "a"
boot_msg:
    .ascii "OS is booting...\r\n"
    .byte 0x0
    boot_msg_len = . - boot_msg

load_disk_error_msg:
    .ascii "Read loader from sector error...\r\n"
    .byte 0x0
    load_disk_error_msg_len = . - load_disk_error_msg

.section ".boottail", "a"
.word 0xAA55
