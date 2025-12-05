#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <lib/kmemory.h>
#include <panic/panic.h>


/* VM Area */
//mm_struct_t kmm_struct;
linked_list_t k_vmlist;     /*vm_area_t*/
DECLARE_SPINLOCK(k_vmlist_lock);


vm_area_t *_vm_area_create(uptr va_start, uptr va_end, u64 flags);


void vmalloc_init() {
    ptm_alloc_pdptes((void *)MI_KERNEL_VMAREA_SAPACE_START, MI_KERNEL_VMAREA_SAPACE_SIZE / PAGE_SIZE, VMM_FLAGS_DEFAULT);

    dlist_init(&k_vmlist);

    klog_debug("vmalloc initialized");
}


mm_struct_t *mm_create() {
    mm_struct_t *mm = kzalloc(sizeof(mm_struct_t));
    dlist_init(&mm->vm_area_head);
    spin_init(&mm->vm_area_lock);

    pml4e_t *page_dir = (pml4e_t *)vmm_alloc_page();
    memset(page_dir, 0, PAGE_SIZE);
    mm->page_dir = (void *)page_dir;
    mm->page_dir_pa = pfn_to_page(mi_ptr_to_pte(page_dir)->page_frame)->phys_addr;

    uptr *kpage_dir = (uptr *)pml4_root;
    for (int i = PT_HIGH_HALF_START; i < PT_ENTRY_COUNT; i++) {
        page_dir[i].as_u64 = kpage_dir[i];
    }

    ptm_recursive_init(page_dir, mm->page_dir_pa);

    return mm;
}

void mm_free(mm_struct_t *mm) {
    u64 old_cr3;
    u64 switch_flags;
    
    mm_switch_save(mm->page_dir_pa, old_cr3, switch_flags);

    while (!dlist_is_empty(&mm->vm_area_head)) {
        linked_list_t *next = null;
        dlist_get_next(&mm->vm_area_head, &next);

        if (next) {
            vm_area_t *vmarea = dlist_container_of(next, vm_area_t, list_entry);
            vmm_vunmap((void *)vmarea->vm_start, SIZE_TO_PAGE(vmarea->vm_end - vmarea->vm_start), vmarea->vm_flags);
            dlist_remove_entry(&vmarea->list_entry);
            slab_free(vmarea, null);
        }
        next = null;
    }

    mm_switch_restore(old_cr3, switch_flags);

    vmm_free_page(mm->page_dir);
    kfree(mm);
}

vm_area_t *mm_find_vm_area(linked_list_t *vmlist, void *ptr, size_t size, u64 flags) {

    if (ptr) {
        uptr va_start = (uptr)ptr;
        uptr va_end = (uptr)ptr + size;

        if (va_end >= mm_get_vmarea_end(flags)) {
            return null;
        }

        /*if memory is already allocated, return null.*/
        dlist_foreach(vmlist, entry) {
            vm_area_t *vmarea = dlist_container_of(entry, vm_area_t, list_entry);

            if (vmarea->vm_start <= va_start && vmarea->vm_end > va_start 
                || vmarea->vm_start < va_end && vmarea->vm_end >= va_end
                || (va_start >= vmarea->vm_start && va_end <= vmarea->vm_end)) {
                
                return null;
            }
        }

        vm_area_t *vm_area_entry = _vm_area_create(va_start, va_end, flags);
        dlist_add_prev(vmlist, vm_area_entry);
        return vm_area_entry;
    }

     if (dlist_is_empty(vmlist)) {
        uptr va_start = mm_get_vmarea_start(flags);
        uptr va_end = mm_get_vmarea_start(flags) + size;

        if (va_end >= mm_get_vmarea_end(flags)) {
            return null;
        }

        vm_area_t *vm_area_entry = _vm_area_create(va_start, va_end, flags);

        dlist_add_prev(vmlist, vm_area_entry);

        return vm_area_entry;
    }

    vm_area_t *vm_area = vmlist->next;

    if (vm_area->list_entry.next == vmlist) {
        uptr va_start = vm_area->vm_end;
        uptr va_end = vm_area->vm_end + size;

        if (va_end >= mm_get_vmarea_end(flags)) {
            return null;
        }

        vm_area_t *vm_area_entry = _vm_area_create(va_start, va_end, flags);

        dlist_add_next(&vm_area->list_entry, &vm_area_entry->list_entry);

        return vm_area_entry;
    }

    // Find memory cave

    dlist_foreach(vmlist, entry) {
        if (entry->next == vmlist) {
            break;
        }

        vm_area_t *vm_area_start = dlist_container_of(entry, vm_area_t, list_entry);
        vm_area_t *vm_area_end = dlist_container_of(entry->next, vm_area_t, list_entry);

        if (vm_area_end->vm_start - vm_area_start->vm_end >= size) {
            // Founded cave
            uptr va_start = vm_area_start->vm_end;
            uptr va_end = vm_area_start->vm_end + size;

            if (va_end > vm_area_end->vm_start) {
                return null;
            }

            vm_area_t *vm_area_entry = _vm_area_create(va_start, va_end, flags);

            dlist_add_next(vm_area_start, &vm_area_entry->list_entry);

            return vm_area_entry;
        }
    }

    vm_area_t *vm_area_prev = dlist_container_of(vmlist->prev, vm_area_t, list_entry);
    uptr va_start = vm_area_prev->vm_end;
    uptr va_end = vm_area_prev->vm_end + size;

    if (va_end >= mm_get_vmarea_end(flags)) {
        return null;
    }

    vm_area_t *vm_area_entry = _vm_area_create(va_start, va_end, flags);

    dlist_add_next(vm_area_prev, &vm_area_entry->list_entry);

    return vm_area_entry;
}

