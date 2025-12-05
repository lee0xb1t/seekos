#ifndef __TIMER_H
#define __TIMER_H
#include <lib/ktypes.h>


#define APIC_TIMER_IRQ       222


typedef enum {
    TIMER_ONESHOT = 0,
    TIMER_PERIODIC,
    TIMER_TSC_DEADLINE,
}apic_timer_mode_t;


void apic_timer_init();

void apic_timer_stop();
void apic_timer_start();
void apic_timer_set_mode(int mode);
void apic_timer_set_period_ms(u64 millis);
void apic_timer_set_handler(void (*h)(void *));

#endif