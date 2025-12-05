#include <system/gdt.h>
#include <ia32/segment.h>
#include <ia32/cpuinstr.h>
#include <ia32/taskstate.h>
#include <lib/kmemory.h>


// See `gdt_support.S`
extern void _flush_gdt(void);

//
// Kernel Stack
//

extern uptr kernel_stack_of_tss;
extern uptr kernel_stack_of_ist1;
extern uptr kernel_stack_of_ist2;

tss64_t bsp_tss = {0};


//
// 0    is Intel Reverse
// 1-6  is Code and Data segment
// 7-8  is Interrupt Stack of TSS
//
gdt_entry_t bsp_gdt_entries[MAX_GDT_ENTRIES];


void gdt_entry_set(gdt_entry_t *gdt_entries, u16 selector, u32 base, u32 limit, u8 access, u8 flags) {
    int index = selector >> 3;
    gdt_entry_t *gdt_entry = &(gdt_entries[index]);

    gdt_entry->limit_low = limit & 0xffff;
    gdt_entry->base_low = base & 0xffff;
    gdt_entry->base_middle = (base >> 16) & 0xffff;
    gdt_entry->base_high = (base >> 24) & 0xff;

    u8 limit_high = (limit >> 16) & 0xf;
    u16 attr_flags = ((u16)flags << 4) | limit_high;
    attr_flags = (attr_flags << 8) | access;
    gdt_entry->attr.flags = attr_flags;
}

void gdt_sysentry_set(gdt_entry_t *gdt_entries, u16 selector, uptr base, u32 limit, u8 access, u8 flags) {
    gdt_entry_set(gdt_entries, selector, base, limit, access, flags);
    int index = selector >> 3;
    gdt_sysentry_t *gdt_entry = (gdt_sysentry_t *)&(gdt_entries[index]);
    gdt_entry->base_high32 = (u32)(base >> 32);
}

void gdt_init(cpu_ctrl_t *cpu_ctrl) {
    gdt_entry_t *gdt_entries = null;
    tss64_t *tss = null;

    if (cpu_ctrl->is_bsp) {
        tss = &bsp_tss;
        gdt_entries = bsp_gdt_entries;

        memset(gdt_entries, 0, sizeof(gdt_entry_t) * MAX_GDT_ENTRIES);
        memset(tss, 0, sizeof(tss64_t));

        cpu_ctrl->tss = tss;
        cpu_ctrl->gdt_entries = gdt_entries;
    } else {
        tss = cpu_ctrl->tss;
        gdt_entries = cpu_ctrl->gdt_entries;
    }
    
    gdt_entry_set(gdt_entries, KERNEL_CODE_SELECTOR, 
        0, 0, 
        SEGMENT_ACCESS_PRESENT | SEGMENT_ACCESS_DPL0 | SEGMENT_ACCESS_CODE,
        SEGMENT_FLAGS_LONG | SEGMENT_FLAGS_GRANULARITY
    );

    gdt_entry_set(gdt_entries, KERNEL_DATA_SELECTOR, 
        0, 0, 
        SEGMENT_ACCESS_PRESENT | SEGMENT_ACCESS_DPL0 | SEGMENT_ACCESS_DATA,
        SEGMENT_FLAGS_GRANULARITY | SEGMENT_FLAGS_LONG
    );


    gdt_entry_set(gdt_entries, USER_CODE_SELECTOR, 
        0, 0, 
        SEGMENT_ACCESS_PRESENT | SEGMENT_ACCESS_DPL3 | SEGMENT_ACCESS_CODE,
        SEGMENT_FLAGS_GRANULARITY | SEGMENT_FLAGS_LONG
    );

    gdt_entry_set(gdt_entries, USER_DATA_SELECTOR, 
        0, 0, 
        SEGMENT_ACCESS_PRESENT | SEGMENT_ACCESS_DPL3 | SEGMENT_ACCESS_DATA,
        SEGMENT_FLAGS_GRANULARITY | SEGMENT_FLAGS_LONG
    );


    //
    // Initialize interrupt stacks in TSS
    //
    tss->rsp0 = 0;
    if (cpu_ctrl->is_bsp) {
        tss->ist1 = kernel_stack_of_ist1;
        tss->ist2 = kernel_stack_of_ist2;
    }

    gdt_sysentry_set(gdt_entries, TSS_KERNEL_STACK_SELECTOR, (uptr)tss, sizeof(tss64_t) - 1, SEGMENT_ACCESS_TSS64, SEGMENT_FLAGS_LONG);

    __lgdt(gdt_entries, sizeof(gdt_entry_t) * MAX_GDT_ENTRIES);
    _flush_gdt();
    __ltr(TSS_KERNEL_STACK_SELECTOR);
}
