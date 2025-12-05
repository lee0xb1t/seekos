#ifndef KSEMAPHORE_H
#define KSEMAPHORE_H
#include <base/spinlock.h>


typedef struct {
    spinlock_t lock;
    int counter;
    int max;
    int waiters;
    int wakeups;
} ksemaphore_t;

#define DECLARE_SEMP(name, c, m)  \
    static volatile ksemaphore_t name = {   \
        .lock.a.counter = 0,                \
        .counter = c,       \
        .max = m,       \
        .waiters = 0,       \
        .wakeups = 0,       \
    }


void ksemapore_init(ksemaphore_t *s, int new, int max);
void ksemapore_acquire(ksemaphore_t *s);
void ksemapore_release(ksemaphore_t *s);

#define ksemp_init(s) ksemapore_init(s)
#define ksemp_acquire(s) ksemapore_acquire(s)
#define ksemp_release(s) ksemapore_acquire(s)

#endif