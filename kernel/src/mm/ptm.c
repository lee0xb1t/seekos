#include <mm/mm.h>
#include <log/klog.h>
#include <panic/panic.h>
#include <ia32/cpuinstr.h>
#include <lib/kmemory.h>
#include <lib/kutils.h>
#include <base/spinlock.h>


DECLARE_SPINLOCK(ptm_lock);

void _prepare_hh_init();
void _prepare_kernel_init(struct limine_memmap_response *mm, struct limine_executable_address_response *execa);

void _early_prepare_pgtable(const void* virt_addr, page_t *map_page, u64 page_attr);
bool _ptmp_address_valid_root(pml4e_t *root, void *virt_addr);


void ptm_init(struct limine_memmap_response *mm, struct limine_executable_address_response *execa) {
    klog_debug("\n");

    memset(pml4_root, 0, PAGE_SIZE);
    pml4_root_phys = ((uptr)pml4_root) - execa->virtual_base + execa->physical_base;

    _prepare_hh_init();
    _prepare_kernel_init(mm, execa);

    /* Write new cr3 */
    klog_debug("PML4 root physical address: %p\n", pml4_root_phys);
#ifdef _TEST_CASE
    _ptmp_address_valid_root(pml4_root, (void *)ptm_init);
    _ptmp_address_valid_root(pml4_root, (void *)kmem.bitmap_start);
    _ptmp_address_valid_root(pml4_root, (void *)kmem.pfn_database_start);
#endif
    __writecr3(pml4_root_phys);

    klog_debug("PTM initialized.\n\n");
}


/* Initialize High half on new page table */
void _prepare_hh_init() {
    memset(pml4_root, 0, PAGE_SIZE);

    // pml4e_t *recursive_entry = &pml4_root[RECURSIVE_ENTRY_IDX];
    // recursive_entry->as_u64 = pml4_root_phys;
    // recursive_entry->present = 1;
    // recursive_entry->write = 1;
    ptm_recursive_init(pml4_root, pml4_root_phys);

    for (int i = 256; i < PT_ENTRY_COUNT; i++) {
        // if (i == RECURSIVE_ENTRY_IDX) {
        //     /* Recursive mapping */
        //     pml4e_t *pml4e_va = &pml4_root[RECURSIVE_ENTRY_IDX];
        //     pml4e_va->page_frame = pml4_root_phys >> PAGE_SHIFT;
        //     pml4e_va->present = 1;
        //     pml4e_va->write = 1;
        //     continue;
        // }
        
        if (!pml4_root[i].present) {
            pfn_number pfn = pmm_alloc_page();
            pml4_root[i].page_frame = pfn;
            pml4_root[i].present = 1;
            pml4_root[i].write = 1;

            uptr newpage = pfn << PAGE_SHIFT;
            memset(__PHYS_TO_VIRT(newpage), 0, PAGE_SIZE);
        }
    }
}

void ptm_recursive_init(pml4e_t *r, u64 phys) {
    pml4e_t *recursive_entry = &r[RECURSIVE_ENTRY_IDX];
    recursive_entry->as_u64 = phys;
    recursive_entry->present = 1;
    recursive_entry->write = 1;
}


void _early_prepare_pgtable(const void* virt_addr, page_t *map_page, u64 page_attr) {
    uptr pml4e_idx = PAGING_PML4_INDEX((uptr)virt_addr);
    uptr pdpte_idx = PAGING_PDPT_INDEX((uptr)virt_addr);
    uptr pde_idx = PAGING_PDT_INDEX((uptr)virt_addr);
    uptr pte_idx = PAGING_PT_INDEX((uptr)virt_addr);

    pml4e_t *pml4e_va = &pml4_root[pml4e_idx];

    if (!pml4e_va->present) {
        pfn_number pfn = pmm_alloc_page();
        uptr phys_addr = pfn << PAGE_SHIFT;

        pt_entry_t *pt_entry = __PHYS_TO_VIRT(phys_addr);
        memset(pt_entry, 0, PAGE_SIZE);

        pml4e_va->page_frame = pfn;
        pml4e_va->write = 1;
        pml4e_va->present = 1;
    }

    pdpte_t *pdpt_va = __PHYS_TO_VIRT(pml4e_va->page_frame << PAGE_SHIFT);
    pdpte_t *pdpte_va = &pdpt_va[pdpte_idx];

    if (!pdpte_va->present) {
        pfn_number pfn = pmm_alloc_page();
        uptr phys_addr = pfn << PAGE_SHIFT;

        pt_entry_t *pt_entry = __PHYS_TO_VIRT(phys_addr);
        memset(pt_entry, 0, PAGE_SIZE);

        pdpte_va->page_frame = pfn;
        pdpte_va->write = 1;
        pdpte_va->present = 1;
    }

    pde_t *pdt_va = __PHYS_TO_VIRT(pdpte_va->page_frame << PAGE_SHIFT);
    pde_t *pde_va = &pdt_va[pde_idx];

    if (!pde_va->present) {
        pfn_number pfn = pmm_alloc_page();
        uptr phys_addr = pfn << PAGE_SHIFT;

        pt_entry_t *pt_entry = __PHYS_TO_VIRT(phys_addr);
        memset(pt_entry, 0, PAGE_SIZE);

        pde_va->page_frame = pfn;
        pde_va->write = 1;
        pde_va->present = 1;
    }

    pte_t *pt_va = __PHYS_TO_VIRT(pde_va->page_frame << PAGE_SHIFT);
    pte_t *pte_va = &pt_va[pte_idx];

    pte_va->as_u64 = page_attr;
    pte_va->page_frame = map_page->phys_addr >> PAGE_SHIFT;
}


