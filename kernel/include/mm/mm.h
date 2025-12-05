#ifndef __mm_h__
#define __mm_h__

#include <bootparams.h>
#include <limine.h>
#include <ia32/paging.h>
#include <lib/ktypes.h>
#include <base/linkedlist.h>
#include <lib/kutils.h>
#include <system/kstatus.h>
#include <system/klayout.h>
#include <base/spinlock.h>


//
// Kernel virt range
//

// #define MI_HIGHEST_START            0xffff000000000000

// #define MI_SYSTEM_START             0xffff800000000000

#define MI_APENTRY_LOWEST_START         0x0
#define MI_APENTRY_LOWEST_END           0x1fff
#define MI_APENTRY_LOWEST_SIZE          (MI_APENTRY_LOWEST_END + 1 - MI_APENTRY_LOWEST_START)


/* 0x80, 0, 0, 0, 0 */
#define MI_EARLY_DIRECT_START       0xffff800000000000


/* 0x1ed, 0, 0, 0, 0 */
#define FRAME_BUFFER_START          0xfffff68000000000
/* 0x1ed, 0xff, 0x1ff, 0x1ff, 0xfff */
#define FRAME_BUFFER_END            0xfffff6bfffffffff
#define FRAME_BUFFER_SIZE           (FRAME_BUFFER_END + 1 - FRAME_BUFFER_START)

/* 0x1ee, 0, 0, 0, 0 */
#define MI_DIRECT_MAPPING_RANGE_START      0xfffff70000000000
/* 0x1ee, 0x1ff, 0x1ff, 0x1ff, 0xfff */
#define MI_DIRECT_MAPPING_RANGE_END        0xfffff77fffffffff
#define MI_DIRECT_MAPPING_RANGE_SIZE       (MI_DIRECT_MAPPING_RANGE_END + 1 - MI_DIRECT_MAPPING_RANGE_START)

/* 0x1fe, 0, 0, 0, 0 */
#define MI_RECURSIVE_START          0xffffff0000000000
/* 0x1fe, 0x1ff, 0x1ff, 0x1ff, 0xfff */
#define MI_RECURSIVE_END            0xffffff7fffffffff
#define MI_RECURSIVE_SIZE           (MI_RECURSIVE_END + 1 - MI_RECURSIVE_START)

/* 0x1fc, 0, 0, 0, 0 */
#define MI_KERNEL_VMAREA_SAPACE_START       0xfffffe0000000000
/* 0x1fd, 0x1ff, 0x1ff, 0x1ff, 0xfff */
#define MI_KERNEL_VMAREA_SAPACE_END         0xfffffeffffffffff
#define MI_KERNEL_VMAREA_SAPACE_SIZE        (MI_KERNEL_VMAREA_SAPACE_END + 1 - MI_KERNEL_VMAREA_SAPACE_START)

#define MI_KERNEL_START             ((uptr)__kernel_start)
#define MI_KERNEL_END               ((uptr)__kernel_end)

#define MI_HIGHEST_SYSTEM_ADDRESS   0xffffffffffffffff


#define MI_SMP_APENTRY_PHYS_START   (0x1000)
#define MI_SMP_APENTRY_START        (0xfffff70000000000 + MI_SMP_APENTRY_PHYS_START)
#define MI_SMP_APENTRY_END          (0xfffff70000000000 + MI_SMP_APENTRY_PHYS_START + 0xfff)
#define MI_SMP_APENTRY_SIZE         (MI_SMP_APENTRY_END + 1 - MI_SMP_APENTRY_START)



//
// User virt range
//

/* 0, 0, 0, 0, 0 */
#define MI_USER_VMAREA_SAPACE_START       0x10000
/* 0x1fd, 0x1ff, 0x1ff, 0x1ff, 0xfff */
#define MI_USER_VMAREA_SAPACE_END         0x7fffffffffff
#define MI_USER_VMAREA_SAPACE_SIZE        (MI_USER_VMAREA_SAPACE_END + 1 - MI_USER_VMAREA_SAPACE_START)


