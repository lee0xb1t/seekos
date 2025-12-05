#include <system/idt.h>
#include <system/gdt.h>
#include <ia32/taskstate.h>
#include <ia32/cpuinstr.h>
#include <ia32/interrupt.h>
#include <lib/kmemory.h>


idt_entry_t idt_entries[INTERRUPT_TABLE_MAX_ENTRIES];


//
// Interrupt Stubs
//
extern intr_stub_t intr_stubs[];


//
// Interrupt entries
//

extern void exc_divide_error(void);
extern void exc_debug(void);
extern void exc_nmi(void);
extern void exc_breakpoint(void);
extern void exc_overflow(void);
extern void exc_bounds(void);
extern void exc_undefined_opcode(void);
extern void exc_device_not_available(void);
extern void exc_double_fault(void);
extern void exc_coprocessor_segment_overrun(void);
extern void exc_invalid_tss(void);
extern void exc_segment_not_present(void);
extern void exc_stack_segment_fault(void);
extern void exc_general_protection(void);
extern void exc_page_fault(void);
extern void exc_x87_fpu_err(void);
extern void exc_alignment_check(void);
extern void exc_machine_check(void);
extern void exc_simd_exception(void);
extern void exc_virtualization_exception(void);
extern void exc_control_protection_exception(void);



//
// Functions
//

static inline void idt_entry_set(u8 vector, u16 selector, u8 ist, u8 attributes, void* isr) {
    uptr addr = (uptr)isr;
    idt_entries[vector].isr_low = addr & 0xffff;
    idt_entries[vector].isr_mid = (addr >> 16) & 0xffff;
    idt_entries[vector].isr_high = (addr >> 32) & 0xffffffff;
    idt_entries[vector].selector = selector;
    idt_entries[vector].ist = ist;
    idt_entries[vector].attributes = attributes;
}


static inline void idt_set_interrupt_gate(u8 vector, u8 ist, void* isr) {
    idt_entry_set(vector, KERNEL_CODE_SELECTOR, ist, IDT_ATTR_INTERRUPT_GATE, isr);
}

static inline void idt_set_trap_gate(u8 vector, u8 ist, void* isr) {
    idt_entry_set(vector, KERNEL_CODE_SELECTOR, ist, IDT_ATTR_TRAP_GATE, isr);
}

static inline void idt_set_trap_gate_dpl3(u8 vector, u8 ist, void* isr) {
    idt_entry_set(vector, KERNEL_CODE_SELECTOR, ist, IDT_ATTR_TRAP_GATE | IDT_ATTR_DPL3, isr);
}


void idt_init() {

    //
    // Initialize Interrupts
    //

    memset(idt_entries, 0, sizeof(idt_entry_t) * INTERRUPT_TABLE_MAX_ENTRIES);

    idt_set_trap_gate(0, 0, exc_divide_error);
    idt_set_trap_gate(1, 0, exc_debug);
    idt_set_trap_gate(2, 0, exc_nmi);
    idt_set_trap_gate_dpl3(3, 0, exc_breakpoint);
    idt_set_trap_gate(4, 0, exc_overflow);
    idt_set_trap_gate(5, 0, exc_bounds);
    idt_set_trap_gate(6, 0, exc_undefined_opcode);
    idt_set_trap_gate(7, 0, exc_device_not_available);
    idt_set_trap_gate(8, 0, exc_double_fault);
    idt_set_trap_gate(9, 0, exc_coprocessor_segment_overrun);
    idt_set_trap_gate(10, 0, exc_invalid_tss);
    idt_set_trap_gate(11, 0, exc_segment_not_present);
    idt_set_trap_gate(12, 0, exc_stack_segment_fault);
    idt_set_trap_gate(13, 0, exc_general_protection);
    idt_set_trap_gate(14, 0, exc_page_fault);

    // Interrupt 15 is Intel reversed. Do not use.

    idt_set_trap_gate(16, 0, exc_x87_fpu_err);
    idt_set_trap_gate(17, 0, exc_alignment_check);
    idt_set_trap_gate(18, 0, exc_machine_check);
    idt_set_trap_gate(19, 0, exc_simd_exception);
    idt_set_trap_gate(20, 0, exc_virtualization_exception);
    idt_set_trap_gate(21, 0, exc_control_protection_exception);


    for (int i = INTR_IRQ_START; i < INTERRUPT_TABLE_MAX_ENTRIES; i++) {
        intr_stub_t *intr_stub = intr_stubs + (i - INTR_IRQ_START);
        idt_set_interrupt_gate(i, 0, (void *)&intr_stub->start);
    }
}

void idt_load() {
    __lidt(idt_entries, INTERRUPT_TABLE_MAX_ENTRIES);
}

void idt_set_interrupt(int vector, interupt_handler_t h) {
    idt_set_interrupt_gate(vector, 0, (void *)h);
}

void idt_restore_interrupt(int vector) {
    intr_stub_t *intr_stub = intr_stubs + (vector - INTR_IRQ_START);
    idt_set_interrupt_gate(vector, 0, (void *)&intr_stub->start);
}
