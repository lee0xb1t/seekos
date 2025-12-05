// TODO Restructure Memory Management
#include <mm/mm.h>
#include <log/klog.h>
#include <panic/panic.h>
#include <lib/kmemory.h>
#include <base/spinlock.h>
#include <ia32/cpuinstr.h>


/* variables */

/* Slab */
slab_size_t slab_size_list[] = {
    { "kmalloc-8", 8, 1 },
    { "kmalloc-16", 16, 1 },
    { "kmalloc-32", 32, 1 },
    { "kmalloc-64", 64, 1 },
    { "kmalloc-128", 128, 2 },
    { "kmalloc-256", 256, 2 },
    { "kmalloc-512", 512, 2 },
    { "kmalloc-1k", 1024, 4 },
    { "kmalloc-2k", 2048, 4 },
    { "kmalloc-4k", 4096, 4 },
    { "kmalloc-8k", 8192, 4 }
};

slab_cache_t meta_cache;
linked_list_t cache_cache_head;     /* slab_cache_t */

//spin_lock_t slab_lock;
DECLARE_SPINLOCK(slab_lock);


/* prototypes */



/* implements */

void vmm_init() {
    klog_debug("\n");

    
    ptm_alloc_pdptes((void *)MI_DIRECT_MAPPING_RANGE_START, MI_DIRECT_MAPPING_RANGE_SIZE / PAGE_SIZE, VMM_FLAGS_DEFAULT);
    // ptm_alloc_pdptes((void *)MI_APENTRY_LOWEST_START, MI_APENTRY_LOWEST_SIZE / PAGE_SIZE, VMM_FLAGS_DEFAULT);

    slab_init();

    klog_debug("VMM initialized.\n\n");
}

uptr vmm_get_phys_addr(void *va) {
    pte_t *pte = ptm_get_pte(va);
    return PAGE_TO_SIZE(pte->page_frame);
}

void vmm_map(void *va, uptr pa, size_t nr_pages, u64 flags) {
    ptm_map_pages(va, pa, nr_pages, flags);
}

void vmm_unmap(void *va, size_t nr_pages) {
    ptm_unmap_pages(va, nr_pages);
}

void vmm_vmap(void *va, size_t nr_pages, u64 flags) {
    if ((flags & VMM_FLAGS_MMIO) == VMM_FLAGS_MMIO) {
        pfn_number pfn = pmm_alloc_pages(nr_pages);
        vmm_map(va, pfn_to_page(pfn), nr_pages, VMM_FLAGS_MMIO);
    } else {
        uptr va_end = (uptr)va + PAGE_TO_SIZE(nr_pages);
        for (uptr va_start = (uptr)va; va_start < va_end; va_start += PAGE_SIZE) {
            vmm_map(va_start, pfn_to_page(pmm_alloc_page())->phys_addr, 1, flags);
        }
    }
}

void vmm_vunmap(void *va, size_t nr_pages, u64 flags) {
    if ((flags & VMM_FLAGS_MMIO) == VMM_FLAGS_MMIO) {
        vmm_unmap(va, nr_pages);
    } else {
        uptr va_end = (uptr)va + PAGE_TO_SIZE(nr_pages);
        for (uptr va_start = (uptr)va; va_start < va_end; va_start += PAGE_SIZE) {
            vmm_unmap((void *)va_start, 1);
        }
    }
}

void *vmm_alloc_page() {
    return vmm_alloc_pages(1);
}

void vmm_free_page(void *ptr) {
    vmm_free_pages(ptr, 1);
}

void *vmm_alloc_pages(size_t nr_pages) {
    pfn_number pfn = pmm_alloc_pages(nr_pages);
    uptr phys_addr = pfn_to_ptr(pfn);
    uptr virt_addr = MI_DIRECT_MAPPING_RANGE_START + phys_addr;
    ptm_map_pages((void *)virt_addr, phys_addr, nr_pages, VMM_FLAGS_DEFAULT);
    return (void *)virt_addr;
}

void vmm_free_pages(void *ptr, size_t nr_pages) {
    if (ptr == null)
        return;
    pfn_number pfn = (pfn_number)(((uptr)ptr - MI_DIRECT_MAPPING_RANGE_START) >> PAGE_SHIFT);
    ptm_unmap_pages(ptr, nr_pages);
    // pmm_free_pages(pfn);
}

void slab_init() {
    slab_cache_init(&meta_cache, "slab_meta_cahce", sizeof(slab_cache_t), SLAB_META_CACHE_NRPAGES);
    slab_cache_growth(&meta_cache);
    dlist_init(&cache_cache_head);

    int size_count = sizeof(slab_size_list) / sizeof(slab_size_t);
    for (int i = 0; i < size_count; i++) {
        slab_size_t *slab_size = &slab_size_list[i];
        slab_cache_t *slab_cache = slab_cache_create_cache(slab_size->name, slab_size->obj_size, slab_size->nr_pages);
        slab_cache_growth(slab_cache);
        dlist_add_prev(&cache_cache_head, &slab_cache->list);
        klog_debug("Slab %s initialized!\n", slab_size_list[i].name);
    }

}