void _prepare_kernel_init(struct limine_memmap_response *mm, struct limine_executable_address_response *execa) {
    /* Create a new page table */
    /* Pages is already reserved */

    for (size_t i = 0; i < mm->entry_count; i++) {
        if (mm->entries[i]->type == LIMINE_MEMMAP_EXECUTABLE_AND_MODULES) {
            uptr va = execa->virtual_base + mm->entries[i]->base - execa->physical_base;
            for (uptr offset = 0; offset < mm->entries[i]->length; offset += PAGE_SIZE) {
                uptr pa = execa->physical_base + offset;
                pfn_number pfn = pa >> PAGE_SHIFT;
                page_t *page = &kmem.pfn_database_start[pfn];
                uptr va_ = va + offset;
                _early_prepare_pgtable((void *)va_, page, VMM_FLAGS_DEFAULT);
            }
        } else {
            if (mm->entries[i]->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
                || mm->entries[i]->type == LIMINE_MEMMAP_ACPI_TABLES) {

                for (uptr offset = 0; offset < mm->entries[i]->length; offset += PAGE_SIZE) {
                    uptr pa = mm->entries[i]->base + offset;
                    uptr va = __PHYS_TO_VIRT(pa);
                    pfn_number pfn = pa >> PAGE_SHIFT;
                    page_t *page = &kmem.pfn_database_start[pfn];
                    _early_prepare_pgtable((void *)va, page, VMM_FLAGS_DEFAULT);
                }
            }
        }
    }

    for (uptr va = (uptr)kmem.bitmap_start; va < (uptr)kmem.bitmap_end; va += PAGE_SIZE) {
        uptr pa = __VIRT_TO_PHYS(va);
        pfn_number pfn = pa >> PAGE_SHIFT;
        page_t *page = &kmem.pfn_database_start[pfn];
        _early_prepare_pgtable((void *)va, page, PTE_64_PRESENT_FLAG | PTE_64_WRITE_FLAG);
    }

    for (uptr va = (uptr)kmem.pfn_database_start; va < (uptr)kmem.pfn_database_end; va += PAGE_SIZE) {
        uptr pa = __VIRT_TO_PHYS(va);
        pfn_number pfn = pa >> PAGE_SHIFT;
        page_t *page = &kmem.pfn_database_start[pfn];
        _early_prepare_pgtable((void *)va, page, PTE_64_PRESENT_FLAG | PTE_64_WRITE_FLAG);
    }
}


void ptm_alloc_pml4es(void* virt_addr, u32 nr_pages, u64 flags) {
    u64 lock_flags;
    pml4e_t *pointer_pml4e = null;

    pml4e_t *pml4e_start = mi_ptr_to_pml4e(virt_addr);
    pml4e_t *pml4e_end = mi_ptr_to_pml4e( (void *)((uptr)virt_addr + (nr_pages * PAGE_SIZE)) );

    spin_lock_irq(&ptm_lock, lock_flags);

    for (pointer_pml4e = pml4e_start; pointer_pml4e <= pml4e_end; pointer_pml4e++) {
        if (!pointer_pml4e->present) {
            pfn_number pfn = pmm_alloc_page();

            pointer_pml4e->page_frame = pfn;
            pointer_pml4e->present = PT_ENTRY_64_PRESENT(flags);
            pointer_pml4e->write = PT_ENTRY_64_WRITE(flags);
            pointer_pml4e->supervisor = PT_ENTRY_64_SUPERVISOR(flags);
            pointer_pml4e->write_through = PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH(flags);
            pointer_pml4e->cache_disable = PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE(flags);
            pointer_pml4e->accessed = PT_ENTRY_64_ACCESSED(flags);
            pointer_pml4e->restart = PDPTE_64_RESTART(flags);

            memset(mi_pte_to_ptr(pointer_pml4e), 0, PAGE_SIZE);
        }
    }

    spin_unlock_irq(&ptm_lock, lock_flags);
}


