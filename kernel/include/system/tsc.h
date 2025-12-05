#ifndef __TSC_H
#define __TSC_H
#include <lib/ktypes.h>


void tsc_init();

void tsc_sleep_millis(u64 millis);
u64 tsc_get_nanos();
u64 tsc_get_millis();

#endif