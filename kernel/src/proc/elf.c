#include <proc/elf.h>
#include <base/spinlock.h>
#include <fs/vfs.h>
#include <mm/kmalloc.h>
#include <lib/kstring.h>
#include <lib/kmemory.h>
#include <log/klog.h>


DECLARE_SPINLOCK(elf_lock);


i32 elf_load(task_t *t, const char* path) {
    u64 flags;

    spin_lock_irq(&elf_lock, flags);

    vfs_handle_t fh = vfs_open(path, VFS_MODE_READWRITE);
    if (fh == VFS_INVALID_HANDLE) {
        spin_unlock_irq(&elf_lock, flags);
        return -1;
    }

    int fsize = vfs_lseek(fh, 0, SEEK_END);
    if (fsize == 0) {
        spin_unlock_irq(&elf_lock, flags);
        return -1;
    }

    vfs_lseek(fh, 0, SEEK_SET);

    u8 *fbuff = (u8 *)kzalloc(fsize+1);
    i32 rflag = vfs_read(fh, fsize, fbuff);
    if (rflag == -1) {
        spin_unlock_irq(&elf_lock, flags);
        return -1;
    }

    elf64_ehdr_t *ehdr = (elf64_ehdr_t *)fbuff;
    if (*(u32 *)ehdr->e_ident != ELFMAGIC) {
        spin_unlock_irq(&elf_lock, flags);
        return -1;
    }

    for (int i = 0; i < ehdr->e_phnum; i++) {
        elf64_phdr_t *phdr = elf_program(ehdr, i);

        switch (phdr->type)
        {
        case PT_LOAD:
            if (phdr->memsz && phdr->filesz) {
                void *pdata = vmalloc(t->mm, phdr->vaddr, phdr->memsz, VMM_FLAGS_USER);
                memcpy(pdata, elf_program_data(ehdr, i), phdr->filesz);
            }
            break;
        
        default:
            break;
        }
    }

    void *ustack = vmalloc(t->mm, null, TASK_STACK_SIZE_32KB, VMM_FLAGS_USER);
    task_setup_ustack(t, ustack, TASK_STACK_SIZE_32KB);
    task_setup_routine(t, ehdr->e_entry);

    t->auxv.entry = ehdr->e_entry;
    // TODO
    t->auxv.phdr = 0;
    t->auxv.phent = 0;
    t->auxv.phnum = 0;

    kfree(fbuff);

    spin_unlock_irq(&elf_lock, flags);
    return 0;
}

inline elf64_shdr_t *elf_section(elf64_ehdr_t *ehdr, int idx) {
    return ((elf64_phdr_t*)((uintptr_t)(ehdr) + (ehdr)->e_shoff + (idx) * (ehdr)->e_shentsize));
}

inline elf64_phdr_t *elf_program(elf64_ehdr_t *ehdr, int idx) {
    return ((elf64_phdr_t*)((uintptr_t)(ehdr) + (ehdr)->e_phoff + (idx) * (ehdr)->e_phentsize));
}

inline void *elf_program_data(elf64_ehdr_t *ehdr, int idx) {
    elf64_phdr_t *phdr = elf_program(ehdr, idx);
    return ((void *)((uintptr_t)(ehdr) + phdr->offset));
}
