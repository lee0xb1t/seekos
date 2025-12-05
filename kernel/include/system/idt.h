#ifndef __idt_h__
#define __idt_h__

#include <lib/ktypes.h>


#define INTERRUPT_TABLE_MAX_ENTRIES      256

#define INTR_IRQ_START                   32


#define IDT_ATTR_PRESENT        (1 << 7)
#define IDT_ATTR_DPL0           (0 << 5)
#define IDT_ATTR_DPL3           (3 << 5)
#define IDT_ATTR_TYPE_TRAP      (0xf)
#define IDT_ATTR_TYPE_INTERRUPT (0xe)

#define IDT_ATTR_INTERRUPT_GATE (IDT_ATTR_PRESENT | IDT_ATTR_TYPE_INTERRUPT)
#define IDT_ATTR_TRAP_GATE      (IDT_ATTR_PRESENT | IDT_ATTR_TYPE_TRAP)


typedef struct _intr_stub_t {
    u8 start[5];
} intr_stub_t;

typedef void (*interupt_handler_t)();


void idt_init();
void idt_load();

void idt_set_interrupt(int vector, interupt_handler_t h);
void idt_restore_interrupt(int vector);


#endif //__idt_h__