vm_area_t *mm_get_vm_area(linked_list_t *vmlist, void *ptr) {
    dlist_foreach(vmlist, entry) {
        vm_area_t *vm_area = dlist_container_of(entry, vm_area_t, list_entry);
        
        if ((uptr) ptr == vm_area->vm_start) {
            dlist_remove_entry(&vm_area->list_entry);
            // vmm_vunmap(vm_area->vm_start, SIZE_TO_PAGE(vm_area->vm_end - vm_area->vm_start), vm_area->vm_flags);
            return vm_area;
        }
    }
    return null;
}

mm_struct_t *mm_copy(mm_struct_t *p) {
    mm_struct_t *n = mm_create();
    
    spin_lock(&p->vm_area_lock);
    // vm_area = mm_find_vm_area(&mm->vm_area_head, ptr, size, flags);
    dlist_foreach(&p->vm_area_head, entry) {
        vm_area_t *vmarea = dlist_container_of(entry, vm_area_t, list_entry);
        size_t sz = vmarea->vm_end - vmarea->vm_start;
        void *temp_ptr = kzalloc(sz);
        memcpy(temp_ptr, (void *)vmarea->vm_start, sz);
        void *ptr = vmalloc(n, (void *)vmarea->vm_start, sz, vmarea->vm_flags);
        mm_write(n, ptr, temp_ptr, sz);
        kfree(temp_ptr);
    }
    spin_unlock(&p->vm_area_lock);

    return n;
}

mm_struct_t *mm_replace(mm_struct_t *mm) {
    spin_lock(&mm->vm_area_lock);
    while (!dlist_is_empty(&mm->vm_area_head)) {
        linked_list_t *next = null;
        dlist_get_next(&mm->vm_area_head, &next);

        if (next) {
            vm_area_t *vmarea = dlist_container_of(next, vm_area_t, list_entry);
            vmm_vunmap((void *)vmarea->vm_start, SIZE_TO_PAGE(vmarea->vm_end - vmarea->vm_start), vmarea->vm_flags);
            dlist_remove_entry(&vmarea->list_entry);
            slab_free(vmarea, null);
        }
        next = null;
    }
    spin_unlock(&mm->vm_area_lock);
    return mm;
}

void mm_read(mm_struct_t *mm, void *virt, u8 *data, u32 len) {
    u64 old_cr3;
    u64 switch_flags;
    mm_switch_save(mm->page_dir_pa, old_cr3, switch_flags);
    memcpy(data, virt, len);
    mm_switch_restore(old_cr3, switch_flags);
}

void mm_write(mm_struct_t *mm, void *virt, u8 *data, u32 len) {
    u64 old_cr3;
    u64 switch_flags;
    mm_switch_save(mm->page_dir_pa, old_cr3, switch_flags);
    memcpy(virt, data, len);
    mm_switch_restore(old_cr3, switch_flags);
}

