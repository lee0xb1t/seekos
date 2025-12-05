#include <mm/mm.h>
#include <log/klog.h>
#include <panic/panic.h>
#include <system/klayout.h>
#include <lib/kmemory.h>
#include <lib/kmath.h>
#include <ia32/cpuinstr.h>


// static addr_space_t kaddr_space;

// static zone_t zones[ZONE_MAX_COUNT] = { 0 };


void _kmem_init(struct limine_memmap_response *mm);

void _early_reserved_mem_init();
void _early_mark_reserved(pfn_number pfn, uptr nr_pages);
void _early_reverse_first_page();


//
// Bitmap
//
void _bitmap_init(struct limine_memmap_response *mm);
void _bitmap_free_pages(struct limine_memmap_response *mm);

void _bitmap_set(pfn_number pfn, size_t nr_pages);
void _bitmap_clear(pfn_number pfn, size_t nr_pages);
bool _bitmap_is_clear(pfn_number pfn, size_t nr_pages);
bool _bitmap_is_set(pfn_number pfn, size_t nr_pages);

void _bitmap_mark_page(pfn_number pfn);
void _bitmap_clear_page(pfn_number pfn);

bool _bitmap_alloc_frames(size_t nr_pages, pfn_number *pfn);
void _bitmap_free_frames(pfn_number pfn, size_t nr_pages);


//
// Pfn
//
void _pfn_database_init(struct limine_memmap_response *mm);
void _pfn_page_init(pfn_number pfn, u32 attrs);


//
// Buddy Allocator
//
void _buddy_init();
page_t *_buddy_alloc_pages(page_t *base, size_t nr_pages);
void _buddy_free_pages(page_t *page, page_t *base, int order);
bool _buddy_page_is_buddy(page_t *page, page_t *buddy, int order);
void _buddy_clear_page_buddy(page_t *page);
void _buddy_set_page_buddy(page_t *page);
void _buddy_split(page_t *page, int low, int high);
void _buddy_add_to_free_list(page_t *page, int order);
page_t *_buddy_get_from_free_area(struct free_area_t *area, int order);
void _buddy_set_page_order(page_t *page, int order);


void pmm_init(struct limine_memmap_response *mm) {
    for (int i = 0; i < mm->entry_count; i++) {
        klogi(
            "base: %p, size: %d, type: %d\n",
            mm->entries[i]->base,
            mm->entries[i]->length,
            mm->entries[i]->type
        );
    }

    _kmem_init(mm);
    _bitmap_init(mm);
    _pfn_database_init(mm);
    
    _bitmap_free_pages(mm);
    _early_reverse_first_page();

    _buddy_init();

    spin_init(&kmem.pfn_lock);

    klog_debug("PMM initialized.\n\n");
}


pfn_number pmm_alloc_page() {
    return pmm_alloc_pages(1);
}


void pmm_free_page(pfn_number pfn) {
    pmm_free_pages(pfn);
}


pfn_number pmm_alloc_pages(size_t nr_pages) {
    u64 flags;
    if (nr_pages == 0)
        return null;

    if (nr_pages > kmem.nr_free_pages) {
        panic("[PMM] not enough free physical memory");
    }

    spin_lock_irq(&kmem.pfn_lock, flags);
    pfn_number pfn =  page_to_pfn(_buddy_alloc_pages(&kmem.pfn_database_start, nr_pages));
    spin_unlock_irq(&kmem.pfn_lock, flags);
    return pfn;
}


void pmm_free_pages(pfn_number pfn) {
    u64 flags;
    spin_lock_irq(&kmem.pfn_lock, flags);
    page_t *page = pfn_to_page(pfn);
    _buddy_free_pages(page, kmem.pfn_database_start, page->order);
    spin_unlock_irq(&kmem.pfn_lock, flags);
}


