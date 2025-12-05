#include <limine.h>
#include <lib/kmemory.h>
#include <lib/kcolor.h>
#include <lib/kstring.h>
#include <log/klog.h>
#include <com/serial.h>
#include <system/gdt.h>
#include <system/idt.h>
#include <system/madt.h>
#include <system/apic.h>
#include <system/smp.h>
#include <system/cpu.h>
#include <system/isr.h>
#include <system/cpuf.h>
#include <system/hpet.h>
#include <system/acpi.h>
#include <proc/sched.h>
#include <proc/kevent.h>
#include <proc/syscall.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <ia32/cpuinstr.h>
#include <device/display/fb.h>
#include <device/display/term.h>
#include <device/keyboard/keyboard.h>
#include <device/storage/ata.h>
#include <fs/vfs.h>
#include <fs/ttyfs.h>
#include <fs/ramfs.h>
#include <fs/fat32.h>


// Set the base revision to 4, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile u64 limine_base_revision[] = LIMINE_BASE_REVISION(4);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 4
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 4
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 4
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_file_request exec_request = {
    .id = LIMINE_EXECUTABLE_FILE_REQUEST_ID,
    .revision = 4
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 4
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_address_request execaddr_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
    .revision = 4
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 4
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile u64 limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile u64 limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;


void kernel_main() {
    serial_init();

    klogi("\n\n[*] Enter kernel!\n");

    if (hhdm_request.response) {
        klogi("HHDM offset: %p\n", hhdm_request.response->offset);
    }

    if (exec_request.response) {
        klogi("Executable File path: %s, base: %p, size: %p\n", 
            exec_request.response->executable_file->path,
            exec_request.response->executable_file->address,
            exec_request.response->executable_file->size
        );
    }

    klogi("Internal Module count: %d\n", module_request.internal_module_count);
    if (module_request.response) {
        klogi("Module count: %d\n", module_request.response->module_count);
        for (int i = 0; i < module_request.response->module_count; i++) {
            klogi("\tModule(%d) path: %s, addr: %p, size: %p, string : %s\n", i, 
                module_request.response->modules[i]->path,
                module_request.response->modules[i]->address,
                module_request.response->modules[i]->size,
                module_request.response->modules[i]->string
            );
        }
    }

    klogi("Executable Address va: %p, pa: %p\n", 
        execaddr_request.response->virtual_base, 
        execaddr_request.response->physical_base
    );

    cpu_feature_init();
    cpuf_info_init();

    cpu_ctrl_t *cpu_ctrl = cpu_init(0);

    cpu_gs_init(cpu_ctrl);

    gdt_init(cpu_ctrl);

    isr_init0();

    idt_init();
    idt_load();
    
    pmm_init(memmap_request.response);

    pmm_dump_usage(true);

#ifdef _TEST_CASE
    #define TEST_COUNT      200
    #define TEST_COUNT2     100
    uptr addrs[TEST_COUNT] = {0};
    for (int j = 0; j < TEST_COUNT2; j++) {
        for (int i = 0; i < TEST_COUNT; i++) {
            addrs[i] = pmm_alloc_pages(i + 1 + j);
        }
        for (int i = 0; i < TEST_COUNT; i++) {
            pmm_free_pages(addrs[i]);
        }
    }
    pmm_dump_usage(true);
#endif//_TEST_CASE
    

    ptm_init(memmap_request.response, execaddr_request.response);

    pmm_dump_usage(true);

    vmm_init();

    vmalloc_init();

#ifdef _TEST_CASE
    void *ptr1 = vmalloc(null, null, 33, VMM_FLAGS_DEFAULT);
    void *ptr2 = vmalloc(null, null, 33, VMM_FLAGS_DEFAULT);
    memcpy(ptr1, ptr2, 33);

    void *ptrs[100] = {0};
    for (int i = 0; i < 100; i++) {
        size_t size = ((i % 10) + 1) * PAGE_SIZE * 10;
        ptrs[i] = vmalloc(null, null, size, VMM_FLAGS_DEFAULT);
        memset(ptrs[i], 0, size);
    }
    dump_mm(null);
    
    for (int i = 0; i < 50; i++) {
        vfree(null, ptrs[i]);
    }
    dump_mm(null);

    for (int i = 100; i > 50; i--) {
        vfree(null, ptrs[i - 1]);
    }
    dump_mm(null);

    vmm_dump_slab();
    void *ptrs1[100] = {0};
    for (int i = 0; i < 27; i++) {
        ptrs1[i] = kmalloc(64);
        memset(ptrs1[i], 0, 64);
    }
    vmm_dump_slab();
    for (int i = 0; i < 27; i++) {
        kfree(ptrs1[i]);
    }
    vmm_dump_slab();
    klogd("\n");
#endif//_TEST_CASE

    fb_init(framebuffer_request.response);
    term_init(TR_FRAMEBUFFER);
    term_clear();
    klog_set_mode(KLOG_TERM);


#ifdef _TEST_CASE
    klogd("this is term!\n");
    klog_warn("this is warn!\n");
    klog_error("this is error!\n");

    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < 20; i++) {
            term_putch(0, 'A');
            term_refresh();
        }
        for (int i = 0; i < 20; i++) {
            term_back();
            term_refresh();
        }
    }
    klogd("\n");
