#ifndef __CPU_H
#define __CPU_H
#include <lib/ktypes.h>
#include <ia32/ia32.h>
#include <ia32/taskstate.h>
#include <ia32/segment.h>
#include <ia32/interrupt.h>


#define MAX_CPUS        256


typedef struct _cpu_ctrl_t {
    struct _cpu_ctrl_t *self;
    bool is_bsp;
    u8 __PADDING0;
    u16 id;
    u32 __PADDING1;
    tss64_t *tss;
    // idt_entry_t *idt;
    void *kstack;
    void *ustack;
    gdt_entry_t *gdt_entries;
} cpu_ctrl_t;


#define percpu_container_of(member) ( (int)(&((cpu_ctrl_t *)0)->member) )

static inline u64 percpu_u64(int offset_of) {
    u64 val = 0;
    asm volatile(
        "movq %%gs:%1, %0 \n"
        : "=r" (val)
        : "m" (*(u16 *)(offset_of))
    );
    return val;
}

static inline u16 percpu_u16(int offset_of) {
    u16 val = 0;
    asm volatile(
        "movw %%gs:%1, %0 \n"
        : "=r" (val)
        : "m" (*(u16 *)(offset_of))
    );
    return val;
}

static inline cpu_ctrl_t *percpu_self() {
    cpu_ctrl_t * val = 0;
    int offset_of = 0;
    asm volatile(
        "movq %%gs:%1, %0 \n"
        : "=r" (val)
        : "m" (*(u16 *)(offset_of))
    );
    return val;
}

#define percpu_u64_of(member) percpu_u64( percpu_container_of(member) )
#define percpu_u16_of(member) percpu_u16( percpu_container_of(member) )

#define percpu() percpu_self()


cpu_ctrl_t *cpu_init(int cpuno);
void cpu_gs_init(cpu_ctrl_t *);

u16 num_cpus();

void cpu_feature_init();

#endif//__CPU_H