void pmm_dump_usage(bool brief) {
    u64 flags;
    spin_lock_irq(&kmem.pfn_lock, flags);

    size_t totalmem = PAGE_TO_SIZE(kmem.nr_pages);
    size_t freemem = PAGE_TO_SIZE(kmem.nr_free_pages);
    size_t usedmem = totalmem - freemem;
    size_t total_pages = kmem.nr_pages;
    size_t free_pages = kmem.nr_free_pages;
    
    spin_unlock_irq(&kmem.pfn_lock, flags);

    klog_debug("\nDump memory usgae:\n");
    klog_debug("  Total memory:%qkb (%qmb)\n", totalmem / 1024, totalmem / 1024 / 1024);
    klog_debug("  Free memory:%qkb (%qmb)\n", freemem / 1024, freemem / 1024 / 1024);
    klog_debug("  Used memory:%qkb (%qmb)\n", usedmem / 1024, usedmem / 1024 / 1024);
    klog_debug("  Total pages:%q\n", total_pages);
    klog_debug("  Free pages:%q\n", free_pages);

    uptr bitmap_total = 0;
    for (int i = 0; i < kmem.bitmap_count; i++) {
        u64 bitmap = kmem.bitmap_start[i];
        for (int bit_idx = 0; bit_idx < BITS_PER_U64; bit_idx++) {
            pfn_number pfn = (i * BITS_PER_U64) + bit_idx;
            if ((bitmap & (1ull << bit_idx)) == 0)
                bitmap_total++;
        }
    }
    klog_debug("  Free bitmap pages:%q\n", bitmap_total);

    size_t buddy_free = 0;
    for (int order = 0; order < MM_MAX_ORDER; order++) {
        uptr nr_free = kmem.buddy_free_area[order].nr_free;
        klog_debug("  order: %d, nr_free: %d, nr_pages: %d\n", order, nr_free, nr_free * (1ull << order));
        if (!brief) {
            dlist_foreach(&kmem.buddy_free_area[order].list_head, entry) {
                page_t *page = dlist_container_of(entry, page_t, list_entry);
                klog_debug("    phys_addr: %p, in_buddy: %d, order: %d\n", page->phys_addr, page->in_buddy, page->order);
            }
        }
        buddy_free += nr_free * (1ull << order);
    }
    klog_debug("  Free buddy pages:%q\n", buddy_free);
}


void _bitmap_set(pfn_number pfn, size_t nr_pages) {
    for (size_t i = 0; i < nr_pages; i++) {
        u32 idx = (pfn + i) / 64;
        u32 bit_idx = (pfn + i) % 64;
        u64 *bm = &kmem.bitmap_start[idx];
        *bm = (*bm) | ((u64)1 << bit_idx);
    }
}


void _bitmap_clear(pfn_number pfn, size_t nr_pages) {
    for (size_t i = 0; i < nr_pages; i++) {
        u32 idx = (pfn + i) / BITS_PER_U64;
        u32 bit = (pfn + i) % BITS_PER_U64;
        u64 *bm = &kmem.bitmap_start[idx];
        *bm = (*bm) & (~(1ull << bit));
    }
}


bool _bitmap_is_clear(pfn_number pfn, size_t nr_pages) {
    for (size_t i = 0; i < nr_pages; i++) {
        u32 idx = (pfn + i) / 64;
        u32 bit = (pfn + i) % 64;
        u64 *bm = &kmem.bitmap_start[idx];
        if ( ((*bm) & ((u64)1 << bit)) ) {
            return false;
        }
    }
    return true;
}


bool _bitmap_is_set(pfn_number pfn, size_t nr_pages) {
    for (size_t i = 0; i < nr_pages; i++) {
        u32 idx = (pfn + i) / 64;
        u32 bit = (pfn + i) % 64;
        u64 *bm = &kmem.bitmap_start[idx];
        if ( ((*bm) & ((u64)1 << bit)) == 0 ) {
            return false;
        }
    }
    return true;
}


void _kmem_init(struct limine_memmap_response *mm) {
    size_t total_size = 0;
    size_t nr_pages_total = 0;
    uptr phys_limit = 0;

    for (size_t i = 0; i < mm->entry_count; i++) {
        if (mm->entries[i]->type == LIMINE_MEMMAP_USABLE) {
            total_size += mm->entries[i]->length;
        }
        phys_limit = mm->entries[i]->base + mm->entries[i]->length;
        phys_limit = PAGE_ALIGN_UP_IF(phys_limit);
    }

    nr_pages_total = SIZE_TO_PAGE(total_size);

    kmem.nr_pages = nr_pages_total;
    kmem.nr_free_pages = 0;
    kmem.phys_limit = phys_limit;
}


