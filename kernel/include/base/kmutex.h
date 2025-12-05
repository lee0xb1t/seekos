#ifndef KMUTEX_H
#define KMUTEX_H
#include <base/spinlock.h>
#include <proc/sched.h>


typedef struct {
    spinlock_t s;
} kmutex_t;

#define DECLARE_MUTEX(name) \
    static volatile kmutex_t name = { .s.a.counter = 0 }


void kmutex_init(kmutex_t *);
void kmutex_acquire(kmutex_t *m);
void kmutex_release(kmutex_t *m);

#define kmutex_lock(m)   \
    kmutex_acquire(m)

#define kmutex_unlock(m)   \
    kmutex_release(m)


#endif