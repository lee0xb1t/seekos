#ifndef __SMP_H
#define __SMP_H
#include <lib/ktypes.h>


typedef struct {
    u16  __donotuse;
    u64 long_idt_ptr;
    u64 long_stack_ptr;
    u64 long_cr3_val;
    u64 long_entry_ptr;
    u64 long_cpu_ctrl;
} __attribute__((packed)) ap_args_t;

void smp_init();

#endif