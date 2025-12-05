#ifndef __gdt_h__
#define __gdt_h__

#include <lib/ktypes.h>
#include <system/cpu.h>


#define MAX_GDT_ENTRIES     256


#define SEGMENT_ACCESS_RW           (1 << 1)
#define SEGMENT_ACCESS_DIRECTION_CONFIRMIMG       (1 << 2)
#define SEGMENT_ACCESS_EXECUTE      (1 << 3)
#define SEGMENT_ACCESS_SYSTEM       (1 << 4)
#define SEGMENT_ACCESS_DPL0         (0 << 5)
#define SEGMENT_ACCESS_DPL3         (3 << 5)
#define SEGMENT_ACCESS_PRESENT      (1 << 7)

#define SEGMENT_ACCESS_CODE         (SEGMENT_ACCESS_SYSTEM | SEGMENT_ACCESS_EXECUTE | SEGMENT_ACCESS_RW)
#define SEGMENT_ACCESS_DATA         (SEGMENT_ACCESS_SYSTEM | SEGMENT_ACCESS_RW)

#define SEGMENT_ACCESS_TSS16        (SEGMENT_ACCESS_PRESENT | SEGMENT_ACCESS_DPL0 | 0x1)
#define SEGMENT_ACCESS_LDT          (SEGMENT_ACCESS_PRESENT | SEGMENT_ACCESS_DPL0 | 0x2)
#define SEGMENT_ACCESS_TSS16_BUSY   (SEGMENT_ACCESS_PRESENT | SEGMENT_ACCESS_DPL0 | 0x3)
#define SEGMENT_ACCESS_TSS32        (SEGMENT_ACCESS_PRESENT | SEGMENT_ACCESS_DPL0 | 0x9)
#define SEGMENT_ACCESS_TSS32_BUSY   (SEGMENT_ACCESS_PRESENT | SEGMENT_ACCESS_DPL0 | 0xb)

#define SEGMENT_ACCESS_TSS64        (SEGMENT_ACCESS_PRESENT | SEGMENT_ACCESS_DPL0 | 0x9)
#define SEGMENT_ACCESS_TSS64_BUSY   (SEGMENT_ACCESS_PRESENT | SEGMENT_ACCESS_DPL0 | 0xb)


#define SEGMENT_FLAGS_GRANULARITY   (1 << 3)
#define SEGMENT_FLAGS_SIZE_4KB      (1 << 2)
#define SEGMENT_FLAGS_LONG          (1 << 1)



#define KERNEL_CODE_SELECTOR        0x0008
#define KERNEL_DATA_SELECTOR        0x0010

#define USER_DATA_SELECTOR          0x0023
#define USER_CODE_SELECTOR          0x002b


#define TSS_KERNEL_STACK_SELECTOR   0x0078


void gdt_init(cpu_ctrl_t *cpu_ctrl);

void gdt_entry_set(gdt_entry_t *gdt_entries, u16 selector, u32 base, u32 limit, u8 access, u8 flags);
void gdt_sysentry_set(gdt_entry_t *gdt_entries, u16 selector, u64 base, u32 limit, u8 access, u8 flags);


#endif //__gdt_h__