// #define MI_HYPERSPACE_PTES          255


#define MI_KERNEL_START_PHYS            (__kernel_start - (u8 *)KERNEL_KERNEL_START)
#define MI_KERNEL_END_PHYS              (__kernel_end - (u8 *)KERNEL_KERNEL_START)
#define MI_KERNEL_SIZE                  ((u64)__kernel_end - (u64)__kernel_start)

#define MI_STEXT_START_PHYS             (__stext_start - (u8 *)KERNEL_KERNEL_START)
#define MI_STEXT_END_PHYS               (__stext_end - (u8 *)KERNEL_KERNEL_START)



//
// Paging
//

#define PAGE_PML4I_SHIFT        39
#define PAGE_PDPTI_SHIFT        30
#define PAGE_PDI_SHIFT          21
#define PAGE_PTI_SHIFT          12


#define PAGE_SIZE               4096
#define PAGE_MASK               (~(PAGE_SIZE - 1))
#define PAGE_SHIFT              12
#define PAGE_ALIGN(s)           (((uptr)(s)) & PAGE_MASK)
#define PAGE_ALIGN_UP(s)        (PAGE_ALIGN((s)) + PAGE_SIZE)
#define PAGE_ALIGN_UP_IF(s)   \
        (  ((s) & (PAGE_SIZE - 1)) ? PAGE_ALIGN_UP(s) : (s)   )


#define PAGE_2MB_SIZE           0x200000
#define PAGE_2MB_MASK           (~(PAGE_2MB_SIZE - 1))
#define PAGE_2MB_SHIFT          21
#define PAGE_2MB_ALIGN(s)       ((s) & PAGE_2MB_MASK)
#define PAGE_2MB_ALIGN_UP(s)    (PAGE_2MB_ALIGN((s)) + PAGE_2MB_SIZE)
#define PAGE_2MB_ALIGN_UP_IF(s)   \
        (  ((s) & (PAGE_2MB_SIZE - 1)) ? PAGE_2MB_ALIGN(s) : (s)   )


#define SIZE_TO_PAGE(s)         ( ((uptr)(s)) / PAGE_SIZE )
#define PAGE_TO_SIZE(s)         ( ((uptr)(s)) * PAGE_SIZE )

#define PT_HIGH_HALF_START      256
#define PT_ENTRY_COUNT          512

#define RECURSIVE_ENTRY_OFFSET      ((uptr)0xff0)
#define RECURSIVE_ENTRY_IDX         (RECURSIVE_ENTRY_OFFSET / 8)


#define MI_RESURSIVE_PML4E_BASE     0xffffff7fbfdfe000
#define MI_RESURSIVE_PDPTE_BASE     0xffffff7fbfc00000
#define MI_RESURSIVE_PDE_BASE       0xffffff7f80000000
#define MI_RESURSIVE_PTE_BASE       0xffffff0000000000


#define PAGING_PML4_INDEX(va)       ((((uptr)(va)) >> PAGE_PML4I_SHIFT) & 0x1ff)
#define PAGING_PDPT_INDEX(va)       ((((uptr)(va)) >> PAGE_PDPTI_SHIFT) & 0x1ff)
#define PAGING_PDT_INDEX(va)        ((((uptr)(va)) >> PAGE_PDI_SHIFT) & 0x1ff)
#define PAGING_PT_INDEX(va)         ((((uptr)(va)) >> PAGE_PTI_SHIFT) & 0x1ff)
#define PAGING_OFFSET(va)           (((uptr)(va)) & 0xfff)


#define PAGING_MKAE_KERNEL_ADDR(index4,index3,index2,index1,offset) \
    (MI_SYSTEM_START | ((index4) << 39) | ((index3) << 30) | ((index2) << 21) | ((index1) << 12) | (offset))

#define BITMAP_PER_BYTE             (PAGE_SIZE * 8ull)
#define BITMAP_PER_U64              (PAGE_SIZE * 64ull)

