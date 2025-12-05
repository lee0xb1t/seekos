#include <base/kmutex.h>


void kmutex_init(kmutex_t *m) {
    spin_init(&m->s);
}

inline void kmutex_acquire(kmutex_t *m) {
    // TODO
    // 可重入
    // fifo队列
    while (!spin_try_acquire(&m->s)) {
        sched_sleep(0);
    }
}

inline void kmutex_release(kmutex_t *m) {
    spin_release(&m->s);
}