void _bitmap_init(struct limine_memmap_response *mm) {
    // find bitmap start
    size_t bitmap_count = ((kmem.phys_limit + BITMAP_PER_U64) & (~BITMAP_PER_U64)) / BITMAP_PER_U64;
    size_t bitmap_size = bitmap_count * BYTES_PER_U64;
    uptr bitmap_start_pa = 0;

    for (size_t i = 0; i < mm->entry_count; i++) {
        if (mm->entries[i]->type == LIMINE_MEMMAP_USABLE) {
            if (mm->entries[i]->length > bitmap_size) {
                bitmap_start_pa = mm->entries[i]->base + mm->entries[i]->length - bitmap_size;
                mm->entries[i]->length -= bitmap_size;
                break; 
            }
        }
    }

    u64 *bitmap_start = (u64 *)__PHYS_TO_VIRT(bitmap_start_pa);
    uptr bitmap_end = ((uptr)bitmap_start) + bitmap_size;

    kmem.bitmap_start = bitmap_start;
    kmem.bitmap_pa = bitmap_start_pa;
    kmem.bitmap_count = bitmap_count;
    kmem.bitmap_end = bitmap_end;
    kmem.bitmap_start_of_search = 0;

    for (int i = 0; i < bitmap_count; i++) {
        bitmap_start[i] = U64_MAX;
    }
}


void _bitmap_free_pages(struct limine_memmap_response *mm) {
    for (size_t i = 0; i < mm->entry_count; i++) {
        if (mm->entries[i]->type == LIMINE_MEMMAP_USABLE) {
            _bitmap_clear(mm->entries[i]->base >> PAGE_SHIFT, SIZE_TO_PAGE(mm->entries[i]->length));
        }
    }
}


bool _bitmap_alloc_frames(size_t nr_pages, pfn_number *pfn) {
    uptr phys_limit = kmem.phys_limit;

    for (uptr i = 0; i < phys_limit; i++) {
        //uptr phys_addr = i << PAGE_SHIFT;
        if (_bitmap_is_clear(i, 1)) {
            if (_bitmap_is_clear(i, nr_pages)) {
                _bitmap_set(i, nr_pages);
                *pfn = i;
                return true;
            }
        }
    }
    return false;
}

void _bitmap_free_frames(pfn_number pfn, size_t nr_pages) {
    _bitmap_clear(pfn, nr_pages);
}


void _bitmap_mark_page(pfn_number pfn) {
    _bitmap_set(pfn, 1);
}


void _bitmap_clear_page(pfn_number pfn) {
    _bitmap_clear(pfn, 1);
}


void _pfn_database_init(struct limine_memmap_response *mm) {
    size_t pfn_db_size = SIZE_TO_PAGE(kmem.phys_limit) * sizeof(page_t);
    pfn_db_size = PAGE_ALIGN_UP_IF(pfn_db_size);

    uptr pfn_db_start_pa = 0;

    for (size_t i = 0; i < mm->entry_count; i++) {
        if (mm->entries[i]->type == LIMINE_MEMMAP_USABLE) {
            if (mm->entries[i]->length > pfn_db_size) {
                pfn_db_start_pa = mm->entries[i]->base + mm->entries[i]->length - pfn_db_size;
                mm->entries[i]->length -= pfn_db_size;
                break; 
            }
        }
    }

    page_t *pfn_db_start = __PHYS_TO_VIRT(pfn_db_start_pa);
    uptr pfn_db_end = (uptr)pfn_db_start + pfn_db_size;

    kmem.pfn_database_start = __PHYS_TO_VIRT(pfn_db_start_pa);
    kmem.pfn_database_pa = pfn_db_start_pa;
    kmem.pfn_database_bytes = pfn_db_size;
    kmem.pfn_database_end = pfn_db_end;

    for (uptr pfn = 0; pfn < SIZE_TO_PAGE(kmem.phys_limit); pfn++) {
        _pfn_page_init(pfn, 0);
    }
}


void _pfn_page_init(pfn_number pfn, u32 attrs) {
    kmem.pfn_database_start[pfn].phys_addr = pfn << PAGE_SHIFT;
    kmem.pfn_database_start[pfn].refcount = 0;
    kmem.pfn_database_start[pfn].flags = 0;
    kmem.pfn_database_start[pfn].order = 0;
    dlist_init(&kmem.pfn_database_start[pfn].list_entry);
}

void _early_reverse_first_page() {
    // In limine, first page always reversed.
}

void _buddy_init() {
    for (int order = 0; order < MM_MAX_ORDER; order++) {
        kmem.buddy_free_area[order].nr_free = 0;
        dlist_init(&kmem.buddy_free_area[order].list_head);
    }

    for (size_t i = 0; i < kmem.bitmap_count; i++) {
        u64 bitmap = kmem.bitmap_start[i];
        
        // bitmap = 0说明该bitmap指向的范围内存未使用
        if (bitmap == 0) {
            pfn_number pfn = i * BITS_PER_U64;
            _buddy_free_pages(pfn_to_page(pfn), kmem.pfn_database_start, ffs(BITS_PER_U64));
        } else {
            for (int bit_idx = 0; bit_idx < BITS_PER_U64; bit_idx++) {
                pfn_number pfn = (i * BITS_PER_U64) + bit_idx;
                if ((bitmap & (1ull << bit_idx)) == 0)
                    _buddy_free_pages(pfn_to_page(pfn), kmem.pfn_database_start, 0);
            }
        }
    }
    
}

