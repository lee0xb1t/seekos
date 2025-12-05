#include <mm/kmalloc.h>
#include <mm/mm.h>
#include <log/klog.h>
#include <panic/panic.h>
#include <lib/kmemory.h>
#include <lib/kmath.h>


void *kmalloc(size_t size) {
    if (size == 0) {
        return null;
    }

    if (size <= SLAB_ALLOC_MAX_SIZE) {
        size_t size_up = (size % 8 == 0) ? size : ( (size + 8) & (~0x07) );
        if (size_up <= SLAB_ALLOC_MAX_SIZE) {
            return slab_alloc(size_up);
        }
    }

    size = PAGE_ALIGN_UP_IF(size);
    return vmalloc(null, null, size, VMM_FLAGS_DEFAULT);
}

void kfree(void *ptr) {
    bool is_slab = false;

    slab_free(ptr, &is_slab);

    if (!is_slab) {
        vfree(null, ptr);
    }
}

void *kzalloc(size_t size_of_bytes) {
    void *ptr = kmalloc(size_of_bytes);
    memset(ptr, 0, size_of_bytes);
    return ptr;
}

void *krealloc(void *ptr, size_t old_size, size_t new_size) {
    void *newptr = kzalloc(new_size);
    if (ptr) {
        memcpy(newptr, ptr, min(old_size, new_size));
        kfree(ptr);
    }
    return newptr;
}