void ptm_alloc_pdptes(void* virt_addr, u32 nr_pages, u64 flags) {
    u64 lock_flags;
    pdpte_t *pointer_pdpte = null;

    pdpte_t *pdpte_start = mi_ptr_to_pdpte(virt_addr);
    pdpte_t *pdpte_end = mi_ptr_to_pdpte( (void *)((uptr)virt_addr + (nr_pages * PAGE_SIZE)) );

    spin_lock_irq(&ptm_lock, lock_flags);

    for (pointer_pdpte = pdpte_start; pointer_pdpte <= pdpte_end; pointer_pdpte++) {
        if (!pointer_pdpte->present) {
            pfn_number pfn = pmm_alloc_page();

            pointer_pdpte->page_frame = pfn;
            pointer_pdpte->present = PT_ENTRY_64_PRESENT(flags);
            pointer_pdpte->write = PT_ENTRY_64_WRITE(flags);
            pointer_pdpte->supervisor = PT_ENTRY_64_SUPERVISOR(flags);
            pointer_pdpte->write_through = PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH(flags);
            pointer_pdpte->cache_disable = PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE(flags);
            pointer_pdpte->accessed = PT_ENTRY_64_ACCESSED(flags);
            pointer_pdpte->large_page = PT_ENTRY_64_LARGE_PAGE(flags);
            pointer_pdpte->restart = PDPTE_64_RESTART(flags);

            memset(mi_pte_to_ptr(pointer_pdpte), 0, PAGE_SIZE);
        }
    }

    spin_unlock_irq(&ptm_lock, lock_flags);
}

void ptm_alloc_pdes(void* virt_addr, u32 nr_pages, u64 flags) {
    u64 lock_flags;
    pde_t *pointer_pde = null;

    pde_t *pde_start = mi_ptr_to_pde(virt_addr);
    pde_t *pde_end = mi_ptr_to_pde( (void *)((uptr)virt_addr + (nr_pages * PAGE_SIZE)) );

    spin_lock_irq(&ptm_lock, lock_flags);

    for (pointer_pde = pde_start; pointer_pde <= pde_end; pointer_pde++) {
        if (!pointer_pde->present) {
            pfn_number pfn = pmm_alloc_page();

            pointer_pde->page_frame = pfn;
            pointer_pde->present = PT_ENTRY_64_PRESENT(flags);
            pointer_pde->write = PT_ENTRY_64_WRITE(flags);
            pointer_pde->supervisor = PT_ENTRY_64_SUPERVISOR(flags);
            pointer_pde->write_through = PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH(flags);
            pointer_pde->cache_disable = PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE(flags);
            pointer_pde->accessed = PT_ENTRY_64_ACCESSED(flags);
            pointer_pde->large_page = PT_ENTRY_64_LARGE_PAGE(flags);
            pointer_pde->restart = PDE_64_RESTART(flags);

            memset(mi_pte_to_ptr(pointer_pde), 0, PAGE_SIZE);
        }
    }

    spin_unlock_irq(&ptm_lock, lock_flags);
}


void ptm_map_pages(void* virt_addr, uptr phys_addr, size_t nr_pages, u64 flags) {
    u64 lock_flags;
    pte_t *pointer_pte = null;

    void *start_addr = (void *)virt_addr;
    void *end_addr = (void *)((uptr)virt_addr + (nr_pages * PAGE_SIZE));

    pte_t *pte_start = mi_ptr_to_pte(start_addr);
    pte_t *pte_end = mi_ptr_to_pte(end_addr);

    size_t i = 0;
    pfn_number pfn = phys_addr >> PAGE_SHIFT;

    ptm_alloc_pml4es(virt_addr, nr_pages, flags);
    ptm_alloc_pdptes(virt_addr, nr_pages, flags);
    ptm_alloc_pdes(virt_addr, nr_pages, flags);

    spin_lock_irq(&ptm_lock, lock_flags);

    for (pointer_pte = pte_start; pointer_pte < pte_end; pointer_pte++, i++, pfn++) {
        if (pointer_pte->present) {
            panic("ptm: %x's pte is already exists!", (uptr)virt_addr + (i << PAGE_SHIFT));
        }

        pointer_pte->as_u64 = 0;

        pointer_pte->page_frame = pfn;
        
        pointer_pte->present = PT_ENTRY_64_PRESENT(flags);
        pointer_pte->write = PT_ENTRY_64_WRITE(flags);
        pointer_pte->supervisor = PT_ENTRY_64_SUPERVISOR(flags);
        pointer_pte->write_through = PT_ENTRY_64_PAGE_LEVEL_WRITE_THROUGH(flags);
        pointer_pte->cache_disable = PT_ENTRY_64_PAGE_LEVEL_CACHE_DISABLE(flags);
        pointer_pte->accessed = PT_ENTRY_64_ACCESSED(flags);
        pointer_pte->dirty = PT_ENTRY_64_DIRTY(flags);
        pointer_pte->restart = PTE_64_RESTART(flags);
    }

    spin_unlock_irq(&ptm_lock, lock_flags);

    __invlpg(virt_addr);
}

