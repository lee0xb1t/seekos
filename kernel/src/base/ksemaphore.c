#include <base/ksemaphore.h>
#include <proc/sched.h>


void ksemapore_init(ksemaphore_t *s, int new, int max) {
    spin_init(&s->lock);
    s->counter = new;
    s->max = max;
    s->waiters = 0;
    s->wakeups = 0;
}

void ksemapore_acquire(ksemaphore_t *s) {
    spin_lock(&s->lock);
    if (s->counter > 0) {
        s->counter--;
        spin_unlock(&s->lock);
        return;
    }

    s->waiters++;

    while (s->wakeups == 0) {
        spin_unlock(&s->lock);
        // Its bad implmention
        // TODO cond wait
        sched_sleep(0);
        spin_lock(&s->lock);
    }

    s->waiters--;
    s->wakeups--;

    spin_unlock(&s->lock);
}

void ksemapore_release(ksemaphore_t *s) {
    spin_lock(&s->lock);
    if (s->counter < s->max) {
        s->counter++;
    }

    if (s->waiters > 0) {
        s->wakeups++;
    }

    spin_unlock(&s->lock);
}