void mm_write_byte(mm_struct_t *mm, void *virt, u8 data) {
    u64 old_cr3;
    u64 switch_flags;
    mm_switch_save(mm->page_dir_pa, old_cr3, switch_flags);
    *((u8 *)virt) = data;
    mm_switch_restore(old_cr3, switch_flags);
}

vm_area_t *_vm_area_create(uptr va_start, uptr va_end, u64 flags) {
    vm_area_t *vm_area_entry = (vm_area_t *)slab_alloc(sizeof(vm_area_t));
    if (!vm_area_entry) {
        return null;
    }

    vm_area_entry->vm_start = va_start;
    vm_area_entry->vm_end = va_end;
    vm_area_entry->vm_flags = flags;

    dlist_init(&vm_area_entry->list_entry);

    // vmm_vmap(va_start, SIZE_TO_PAGE(va_end - va_start), flags);

    return vm_area_entry;
}


void *vmalloc(mm_struct_t *mm, void *ptr, size_t size, u64 flags) {
    vm_area_t *vm_area = null;
    u64 old_cr3;
    u64 switch_flags;
    uptr va_start = 0, va_end = 0;

    if (size == 0)
        return null;

    size = PAGE_ALIGN_UP_IF(size);
    // size += PAGE_SIZE;

    if (size >= mm_get_vmarea_start(flags)) {
        return null;
    }

    if (mm == null) {
        spin_lock(&k_vmlist_lock);
        vm_area = mm_find_vm_area(&k_vmlist, ptr, size, flags);
        spin_unlock(&k_vmlist_lock);
    } else {
        mm_switch_save(mm->page_dir_pa, old_cr3, switch_flags);

        spin_lock(&mm->vm_area_lock);
        vm_area = mm_find_vm_area(&mm->vm_area_head, ptr, size, flags);
        spin_unlock(&mm->vm_area_lock);
    }

    if  (vm_area) {
        va_start = vm_area->vm_start;
        va_end = vm_area->vm_end;
        vmm_vmap((void *)va_start, SIZE_TO_PAGE(va_end - va_start), flags);
    }

    if (mm) {
        mm_switch_restore(old_cr3, switch_flags);
    }

    return (void *)va_start;
}

void *vfree(mm_struct_t *mm, void *ptr) {
    u64 old_cr3;
    u64 switch_flags;
    vm_area_t *vm_area = null;

    if (mm == null) {
        spin_lock(&k_vmlist_lock);
        vm_area = mm_get_vm_area(&k_vmlist, ptr);
        spin_unlock(&k_vmlist_lock);
    } else {
        mm_switch_save(mm->page_dir_pa, old_cr3, switch_flags);

        spin_lock(&mm->vm_area_lock);
        vm_area = mm_get_vm_area(&mm->vm_area_head, ptr);
        spin_unlock(&mm->vm_area_lock);
    }

    vmm_vunmap((void *)vm_area->vm_start, SIZE_TO_PAGE(vm_area->vm_end - vm_area->vm_start), vm_area->vm_flags);

    if (mm) {
        mm_switch_restore(old_cr3, switch_flags);
    }

    slab_free(vm_area, null);
}

void dump_mm(mm_struct_t *mm) {
    int i = 0;

    if (mm == null) {
        spin_lock(&k_vmlist_lock);

        dlist_foreach(&k_vmlist, entry) {
            vm_area_t *vm_area = dlist_container_of(entry, vm_area_t, list_entry);
            klog_debug("%d  %p  %p  %d  %d\n", i, vm_area->vm_start, vm_area->vm_end, SIZE_TO_PAGE(vm_area->vm_end - vm_area->vm_start), vm_area->vm_flags);
            i++;
        }

        spin_unlock(&k_vmlist_lock);
    } else {
        spin_lock(&mm->vm_area_lock);
        
        dlist_foreach(&mm->vm_area_head, entry) {
            vm_area_t *vm_area = dlist_container_of(entry, vm_area_t, list_entry);
            klog_debug("%d  %p  %p  %d  %d\n", i, vm_area->vm_start, vm_area->vm_end, SIZE_TO_PAGE(vm_area->vm_end - vm_area->vm_start), vm_area->vm_flags);
            i++;
        }
        
        spin_unlock(&mm->vm_area_head);
    }

    klog_debug("\n");
}
