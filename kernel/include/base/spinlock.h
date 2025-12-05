#ifndef __spinlock_h__
#define __spinlock_h__
#include <base/atomic.h>
#include <base/irqflags.h>
#include <lib/ktypes.h>
#include <lib/kutils.h>
#include <ia32/cpuinstr.h>


typedef struct {
    atomic_t a;
} spinlock_t;

#define DECLARE_SPINLOCK(name) \
    static volatile spinlock_t name = { .a.counter = 0 }


void spin_init(spinlock_t *lock);
bool spin_try_acquire(spinlock_t *lock);
void spin_acquire(spinlock_t *lock);
void spin_release(spinlock_t *lock);


static inline u64 __spin_lock_irqsave(spinlock_t *lock) {
    u64 flags = local_irq_save();
    spin_acquire(lock);
    return flags;
}

static inline void __spin_unlock_irqrestore(spinlock_t *lock, u64 flags) {
    spin_release(lock);
    local_irq_restore(flags);
}

#define spin_lock_irq(lock, flags) \
    do { \
        flags = __spin_lock_irqsave(lock); \
    } while(0)

#define spin_unlock_irq(lock, flags) \
    do { \
        __spin_unlock_irqrestore(lock, flags); \
    } while(0)

    
#define spin_lock spin_acquire
#define spin_unlock spin_release


#endif