#ifndef __atomic_h__
#define __atomic_h__

typedef struct {
    volatile int counter;
} atomic_t;
#define ATOMIC_INIT(i)  { .counter=i }
#define DECLARE_ATOMIC(name, i) static volatile atomic_t name = ATOMIC_INIT(i)


// int atomic_read(const atomic_t *ptr);
// void atomic_set(atomic_t *ptr, int val);
void atomic_inc(atomic_t *v);
void atomic_dec(atomic_t *v);
int atomic_get(atomic_t *v);
void atomic_set(int i, atomic_t *v);
int atomic_cmpxchg(atomic_t *ptr, int oldval, int newval);

#endif //__atomic_h__