#endif//_TEST_CASE

    acpi_init(rsdp_request.response);

    madt_init();
    apic_init();
    ioapic_init();

    isr_init();

    hpet_init();

#ifdef _TEST_CASE
    for (int i = 0; i < 2; i++) {
        hpet_sleep_millis(500);
        klogd("0.5s is gone!\n");
    }
#endif//_TEST_CASE

    tsc_init();

#ifdef _TEST_CASE
    for (int i = 0; i < 2; i++) {
        tsc_sleep_millis(500);
        klogd("0.5s is gone!\n");
    }
#endif//_TEST_CASE

    //

    syscall_init();

    ata_init();

    vfs_init();
    ttyfs_init();
    fat32_init();

    if (module_request.response) {
        for (int i = 0; i < module_request.response->module_count; i++) {
            if (strcmp(module_request.response->modules[i]->string, "INITRD")) {
                void *module_addr = module_request.response->modules[i]->address;
                size_t module_size = module_request.response->modules[i]->size;
                vmm_map(module_addr, __VIRT_TO_PHYS(module_addr), SIZE_TO_PAGE(module_size), VMM_FLAGS_DEFAULT);
                ramfs_init( module_addr, module_size );
                break;
            }
        }
    }

    vfs_mount_fs("/", "fat32");
    vfs_mount_fs("/dev/ttyfs", "ttyfs");

// #ifdef _TEST_CASE
//     // vfs_inode_t *inode = vfs_resolve_path("/", R_CREATE, VFS_NODE_DIRECTOR);
//     // klogd("inode->name: %s\n", inode->name);
//     // vfs_inode_t *inode1 = vfs_resolve_path("/dev", R_CREATE, VFS_NODE_DIRECTOR);
//     // klogd("inode1->name: %s\n", inode1->name);
//     // vfs_inode_t *inode2 = vfs_resolve_path("/dev", R_NO_CREATE, VFS_NODE_DIRECTOR);
//     // klogd("inode2->name: %s\n", inode2->name);
//     // vfs_inode_t *inode3 = vfs_resolve_path("/dev/ttyfs", R_CREATE, VFS_NODE_DIRECTOR);
//     // klogd("inode3->name: %s\n", inode3->name);
//     // char parent_path[100] = {0};
//     // vfs_get_parent("/dev/ttyfs", parent_path, 100);
//     // klogd("%s\n", parent_path);
//     // char parent_path1[100] = {0};
//     // vfs_get_parent("/dev", parent_path1, 100);
//     // klogd("%s\n", parent_path1);
// #endif//_TEST_CASE

    kevent_init();

    keyboard_init();

    sched_init();
#ifdef _TEST_CASE
    sched_test_init();
#endif

    //...
    smp_init();

    sched_execve("/bin/shell", 0, null, "/root");

    isr_enable_interrupts();

    for(;;) __hang();
}