slab_t * slab_create(u32 nr_pages, u32 obj_size, u32 *obj_count_out) {
    *obj_count_out = 0;

    slab_t *slab_start = (slab_t *)vmm_alloc_pages(nr_pages);

    if (slab_start == null) return null;

    size_t slab_bytes = nr_pages * PAGE_SIZE;
    memset(slab_start, 0, slab_bytes);

    u32 obj_count = (slab_bytes - sizeof(slab_t)) / obj_size;
    slab_entry_t *obj_start = (slab_entry_t *)((uptr)slab_start + sizeof(slab_t));

    slab_entry_t *curr_entry = obj_start;
    for (uptr i = 1; i < obj_count; i++) {
        slab_entry_t *slab_entry = (slab_entry_t *)((uptr)obj_start + i * obj_size);
        curr_entry->next = slab_entry;
        curr_entry = curr_entry->next;
    }

    slab_start->s_mem = obj_start;
    slab_start->free_objects = obj_count;
    slab_start->size = slab_bytes;
    slab_start->free_list = obj_start;

    dlist_init(&slab_start->slab_list);

    *obj_count_out = obj_count;

    return slab_start;
}

void slab_cache_init(slab_cache_t *cache, char *name, u32 obj_size, u32 nr_pages) {
    memset(cache, 0, sizeof(slab_cache_t));
    dlist_init(&cache->list);
    dlist_init(&cache->slabs.slab_list);

    cache->obj_size = obj_size;
    cache->nr_pages = nr_pages;

    memcpy(cache->name, name, SLAB_NAME_LENGTH);
}

void slab_cache_growth(slab_cache_t *cache) {
    u32 obj_count = 0;

    slab_t *growth_slab = slab_create(cache->nr_pages, cache->obj_size, &obj_count);
    dlist_add_next(&cache->slabs.slab_list, &growth_slab->slab_list);

    cache->free_objects += obj_count;
    cache->total_objects += obj_count;
    cache->total_pages += cache->nr_pages;
}

void *slab_cache_alloc(slab_cache_t *cache, size_t size_of_bytes) {

    if (cache->obj_size < size_of_bytes) {
        return null;
    }

    if (cache->free_objects == 0) {
        slab_cache_growth(cache);
    }

    slab_t *slabs = &cache->slabs;
    dlist_foreach(&slabs->slab_list, entry) {

        slab_t *slab_entry = dlist_container_of(entry, slab_t, slab_list);

        if (slab_entry->free_objects > 0) {
            slab_entry_t *free_object = slab_entry->free_list;
            slab_entry->free_list = slab_entry->free_list->next;

            slab_entry->free_objects--;
            cache->free_objects--;

            free_object->next = null;
            return free_object;
        }
    }
    
    return null;
}

bool slab_cache_free(slab_cache_t *cache, void *ptr) {
    slab_t *slabs = &cache->slabs;
    dlist_foreach(&slabs->slab_list, entry) {

        slab_t *slab_entry = dlist_container_of(entry, slab_t, slab_list);

        uptr range_start = (uptr)slab_entry->s_mem;
        uptr range_end = (uptr)slab_entry->s_mem + slab_entry->size;

        if ((uptr)ptr >= range_start && (uptr)ptr < range_end) {
            slab_entry_t *free_object = (slab_entry_t *)ptr;

            free_object->next = slab_entry->free_list;
            slab_entry->free_list = free_object;

            slab_entry->free_objects++;
            cache->free_objects++;
            return true;
        }
    }

    return false;
}

slab_cache_t *slab_cache_create_cache(char *name, u32 obj_size, u32 nr_pages) {
    // allocate from meta_cache
    slab_cache_t *slab_cache = (slab_cache_t *)slab_cache_alloc(&meta_cache, sizeof(slab_cache_t));

    if (slab_cache == null) {
        return null;
    }

    slab_cache_init(slab_cache, name, obj_size, nr_pages);

    return slab_cache;
}

void *slab_alloc(size_t size) {
    spin_lock(&slab_lock);

    dlist_foreach(&cache_cache_head, entry) {
        slab_cache_t *cache_entry = dlist_container_of(entry, slab_cache_t, list);

        if (size <= cache_entry->obj_size) {
            spin_unlock(&slab_lock);
            return slab_cache_alloc(cache_entry, size);
        }
    }

    spin_unlock(&slab_lock);

    return null;
}

void slab_free(void *ptr, bool *is_slab) {
    if (is_slab)
         *is_slab = false;

    spin_lock(&slab_lock);

    dlist_foreach(&cache_cache_head, entry) {
        slab_cache_t *cache_entry = dlist_container_of(entry, slab_cache_t, list);

        if (slab_cache_free(cache_entry, ptr)) {
            if (is_slab)
                *is_slab = true;
            break;
        }
    }

    spin_unlock(&slab_lock);
}

void vmm_dump_slab() {
    klog_debug("name  obj_size  free_objects  total_objects  nr_pages  total_pages\n");
    spin_lock(&slab_lock);
    dlist_foreach(&cache_cache_head, entry) {
        slab_cache_t *cache_entry = dlist_container_of(entry, slab_cache_t, list);
        klog_debug("%s\t%d\t%d\t%d\t%d\t%d\n", cache_entry->name, cache_entry->obj_size, cache_entry->free_objects, cache_entry->total_objects, cache_entry->nr_pages, cache_entry->total_pages);
    }
    spin_unlock(&slab_lock);
}


