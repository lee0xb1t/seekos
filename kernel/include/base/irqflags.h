#ifndef __IRQFLAGS_H
#define __IRQFLAGS_H
#include <lib/ktypes.h>


static inline u64 local_irq_save() {
    u64 flags;
    asm volatile(
        "pushfq;"
        "popq %0;"
        "cli;"
        : "=rm" (flags)
        :
        : "memory"
    );
    return flags;
}

static inline void local_irq_restore(u64 flags) {
    asm volatile(
        "pushq %0;"
        "popfq;"
        :
        : "rm" (flags)
        : "memory"
    );
}

#endif