#define MM_128MB_VALUE              ((uptr)1024 * 1024 * 128)

#define MM_2GB_VALUE                ((uptr)1024 * 1024 * 1024 * 2)

#define __VIRT_TO_PHYS(s)           (((uptr)(s)) - MI_EARLY_DIRECT_START)
#define __PHYS_TO_VIRT(s)           ((void*)( ((uptr)(s)) + MI_EARLY_DIRECT_START) )



#define VMM_FLAGS_DEFAULT           (PT_ENTRY_64_PRESENT_FLAG | PT_ENTRY_64_WRITE_FLAG)

#define VMM_FLAGS_USER              (VMM_FLAGS_DEFAULT | PT_ENTRY_64_SUPERVISOR_FLAG)


// 根据IDM.Vol3 13.3.2,MMIO必须为强非缓存(Strong uncacheable)
#define VMM_FLAGS_MMIO              (VMM_FLAGS_DEFAULT | PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE_FLAG |    \
                                    PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH_FLAG)





typedef uptr    pfn_number;

typedef struct _page_t{
    u16 refcount;
    union {
        u16 flags;
        struct {
            u16 in_buddy :1;
        };
    };
    u16 order;
    uptr phys_addr;
    linked_list_t list_entry;
} page_t;



#define MM_MAX_ORDER            11

struct free_area_t
{
    linked_list_t list_head;
    size_t nr_free;
};


typedef struct {
    u64 *bitmap_start;
    uptr bitmap_pa;
    size_t bitmap_count;
    uptr bitmap_end;
    uptr bitmap_start_of_search;

    page_t *pfn_database_start;
    uptr pfn_database_pa;
    uptr pfn_database_bytes;
    uptr pfn_database_end;
    spinlock_t pfn_lock;

    struct free_area_t buddy_free_area[MM_MAX_ORDER];

    size_t nr_pages;
    size_t nr_free_pages;
    uptr phys_limit;
} kmem_info_t;


// enum zone_type_t {
//     ZONE_DMA = 0,
//     ZONE_NORMAL
// };

// #define ZONE_MAX_COUNT       (ZONE_DONTUSE - 1)


enum page_level_t {
    PAGE_LAVEL_PT = 1,
    PAGE_LAVEL_PDT = 2,
    PAGE_LAVEL_PDPT = 3,
    PAGE_LAVEL_PML4T = 4
};


// typedef struct {
//     uptr zone_start;
//     uptr zone_end;
//     enum zone_type_t zone_type;
// } zone_t;




/* Slab */
#define SLAB_NAME_LENGTH        32

typedef struct _slab_entry_t {
    struct _slab_entry_t *next;
} slab_entry_t;

typedef struct _slab_t {
    void *s_mem;
    u32 size;
    u32 free_objects;
    slab_entry_t *free_list;    /* Next available object */
    linked_list_t slab_list;
} slab_t;

typedef struct {
    char name[SLAB_NAME_LENGTH];
    slab_t slabs;
    size_t obj_size;
    u32 free_objects;
    u32 total_objects;
    u32 nr_pages;
    u32 total_pages;

    linked_list_t list;
} slab_cache_t;

typedef struct {
    char name[SLAB_NAME_LENGTH];
    u32 obj_size;
    u32 nr_pages;
} slab_size_t;

#define SLAB_META_CACHE_NRPAGES     2
#define SLAB_ALLOC_MAX_SIZE         (8192)


/* Virtual Memory */

// typedef struct {
//     uptr virt_addr;
//     uptr phys_addr;
//     size_t nr_pages;
//     u64 flags;
//     linked_list_t list;
//     // lock
// } map_info_t;


// typedef struct {
//     uptr vmm_space_start;
//     uptr vmm_space_end;
// } addr_space_t;

typedef struct {
    linked_list_t list_entry;
    uptr vm_start;
    uptr vm_end;
    u64 vm_flags;
} vm_area_t;