void ptm_unmap_pages(void* virt_addr, size_t nr_pages) {
    u64 flags;
    pte_t *pointer_pte = null;

    void *start_addr = (void *)virt_addr;
    void *end_addr = (void *)((uptr)virt_addr + (nr_pages * PAGE_SIZE));

    pte_t *pte_start = mi_ptr_to_pte(start_addr);
    pte_t *pte_end = mi_ptr_to_pte(end_addr);

    spin_lock_irq(&ptm_lock, flags);

    int i = 0;
    for (pointer_pte = pte_start; pointer_pte < pte_end; pointer_pte++, i++) {
        if (!pointer_pte->present) {
            panic("ptm: %x's pte is not exists!", (uptr)virt_addr + (i << PAGE_SHIFT));
        }
        // memset(mi_pte_to_ptr(pointer_pte), 0, PAGE_SIZE);
        pmm_free_pages(pointer_pte->page_frame);
        
        pointer_pte->as_u64 = 0;
    }

    // TODO
    // ptm_free_pdptes(virt_addr, nr_pages, flags);
    // ptm_free_pdes(virt_addr, nr_pages, flags);

    spin_unlock_irq(&ptm_lock, flags);
    
    __invlpg(virt_addr);
}

pte_t *ptm_get_pte(void* virt_addr) {
    return mi_ptr_to_pte(virt_addr);
}

bool _ptmp_address_valid_root(pml4e_t *root, void *virt_addr) {
    klog_debug("\n");
    klog_debug("[%p]:\n", virt_addr);

    uptr pml4e_idx = PAGING_PML4_INDEX((uptr)virt_addr);
    uptr pdpte_idx = PAGING_PDPT_INDEX((uptr)virt_addr);
    uptr pde_idx = PAGING_PDT_INDEX((uptr)virt_addr);
    uptr pte_idx = PAGING_PT_INDEX((uptr)virt_addr);

    uptr pml4e_phys = root[pml4e_idx].page_frame << PAGE_SHIFT;
    if (!root[pml4e_idx].present) {
        klog_debug("\tpml4e is not valid!\n");
        return false;
    } else {
        klog_debug("\tpml4e phys addr: %p\n", pml4e_phys);
    }
    pdpte_t *pdpt = __PHYS_TO_VIRT(pml4e_phys);


    uptr pdpte_phys = pdpt[pdpte_idx].page_frame << PAGE_SHIFT;
    if (!pdpt[pdpte_idx].present) {
        klog_debug("\tpdpte is not valid!\n");
        return false;
    } else {
        klog_debug("\tpdpte phys addr: %p\n", pdpte_phys);
    }
    pde_t *pdt = __PHYS_TO_VIRT(pdpte_phys);


    uptr pde_phys = pdt[pde_idx].page_frame << PAGE_SHIFT;
    if (!pdt[pde_idx].present) {
        klog_debug("\tpde is not valid!\n");
        return false;
    } else {
        klog_debug("\tpde phys addr: %p\n", pde_phys);
    }
    pte_t *pt = __PHYS_TO_VIRT(pde_phys);


    uptr p_phys = pt[pte_idx].page_frame << PAGE_SHIFT;
    if (!pt[pte_idx].present) {
        klog_debug("\tpte is not valid!\n");
        return false;
    } else {
        klog_debug("\tpte phys addr: %p\n", p_phys);
    }
    pt_entry_t *pentry = __PHYS_TO_VIRT(p_phys);
    klog_debug("Final physical address: %p\n", (pentry->page_frame << PAGE_SHIFT) + ((uptr)virt_addr & ~PAGE_MASK));

    klog_debug("\n");

    return true;
}
