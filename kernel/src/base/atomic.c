#include <base/atomic.h>
#include <lib/kutils.h>


// int atomic_read(const atomic_t *ptr) {
//     int res = 0;
//     asm volatile (
//         "movl %1, %0"
//         : "=r" (res)
//         : "m" (ptr)
//     );
//     return res;
// }

// void atomic_set(atomic_t *ptr, int val) {
//     asm volatile (
//         "movl %1, %0"
//         : 
//         : "m" (ptr), "r" (val)
//     );
// }

inline void atomic_inc(atomic_t *v) {
    asm volatile(
        "lock incl %0"
        : "+m"(v->counter)
        :
        : "memory"
    );
}

inline void atomic_dec(atomic_t *v) {
    asm volatile(
        "lock decl %0"
        : "+m"(v->counter)
        :
        : "memory"
    );
}

inline int atomic_get(atomic_t *v) {
    asm volatile("mfence");
    return v->counter;
}

inline void atomic_set(int i, atomic_t *v) {
    v->counter = i;
    asm volatile("mfence");
}

inline int atomic_cmpxchg(atomic_t *ptr, int oldval, int newval) {
    int prev = 0;
    asm volatile(
        "lock cmpxchgl %3, %1;"
        : "=a" (prev), "+m" (ptr->counter)
        : "0" (oldval), "r" (newval)
        : "cc", "memory"
    );
    return prev;
}