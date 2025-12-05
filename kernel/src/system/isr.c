#include <system/isr.h>
#include <system/idt.h>
#include <system/gdt.h>
#include <system/cpu.h>
#include <system/madt.h>
#include <system/apic.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <base/spinlock.h>
#include <panic/panic.h>
#include <ia32/cpuinstr.h>
#include <log/klog.h>


static volatile isr_register_t isr_registers[NUM_VECTORS];
DECLARE_SPINLOCK(isr_registers_lock);

static volatile u8 *irq_bitmap = null;
static volatile int irq_external_max = 0;
DECLARE_SPINLOCK(irq_lock);
// static volatile int ioapic_count = 0;

static volatile isr_isa_override isr_isa_overrides[NUM_ISA_IRQS];


void default_exception_handler(trapframe_t *trapframe);
void unhandled_interrupt_handler(trapframe_t *trapframe);

void _isr_init_override();
void _isr_reserve_isa_irq();

void _irq_bitmap_set(int irqno);
void _irq_bitmap_clear(int irqno);
bool _irq_bitmap_is_set(int irqno);
int _irq_bitmap_find_first_clear(int start, int end);


void isr_init0() {
    for (int i = 0; i < NUM_VECTORS; i++) {
        if (i >= 0 && i < NUM_EXCEPTIONS) {
            isr_registers[i].used = false;
            isr_registers[i].isr_handler = default_exception_handler;
        } else {
            isr_registers[i].used = false;
            isr_registers[i].isr_handler = unhandled_interrupt_handler;
        }
    }
}

void isr_init() {
    irq_bitmap = kzalloc(NUM_VECTORS / 8);
    irq_external_max = ioapic_get_max_irq();

    _isr_reserve_isa_irq();
    _isr_init_override();


    // for (int i = IRQ_0; i < IRQ_MAX + 1; i++) {
    //     isr_bitmap_set(i);
    // }

    // for (int i = ISR_KERNEL_FIXED; i < NUM_VECTORS; i++) {
    //     isr_bitmap_set(i);
    // }

    klogi("ISR Initialized.\n");
}

void _isr_init_override() {
    int mun_ioapic_iso = madt_get_num_ioapic_iso();
    for (int i = 0; i < mun_ioapic_iso; i++) {
        madt_record_iso_t *iso = madt_get_ioapic_iso(i);

        isr_isa_overrides[iso->source].used = true;
        isr_isa_overrides[iso->source].dest = iso->gsi;
        isr_isa_overrides[iso->source].flags = iso->flags;
    }
}

void isr_register_handler(int vector, isr_handler_t isr_handler, isr_type_t isr_type) {
    u64 flags;
    spin_lock_irq(&isr_registers_lock, flags);

    if (isr_registers[vector].used) {
        panic("isr: isr handler(%d) is already used!", vector);
    }

    isr_registers[vector].used = true;
    isr_registers[vector].isr_handler = isr_handler;

    spin_unlock_irq(&isr_registers_lock, flags);
}

void isr_unregister_handler(int vector, isr_handler_t isr_handler) {
    u64 flags;
    spin_lock_irq(&isr_registers_lock, flags);

    if (isr_registers[vector].used) {
        if (vector >= 0 && vector < NUM_EXCEPTIONS) {
            isr_registers[vector].isr_handler = default_exception_handler;
        } else {
            isr_registers[vector].isr_handler = unhandled_interrupt_handler;
        }
    }

    spin_unlock_irq(&isr_registers_lock, flags);
}

void isr_set_handler_type(int vector, isr_type_t type) {
    u64 flags;
    spin_lock_irq(&isr_registers_lock, flags);
    isr_registers[vector].isr_type = type;
    spin_unlock_irq(&isr_registers_lock, flags);
}

void isr_set_handler(int vector, isr_handler_t isr_handler) {
    u64 flags;
    spin_lock_irq(&isr_registers_lock, flags);
    isr_registers[vector].isr_handler = isr_handler;
    spin_unlock_irq(&isr_registers_lock, flags);
}

void exc_common_handler(trapframe_t* trap_frame) {
    u64 flags;

    if (trap_frame->cs == USER_CODE_SELECTOR)
        asm volatile("swapgs");

    spin_lock_irq(&isr_registers_lock, flags);

    isr_register_t *isr_reg = &isr_registers[trap_frame->trapno];
    
    spin_unlock_irq(&isr_registers_lock, flags);

    isr_reg->isr_handler(trap_frame);

    if (trap_frame->cs == USER_CODE_SELECTOR)
        asm volatile("swapgs");
}

void intr_common_handler(trapframe_t* trap_frame) {
    u64 flags;
    
    if (trap_frame->cs == USER_CODE_SELECTOR)
        asm volatile("swapgs");

    spin_lock_irq(&isr_registers_lock, flags);

    isr_register_t *isr_reg = &isr_registers[trap_frame->trapno];
    
    spin_unlock_irq(&isr_registers_lock, flags);
    
    isr_reg->isr_handler(trap_frame);

    if (trap_frame->cs == USER_CODE_SELECTOR)
        asm volatile("swapgs");
}


void default_exception_handler(trapframe_t *trapframe) {
    
    panic("!!!EXCEPTION!!! %d\n"
          "  CPU #%d - %p\n"
          "  rip = %p  rsp = %p\n"
          "  rflags = %p\n"
        , trapframe->trapno
        , percpu_u16_of(id), trapframe->errcode
        , trapframe->rip, trapframe->rsp
        , __rflags()
    );
}

