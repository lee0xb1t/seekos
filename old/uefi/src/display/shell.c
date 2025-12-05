#include <display/shell.h>
#include <display/gui.h>
#include <osloader/loader.h>
#include <acpi.h>
#include <pointer.h>
#include <xlib.h>
#include <apic.h>
#include <hpet.h>


#define MAX_COMMAND_LENGTH  255

extern EFI_SYSTEM_TABLE *gST;

void shell_init() {
    clear_screen();
    
}

void print_regs() {
    uint64_t cr3 = 0;
    uint64_t rsp = 0;
    uint64_t rip = 0;

    __asm__ __volatile__ (
        "movq    %%cr3, %%rax  ;"
        "movq    %%rax, %[cr3] ;"
        "movq    %%rsp, %%rax  ;"
        "movq    %%rax, %[rsp] ;"
        "leaq    0(%%rip), %%rax;"
        "movq    %%rax, %[rip] ;"
        : [cr3] "=m" (cr3), [rsp] "=m" (rsp), [rip] "=c" (rip)
    );

    printv("cr3: %p\r\n", cr3);
    printv("rsp: %p\r\n", rsp);
    printv("rip: %p\r\n", rip);
}


void shell() {
    char buffer[MAX_COMMAND_LENGTH] = { 0 };

    LoadOs();

    while(1) {
        memzero(buffer, MAX_COMMAND_LENGTH);

        print_string("xbit_efi> ");
        if (gets(buffer, MAX_COMMAND_LENGTH) > 0) {
            if (strcmp(buffer, "hello") == 0) {
                printv("Hello UEFI!\r\n");
            } else if (strcmp(buffer, "gui") == 0) {
                gui_init();
            } else if (strcmp(buffer, "pstat") == 0) {
                pstat();
            } else if (strcmp(buffer, "quit") == 0) {
                break;
            } else if (strcmp(buffer, "loados") == 0) {
                LoadOs();
            } else if (strcmp(buffer, "regs") == 0) {
                print_regs();
            } else if (strcmp(buffer, "apic") == 0) {
                print_madt(null);
            } else if (strcmp(buffer, "hpet") == 0) {
                hpet_init();
                print_hpet();
            }
        }

        printv("\r\n");
    }
}
