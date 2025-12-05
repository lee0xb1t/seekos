#include <system/cpu.h>
#include <ia32/cpuinstr.h>
#include <ia32/msr.h>
#include <log/klog.h>
#include <system/apic.h>
#include <system/madt.h>
#include <system/gdt.h>
#include <panic/panic.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <lib/kmemory.h>


static cpu_ctrl_t *cpu_ctrls[MAX_CPUS] = {null};
static int num_cpu_ctrls = 0;

static cpu_ctrl_t bsp_cpu_ctrls = {0};

extern tss64_t bsp_tss;
// extern gdt_entry_t *gdt_entries;
// extern idt_entry_t *idt_entries;


cpu_ctrl_t *cpu_init(int cpuno) {
    if (cpuno == 0) {
        memset(cpu_ctrls, 0, MAX_CPUS * sizeof(cpu_ctrl_t *));
        memset(&bsp_cpu_ctrls, 0, sizeof(cpu_ctrl_t));
        cpu_ctrls[cpuno] = &bsp_cpu_ctrls;
        cpu_ctrls[cpuno]->is_bsp = true;
    } else {
        /*Allocate a cpu ctrl*/
        cpu_ctrls[cpuno] = (cpu_ctrl_t *)kzalloc(sizeof(cpu_ctrl_t));
        cpu_ctrls[cpuno]->gdt_entries = kzalloc(sizeof(gdt_entry_t) * MAX_GDT_ENTRIES);
        cpu_ctrls[cpuno]->tss = kzalloc(sizeof(tss64_t));
        cpu_ctrls[cpuno]->tss->ist1 = (uptr)kzalloc(0x2000);
        cpu_ctrls[cpuno]->tss->ist2 = (uptr)kzalloc(0x2000);
    }

    cpu_ctrls[cpuno]->self = &cpu_ctrls[cpuno]->self;
    cpu_ctrls[cpuno]->id = cpuno;

    return cpu_ctrls[cpuno];
}

void cpu_gs_init(cpu_ctrl_t *cpu_ctrl) {
    write_msr(IA32_GS_BASE_MSR, (u64)cpu_ctrl);
    write_msr(IA32_KERNEL_GS_BASE, 0);
}

u16 num_cpus() {
    return madt_get_num_lapic();
}

void cpu_feature_init() {

    /*PAE PG*/

    /*XMM*/

    /*XSAVE*/

    /*TSC detect
        
    */

    /*rdtsc and rdtscp*/

    /*APIC detect*/

}