void unhandled_interrupt_handler(trapframe_t *trapframe) {
    panic(
        "!!!UNHANDLED INTRRRUPT!!! %d"
        "  CPU #%d\n"
        "  rip = %p  rsp = %p\n"
        , trapframe->trapno
        , percpu_u16_of(id)
        , trapframe->rip, trapframe->rsp
    );
}

void _irq_bitmap_set(int irqno) {
    int idx = irqno / 8;
    int offset = irqno % 8;
    irq_bitmap[idx] &= 1 << offset;
}

void _irq_bitmap_clear(int irqno) {
    int idx = irqno / 8;
    int offset = irqno % 8;
    irq_bitmap[idx] &= ~(1 << offset);
}

bool _irq_bitmap_is_set(int irqno) {
    int idx = irqno / 8;
    int offset = irqno % 8;
    if (irq_bitmap[idx] & (1 << offset)) {
        return true;
    }
    return false;
}

int _irq_bitmap_find_first_clear(int start, int end) {
    for (int i = start; i < end; i++) {
        int idx = i / 8;
        int offset = i % 8;
        if (!(irq_bitmap[idx] & (1 << offset))) {
            return i;
        }
    }
    return -1;
}

void isr_register_irq_h(int irqno, isr_handler_t isr_handler) {
    if (irqno >= NUM_INTERRUPTS) {
        panic("[ISR] irq is not a interrupt");
    }

    int vector = isr_setup_irq(irqno);
    int type = isr_is_softirq(irqno) ? ISR_TYPE_SOFTIRQ : ISR_TYPE_HARDIRQ;
    isr_register_handler(vector, isr_handler, type);
    if (!isr_is_softirq(irqno)) {
        isr_enable_irq(irqno);
    }
}

void isr_register_irq(int irqno) {
    if (irqno >= NUM_INTERRUPTS) {
        panic("[ISR] irq is not a interrupt");
    }

    int vector = isr_setup_irq(irqno);
    int type = isr_is_softirq(irqno) ? ISR_TYPE_SOFTIRQ : ISR_TYPE_HARDIRQ;
    if (!isr_is_softirq(irqno)) {
        isr_enable_irq(irqno);
    }
}

void isr_unregister_irq(int irqno, isr_handler_t isr_handler) {
    // TODO
}

void isr_setup_irq_handler(int irqno, isr_handler_t isr_handler) {
    int vector = irq_to_vector(irqno);
    int type = isr_is_softirq(irqno) ? ISR_TYPE_SOFTIRQ : ISR_TYPE_HARDIRQ;
    isr_register_handler(vector, isr_handler, type);
}

void isr_alloc_softirq() {
    u64 flags;
    spin_lock_irq(&irq_lock, flags);
    int irqno =_irq_bitmap_find_first_clear(irq_external_max, NUM_INTERRUPTS - 1);
    if (irqno == -1) {
        panic("[isr] connot alloc softirq\n");
    }
    spin_unlock_irq(&irq_lock, flags);
    return irqno;
}

void isr_alloc_hardirq() {
    u64 flags;
    spin_lock_irq(&irq_lock, flags);
    int irqno =_irq_bitmap_find_first_clear(0, irq_external_max);
    if (irqno == -1) {
        panic("[isr] connot alloc hardirq\n");
    }
    spin_unlock_irq(&irq_lock, flags);
    return irqno;
}

void _isr_reserve_isa_irq() {
    for (int i = IRQ0; i < NUM_ISA_IRQS; i++) {
        _irq_bitmap_set(i);
    }
}

void isr_reserve_irq(int irqno) {
    if (_irq_bitmap_is_set(irqno)) {
        panic("[ISR] irq is already used.\n");
    }
    _irq_bitmap_set(irqno);
}

int isr_setup_irq(int irqno) {
    u64 flags;
    int vector = irq_to_vector(irqno);

    spin_lock_irq(&irq_lock, flags);

    if (irqno <= irq_external_max) {
        if (irqno < NUM_ISA_IRQS && isr_isa_overrides[irqno].used) {
            irqno = isr_isa_overrides[irqno].dest;
            vector = irq_to_vector(irqno);
            ioapic_set_isa_irq_flags(irqno, vector, isr_isa_overrides[irqno].flags);
        } else {
            ioapic_set_irq_vector(irqno, vector);
        }
    }

    spin_unlock_irq(&irq_lock, flags);
    return vector;
}

void isr_remove_irq(int irqno) {
    // lock
    // TODO
}

bool isr_is_softirq(int irqno) {
    if (irqno > irq_external_max && irqno < NUM_INTERRUPTS) {
        return true;
    }
    return false;
}

int isr_get_isa_irq_override(int irqno) {
    if (irqno >= NUM_ISA_IRQS) {
        panic("[ISR] It is not isa irq.");
    }
    return isr_isa_overrides[irqno].dest;
}

void isr_enable_irq(int irqno) {
    u64 flags;
    spin_lock_irq(&irq_lock, flags);
    ioapic_set_irq_mask(irqno, 0);
    spin_unlock_irq(&irq_lock, flags);
}

void isr_disbale_irq(int irqno) {
    u64 flags;
    spin_lock_irq(&irq_lock, flags);
    ioapic_set_irq_mask(irqno, 1);
    spin_unlock_irq(&irq_lock, flags);
}
