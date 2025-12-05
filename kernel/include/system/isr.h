#ifndef __ISR_H
#define __ISR_H
#include <lib/ktypes.h>


#define IRQ0                0
#define NUM_ISA_IRQS        16

#define NUM_EXCEPTIONS      32
#define NUM_INTERRUPTS      224
#define NUM_VECTORS         256

#define NUM_IRQS_PER_IOAPIC 24

#define ISR_KERNEL_FIXED    240


typedef struct {
    u64 trapno;
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    u64 rdi;
    u64 rsi;
    u64 rbp;
    u64 rbx;
    u64 rdx;
    u64 rcx;
    u64 rax;
    u64 errcode;
    u64 rip;
    u16 cs;
    u16 _padding2;
    u32 _padding3;
    u64 rflags;
    u64 rsp;
    u16 ss;
    u16 _padding0;
    u32 _padding1;
} __attribute__((packed)) trapframe_t;


typedef struct _isr_isa_override {
    bool used;
    int dest;
    u16 flags;
} isr_isa_override;

typedef enum _isr_type_t {
    ISR_TYPE_UNKNOWN = 0,
    ISR_TYPE_ISA_IRQ = 1,
    ISR_TYPE_LAPIC_TIMER = 2,
    ISR_TYPE_SOFTIRQ,
    ISR_TYPE_HARDIRQ,
} isr_type_t;

typedef void (*isr_handler_t)(trapframe_t *);

typedef struct _isr_register_t {
    isr_type_t isr_type;
    isr_handler_t isr_handler;
    int vector;
    bool used;
} isr_register_t;


#define isr_enable_interrupts     __sti
#define isr_disable_interrupts    __cli

#define irq_to_vector(irq)          ((irq) + NUM_EXCEPTIONS)
#define vector_to_irq(vector)       ((vector) - NUM_EXCEPTIONS)



void isr_init0();
void isr_init();

void isr_register_handler(int vector, isr_handler_t isr_handler, isr_type_t isr_type);
void isr_unregister_handler(int vector, isr_handler_t isr_handler);
void isr_set_handler_type(int irqno, isr_type_t type);
void isr_set_handler(int irqno, isr_handler_t isr_handler);

// void isr_set_interrupt(int vector, isr_handler_t isr_handler, isr_type_t isr_type);
// void isr_clear_interrupt(int vector, isr_handler_t isr_handler);

void exc_common_handler(trapframe_t* trap_frame);
void intr_common_handler(trapframe_t* trap_frame);

int isr_setup_irq(int irqno);
void isr_remove_irq(int irqno);
void isr_reserve_irq(int irqno);
void isr_alloc_softirq();
void isr_alloc_hardirq();
bool isr_is_softirq(int irqno);
int isr_get_isa_irq_override(int irqno);
void isr_enable_irq(int irqno);
void isr_disbale_irq(int irqno);

void isr_register_irq_h(int irqno, isr_handler_t isr_handler);
void isr_register_irq(int irqno);
void isr_unregister_irq(int irqno, isr_handler_t isr_handler);
void isr_setup_irq_handler(int irqno, isr_handler_t isr_handler);


#endif