typedef struct {
    linked_list_t vm_area_head;     /*vm_area_t*/
    spinlock_t vm_area_lock;
    size_t size;
    size_t area_count;

    void *page_dir;
    uptr page_dir_pa;
} mm_struct_t;

// #define VM_FLAGS_READ       0x1
// #define VM_FLAGS_WRITE      0x2
// #define VM_FLAGS_EXEC       0x4
// #define VM_FLAGS_GENERIC    (VM_FLAGS_READ | VM_FLAGS_WRITE | VM_FLAGS_EXEC)
// #define VM_FLAGS_IO         0x8



/* Variables */
extern volatile kmem_info_t kmem;
extern volatile pml4e_t pml4_root[PT_ENTRY_COUNT];
extern volatile uptr pml4_root_phys;



/* MM */
FORCEINLINE pml4e_t *mi_ptr_to_pml4e(void* addr) {
    uptr offset = (uptr)addr >> (PAGE_PML4I_SHIFT - 3);
    offset &= 0x1ff << 3;
    return (pml4e_t *)(MI_RESURSIVE_PML4E_BASE + offset);
}

FORCEINLINE pdpte_t *mi_ptr_to_pdpte(void* addr) {
    uptr offset = (uptr)addr >> (PAGE_PDPTI_SHIFT - 3);
    // (0x1ff << 9) | 0x1ff
    offset &= 0x3ffff << 3;
    return (pdpte_t *)(MI_RESURSIVE_PDPTE_BASE + offset);
}

FORCEINLINE pde_t *mi_ptr_to_pde(void* addr) {
    uptr offset = (uptr)addr >> (PAGE_PDI_SHIFT - 3);
    // (((0x1ff << 9) | 0x1ff) << 9) | 0x1ff
    offset &= 0x7ffffff << 3;
    return (pde_t *)(MI_RESURSIVE_PDE_BASE + offset);
}

FORCEINLINE pte_t *mi_ptr_to_pte(void* addr) {
    uptr offset = (uptr)addr >> (PAGE_PTI_SHIFT - 3);
    // (  (  ( (0x1ff << 9) | 0x1ff) << 9 ) | 0x1ff  ) << 9  ) | 0x1ff
    offset &= 0xfffffffff << 3;
    return (pte_t *)(MI_RESURSIVE_PTE_BASE + offset);
}

FORCEINLINE void * mi_pml4e_to_ptr(void* pointer_pml4e) {
    return (void *)(((i64)pointer_pml4e << 52) >> 16);
}

FORCEINLINE void * mi_pdpte_to_ptr(void* pointer_pdpte) {
    return (void *)(((i64)pointer_pdpte << 43) >> 16);
}

FORCEINLINE void * mi_pde_to_ptr(void* pointer_pde) {
    return (void *)(((i64)pointer_pde << 34) >> 16);
}

FORCEINLINE void *mi_pte_to_ptr(void* pointer_pte) {
    return (void *)(((i64)pointer_pte << 25) >> 16);
}

FORCEINLINE page_t *pfn_to_page(pfn_number pfn) {
    return &kmem.pfn_database_start[pfn];
}

FORCEINLINE pfn_number page_to_pfn(page_t *page) {
    return page->phys_addr >> PAGE_SHIFT;
}

FORCEINLINE uptr pfn_to_ptr(pfn_number pfn) {
    return pfn << PAGE_SHIFT;
}

FORCEINLINE pfn_number ptr_to_pfn(uptr p) {
    return p >> PAGE_SHIFT;
}



/* PMM */
void pmm_init(struct limine_memmap_response *mm);

pfn_number pmm_alloc_page();
void pmm_free_page(pfn_number pfn);

pfn_number pmm_alloc_pages(size_t nr_pages);
void pmm_free_pages(pfn_number pfn);

void pmm_dump_usage(bool brief);


/* PTM */
void ptm_init(struct limine_memmap_response *mm, struct limine_executable_address_response *execa);

