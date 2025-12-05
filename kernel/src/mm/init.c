#include <mm/mm.h>
#include <log/klog.h>
#include <panic/panic.h>
#include <system/klayout.h>
#include <lib/kmemory.h>
#include <ia32/cpuinstr.h>


volatile kmem_info_t kmem;

volatile pml4e_t pml4_root[PT_ENTRY_COUNT] __attribute__((aligned(PAGE_SIZE))) = { 0 };
volatile uptr pml4_root_phys = 0;
