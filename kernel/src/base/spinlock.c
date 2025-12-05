#include <base/spinlock.h>
#include <base/atomic.h>
#include <lib/kutils.h>
#include <ia32/cpuinstr.h>


inline void spin_init(spinlock_t *lock) {
    lock->a.counter = 0;
}

inline bool spin_try_acquire(spinlock_t *lock) {
    int oldval = atomic_cmpxchg(&lock->a, 0, 1);
    return oldval == 0;
}

inline void spin_acquire(spinlock_t *lock) {
    while(!spin_try_acquire(lock))
        __pause();
}

inline void spin_release(spinlock_t *lock) {
    __barrier();
    atomic_set(0, &lock->a);
}