page_t *_buddy_alloc_pages(page_t *base, size_t nr_pages) {
    struct free_area_t *free_area = null;

    if (nr_pages > (1ull << (MM_MAX_ORDER - 1))) {
        return null;
    }

    int order = 0;

    for (int i = 0; i < MM_MAX_ORDER; i++) {
        if ( nr_pages <= (1ull << i) ) {
            order = i;
            break;
        }
    }

    if (order > MM_MAX_ORDER - 1) {
        return null;
    }

    for (int curr_order = order; curr_order < MM_MAX_ORDER; curr_order++) {
        free_area = &kmem.buddy_free_area[curr_order];

        page_t *page = _buddy_get_from_free_area(free_area, curr_order);
        if (!page) 
            continue;

        _buddy_split(page, order, curr_order);

        _buddy_set_page_order(page, order);
        
        return page;
    }

    return null;
}

void _buddy_free_pages(page_t *page, page_t *base, int order) {
    uptr page_idx = 0;
    uptr buddy_idx  = 0;
    uptr combined_idx = 0;
    page_t *buddy = null;

    int new_order = order;

    page_idx = page_to_pfn(page);

    while (new_order < MM_MAX_ORDER - 1) {
        buddy_idx = page_idx ^ (1ull << new_order);
        buddy = base + buddy_idx;
        
        if (!_buddy_page_is_buddy(page, buddy, new_order)) {
            break;
        }

        dlist_remove_entry(&buddy->list_entry);
        kmem.buddy_free_area[new_order].nr_free--;
        _buddy_clear_page_buddy(buddy);
        _buddy_set_page_order(buddy, 0);

        kmem.nr_free_pages -= (1ull << new_order);

        combined_idx = page_idx & buddy_idx;
        page = base + combined_idx;
        page_idx = page_to_pfn(page);
        new_order++;
    }

    _buddy_set_page_buddy(page);
    _buddy_set_page_order(page, new_order);
    dlist_add_next(&kmem.buddy_free_area[new_order].list_head, &page->list_entry);
    kmem.buddy_free_area[new_order].nr_free++;
    
    kmem.nr_free_pages += (1ull << new_order);
}

bool _buddy_page_is_buddy(page_t *page, page_t *buddy, int order) {
    pfn_number page_pfn = page_to_pfn(page);
    pfn_number buddy_pfn = page_to_pfn(buddy);

    if ((page_pfn ^ buddy_pfn) != (1ull << order)) {
        return false;
    }


    if(buddy_pfn << PAGE_SHIFT >= kmem.phys_limit) {
        return false;
    }

    return (buddy->in_buddy == 1 && buddy->order == order);
}

void _buddy_clear_page_buddy(page_t *page) {
    page->in_buddy = 0;
}

void _buddy_set_page_buddy(page_t *page) {
    page->in_buddy = 1;
}

void _buddy_split(page_t *page, int low, int high) {
    size_t size = 1ull << high;

    while(high > low) {
        high--;
        size >>= 1;

        _buddy_add_to_free_list(page + size, high);

        kmem.nr_free_pages += (1ull << high);
    }
}

void _buddy_add_to_free_list(page_t *page, int order) {
    volatile struct free_area_t *free_area = &kmem.buddy_free_area[order];
    dlist_add_prev(&free_area->list_head, &page->list_entry);
    _buddy_set_page_buddy(page);
    _buddy_set_page_order(page, order);
    free_area->nr_free++;
}

page_t *_buddy_get_from_free_area(struct free_area_t *area, int order) {
    if (area == null) {
        return null;
    }

    if (area->nr_free == 0) {
        return null;
    }

    page_t *page = null;
    linked_list_t *entry = null;

    dlist_remove_next(&area->list_head, &entry);
    if (!entry)
        return null;

    page = dlist_container_of(entry, page_t, list_entry);

    if (page){
        _buddy_clear_page_buddy(page);
        area->nr_free--;
        
        kmem.nr_free_pages -= (1ull << order);
    }

    return page;
}

void _buddy_set_page_order(page_t *page, int order) {
    page->order = order;
}