void ptm_alloc_pml4es(void* virt_addr, u32 nr_pages, u64 flags);
void ptm_alloc_pdptes(void* virt_addr, u32 nr_pages, u64 flags);
void ptm_alloc_pdes(void* virt_addr, u32 nr_pages, u64 flags);

void ptm_map_pages(void* virt_addr, uptr phys_addr, size_t nr_pages, u64 flags);
void ptm_unmap_pages(void* virt_addr, size_t nr_pages);

pte_t *ptm_get_pte(void* virt_addr);

void ptm_recursive_init(pml4e_t *, u64 phys);


/* VMM */
#define mm_get_vmarea_start(_flags) ( (((_flags) & VMM_FLAGS_USER) == VMM_FLAGS_USER) ? MI_USER_VMAREA_SAPACE_START : MI_KERNEL_VMAREA_SAPACE_START )
#define mm_get_vmarea_end(_flags) ( (((_flags) & VMM_FLAGS_USER) == VMM_FLAGS_USER) ? MI_USER_VMAREA_SAPACE_END : MI_KERNEL_VMAREA_SAPACE_END )

void vmm_init();

uptr vmm_get_phys_addr(void *va);

void vmm_map(void *va, uptr pa, size_t nr_pages, u64 flags);
void vmm_unmap(void *va, size_t nr_pages);

void vmm_vmap(void *va, size_t nr_pages, u64 flags);
void vmm_vunmap(void *va, size_t nr_pages, u64 flags);

void *vmm_alloc_page();
void vmm_free_page(void *ptr);

void *vmm_alloc_pages(size_t nr_pages);
void vmm_free_pages(void *ptr, size_t nr_pages);

// void *vmm_alloc(mm_struct_t *mm, size_t size, u64 flags);
// void *vmm_free(mm_struct_t *mm, void *ptr);

void vmm_dump_vm_area();
void vmm_dump_slab();

void slab_init();
slab_t *slab_create(u32 nr_pages, u32 obj_size, u32 *obj_count_out);
void slab_cache_init(slab_cache_t *cache, char *name, u32 obj_size, u32 nr_pages);
void slab_cache_growth(slab_cache_t *cache);
void *slab_cache_alloc(slab_cache_t *cache, size_t size_of_bytes);
bool slab_cache_free(slab_cache_t *cache, void *ptr);
slab_cache_t *slab_cache_create_cache(char *name, u32 obj_size, u32 nr_pages);
void *slab_alloc(size_t size);
void slab_free(void *ptr, bool *is_slab);


/*vmalloc*/

void vmalloc_init();

void *vmalloc(mm_struct_t *mm, void *ptr, size_t size, u64 flags);
void *vfree(mm_struct_t *mm, void *ptr);

/*mm*/

#define mm_switch_save(new_cr3, old_cr3, flags)  \
    do {    \
        __barrier();    \
        flags = local_irq_save();   \
        old_cr3 = __readcr3();      \
        if (old_cr3 != new_cr3){     \
            __writecr3(new_cr3);        \
            __barrier();    } else {old_cr3 = 0;}   \
    } while(0)

#define mm_switch_restore(old_cr3, flags)  \
    do {    \
        if (old_cr3) {      \
        __barrier();    \
        __writecr3(old_cr3);        \
        __barrier();               } \
        local_irq_restore(flags);   \
    } while(0)

mm_struct_t *mm_create();
void mm_free(mm_struct_t *mm);
vm_area_t *mm_find_vm_area(linked_list_t *vmlist, void *ptr, size_t size, u64 flags);
vm_area_t *mm_get_vm_area(linked_list_t *vmlist, void *ptr);

mm_struct_t *mm_copy(mm_struct_t *p);
mm_struct_t *mm_replace(mm_struct_t *n);

// mm read/write
void mm_read(mm_struct_t *mm, void *virt, u8 *data, u32 len);
void mm_write(mm_struct_t *mm, void *virt, u8 *data, u32 len);
void mm_write_byte(mm_struct_t *mm, void *virt, u8 data);

void dump_mm();

#endif //__mm_h__