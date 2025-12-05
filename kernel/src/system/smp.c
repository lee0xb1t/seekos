#include <system/smp.h>
#include <system/apic.h>
#include <system/hpet.h>
#include <system/cpu.h>
#include <system/gdt.h>
#include <system/idt.h>
#include <proc/syscall.h>
#include <proc/sched.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <lib/kmemory.h>
#include <ia32/cpuinstr.h>
#include <log/klog.h>
#include <panic/panic.h>


extern u8 _binary_ap_trampoline_bin_start[], _binary_ap_trampoline_bin_end[];
extern u8 _binary_ap_trampoline_bin_size[];

DECLARE_ATOMIC(ap_count, 0);


void smp_ap_entry();


void smp_init() {
    uptr apentry_phys = MI_SMP_APENTRY_PHYS_START;
    klogd("[SMP] apentry_phys: %p\n", apentry_phys);

    atomic_inc(&ap_count);
    
    void *apentry_bakup = kzalloc(PAGE_SIZE);
    vmm_map(apentry_phys, apentry_phys, 1, VMM_FLAGS_DEFAULT);

    memcpy(apentry_bakup, apentry_phys, PAGE_SIZE);

    volatile void *ap_entry = (void *)apentry_phys;

    memset(ap_entry, 0, PAGE_SIZE);
    memcpy(ap_entry, _binary_ap_trampoline_bin_start, (size_t)_binary_ap_trampoline_bin_size);

    void *ap_stack = vmalloc(null, null, PAGE_SIZE, VMM_FLAGS_DEFAULT);
    u64 ap_stack_bottom = (u64)ap_stack + PAGE_SIZE;

    cpu_ctrl_t *ap_ctrl = cpu_init(atomic_get(&ap_count));

    ap_args_t *ap_args = (ap_args_t *)ap_entry;
    ap_args->long_idt_ptr = 0;
    ap_args->long_stack_ptr = ap_stack_bottom;
    ap_args->long_cr3_val = __readcr3();
    ap_args->long_entry_ptr = (u64)&smp_ap_entry;
    ap_args->long_cpu_ctrl = (uptr)ap_ctrl;

    apic_send_ipi(1, 0, APIC_ICR_DELIVERY_INIT);
    hpet_sleep_millis(20);

    for (int i = 0; i < 2; i++) {
        apic_send_ipi(1, ptr_to_pfn(apentry_phys), APIC_ICR_DELIVERY_STARTUP);
        hpet_sleep_millis(20);
    }

    int expect = num_cpus();
    for (;;) {
        if (atomic_get(&ap_count) == expect) {
            hpet_sleep_millis(500);
            break;       
        }
    }

    // memcpy(apentry_phys, apentry_bakup, PAGE_SIZE);
    // vmm_unmap(ap_entry, 1);
    // kfree(apentry_bakup);
    // kfree(ap_stack);

    // klogd("smp: apic_reg_read(0x280): %p\n", apic_reg_read(0x280));
    klogi("SMP Initialized.\n");
}

void smp_ap_entry(cpu_ctrl_t *cpu_ctrl) {
    klogi("!!!smp_ap_entry!!!\n");

    cpu_feature_init();

    cpu_gs_init(cpu_ctrl);
    gdt_init(cpu_ctrl);
    idt_load();
    apic_load();

    syscall_init();

    sched_init();
    
    atomic_inc(&ap_count);

    asm volatile("sti");
    __hang();
}
