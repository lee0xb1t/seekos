#include <osloader/loader.h>
#include <display/graphics.h>
#include <fs.h>
#include <acpi.h>
#include <apic.h>
#include <hpet.h>
#include <xlib.h>


#define PAGE_SIZE   0x1000
#define PAGE_MASK   (PAGE_SIZE - 1)

#define KERNEL_START    0x1000000

#define MAX_MEMORY_MAP_SIZE     256


#define MEM_TYPE_AVAILABLE      0x0
#define MEM_TYPE_RESERVED       0x1
#define MEM_TYPE_ACPI           0x2
#define MEM_TYPE_NVS            0x3
#define MEM_TYPE_UNUSABLE       0x4

typedef struct {
    unsigned long long physical_address;
    int number_of_pages;
    unsigned int type;
} __attribute__((packed)) memory_map_desc_t;

typedef struct {
    i32 mapcount;
    u32 __dummy;
    u64 available_memory;
    u64 total_memory;
    u64 max_phys_addr;
    memory_map_desc_t mem_maps[MAX_MEMORY_MAP_SIZE];
} __attribute__((packed)) memory_desc_t;

typedef struct {
    void* framebuffer;
    unsigned int size;
    unsigned int width;
    unsigned int height;
    unsigned int red_mask;
    unsigned int green_mask;
    unsigned int blue_mask;
    unsigned int reversed_mask;
    unsigned int pixel_format;
} __attribute__((packed)) screen_info_t;

typedef struct {
    unsigned long long __dontuse;
    u64 kernel_start;
    screen_info_t screen_info;
    memory_desc_t mem_desc;
    u32 madt;
    u32 madt_size;
    /*hpet*/
    u32 hpet_event_timer_block;
    u32 hpet_address_structure;
    u64 hpet_address;
    u8 hpet_number;
    u16 minimum_tick;
    u8 page_protection;
} __attribute__((packed)) boot_params_t;


EFI_GUID FileInfoGuid = EFI_FILE_INFO_ID;


extern EFI_SYSTEM_TABLE *gST;
extern EFI_HANDLE gImageHandle;


void print_bootparams();

void LoadOs() {
    EFI_STATUS status;
    EFI_FILE *KernelFile;

    /* Load KERNEL */

    status = fs_open_file(L"\\EFI\\boot\\Kernel", &KernelFile);

    if (EFI_ERROR(status)) {
        printv("Failed to open \"Kernel\" file.\n");
        return;
    }

    EFI_FILE_INFO *FileInfo;
    UINTN FileInfoBufferSize = sizeof(EFI_FILE_INFO) + 0x1000;
    
    status = alloc_pool(EfiBootServicesData, FileInfoBufferSize, (void*)&FileInfo);

    if (EFI_ERROR(status)) {
        fs_close_file(KernelFile);
        printv("Failed to allocate FileInfo structure.\n");
        return;
    }

    status = KernelFile->GetInfo(KernelFile, &FileInfoGuid, &FileInfoBufferSize, FileInfo);

    if (EFI_ERROR(status)) {
        free_pool(FileInfo);
        fs_close_file(KernelFile);
        printv("Failed to get \"Kernel\" file info.\n");
        return;
    }

    printv("FileName: %ws, FileSize: %d\n", FileInfo->FileName, FileInfo->FileSize);


    UINTN KernelSize = FileInfo->FileSize;

    /* 4KB align */
    if ((KernelSize & 0xfff) != 0) {
        KernelSize += 0x1000;
        KernelSize &= ~(0xfff);
    }

    int KernelPages = KernelSize / PAGE_SIZE;

    EFI_PHYSICAL_ADDRESS start_pa = KERNEL_START;
    alloc_pages(AllocateAddress, EfiLoaderData, KernelPages, &start_pa);
    memzero((void*)start_pa, KernelSize);

    status = KernelFile->Read(KernelFile, &KernelSize, (void*)start_pa);

    if (EFI_ERROR(status)) {
        free_pages(start_pa, KernelPages);
        free_pool(FileInfo);
        fs_close_file(KernelFile);
        printv("Failed to read \"Kernel\" file.\n");
        return;
    }

    free_pool(FileInfo);
    fs_close_file(KernelFile);

    boot_params_t *params = (boot_params_t *)KERNEL_START;

    
    /* Load MADT */
    // madt_t *madt = null;
    // status = alloc_pool(EfiLoaderData, apic_get_madt_size(), &madt);
    // if (EFI_ERROR(status)) {
    //     printv("Failed to allocate MADT.\n");
    //     return;
    // }

    // memcpy(madt, apic_get_madt(), apic_get_madt_size());
    params->madt = (u32)apic_get_madt();
    params->madt_size = apic_get_madt_size();
    printv("apic_get_madt(): %p\n", apic_get_madt());


    /*Load hpet*/
    hpet_init();
    print_hpet();
    memcpy(&params->hpet_event_timer_block, &hpet_get_hpet()->hardware_rev_id, 20);


    /* Load Memory Map*/

    EFI_MEMORY_DESCRIPTOR* Map = NULL;
    UINTN MapSize, MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;
    gST->BootServices->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
    printv("MapSize: %d\tMapKey: %ud\tDescriptorSize: %d\n", MapSize, MapKey, DescriptorSize);
    MapSize *= 2;
    alloc_pool(EfiLoaderData, MapSize, (void**)&Map);
    memzero(Map, MapSize);
    gST->BootServices->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);

    int MemMapCount = MapSize / DescriptorSize;

    printv("MapSize: %d\tMapKey: %ud\tDescriptorSize: %d\n", MapSize, MapKey, DescriptorSize);
    printv("MemMapCount: %d\n", MemMapCount);

    for (int i = 0; i < MemMapCount - 1; i++) {
        EFI_MEMORY_DESCRIPTOR *desc1 = NextMemoryDescriptor(Map, i * DescriptorSize);
        for (int j = i + 1; j < MemMapCount; j++) {
            EFI_MEMORY_DESCRIPTOR *desc2 = NextMemoryDescriptor(Map, i * DescriptorSize);
            if (desc1->PhysicalStart > desc2->PhysicalStart) {
                EFI_MEMORY_DESCRIPTOR temp = *desc1;
                *desc1 = *desc2;
                *desc2 = temp;
            }
        }
    }

    uint64_t datasize = 0;
    uint64_t allpages = 0;

    for (int i = 0; i <= MemMapCount; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = NextMemoryDescriptor(Map, i * DescriptorSize);

        if (desc->Type == EfiLoaderData) {
            printv("EfiLoaderData\n");
        }

        printv("[%d]\t%p\t%p\t%p\t%d\t%dkb\t%dmb\t%d - %x\n", i, desc->PhysicalStart, desc->PhysicalStart + desc->NumberOfPages * 0x1000, desc->VirtualStart, desc->NumberOfPages, desc->NumberOfPages * 0x1000 / 1024, (desc->NumberOfPages * 0x1000) / 1024 / 1024, desc->Type, desc->Attribute);

        if (desc->Attribute & EFI_MEMORY_WB) {
            datasize += desc->NumberOfPages << EFI_PAGE_SHIFT;
            allpages += desc->NumberOfPages;
        }
    }

    printv("kernelpages: %uq, allpages: %uq, explicit memory: %uqb - %uqmb\n", KernelPages, allpages, datasize, datasize / 1024 / 1024);


    params->kernel_start = start_pa;
    printv("start_pa: %p\n", start_pa);

    for (int i = 0, mi = 0; i < MemMapCount; i++, mi++) {
        EFI_MEMORY_DESCRIPTOR *desc = NextMemoryDescriptor(Map, i * DescriptorSize);

        params->mem_desc.mem_maps[mi].physical_address = desc->PhysicalStart;
        params->mem_desc.mem_maps[mi].number_of_pages = desc->NumberOfPages;

        uptr max_phys = desc->PhysicalStart + desc->NumberOfPages * PAGE_SIZE;
        if (params->mem_desc.max_phys_addr < max_phys) {
            params->mem_desc.max_phys_addr = max_phys - 1;
        }

        switch (desc->Type)
        {
        // case EfiLoaderCode:
        // case EfiLoaderData:
        case EfiBootServicesCode:
        case EfiBootServicesData:
        case EfiConventionalMemory:
            if (desc->PhysicalStart != 0 && desc->Attribute == 0xf) {
                params->mem_desc.mem_maps[mi].type = MEM_TYPE_AVAILABLE;
                params->mem_desc.available_memory += (desc->NumberOfPages * PAGE_SIZE);
            } else {
                params->mem_desc.mem_maps[mi].type = MEM_TYPE_RESERVED;
            }

            params->mem_desc.total_memory += (desc->NumberOfPages * PAGE_SIZE);
            params->mem_desc.mapcount++;
            break;
        
        case EfiACPIReclaimMemory:
            params->mem_desc.mem_maps[mi].type = MEM_TYPE_RESERVED;
            params->mem_desc.total_memory += (desc->NumberOfPages * PAGE_SIZE);
            params->mem_desc.mapcount++;
            break;

        case EfiACPIMemoryNVS:
            params->mem_desc.mem_maps[mi].type = MEM_TYPE_NVS;
            params->mem_desc.total_memory += (desc->NumberOfPages * PAGE_SIZE);
            params->mem_desc.mapcount++;
            break;

        case EfiUnusableMemory:
            params->mem_desc.mem_maps[mi].type = MEM_TYPE_UNUSABLE;
            params->mem_desc.total_memory += (desc->NumberOfPages * PAGE_SIZE);
            params->mem_desc.mapcount++;
            break;
        
        default:
            if ((desc->Attribute & 0xf) == 0xf) {
                params->mem_desc.mem_maps[mi].type = MEM_TYPE_RESERVED;
                params->mem_desc.total_memory += (desc->NumberOfPages * PAGE_SIZE);
                params->mem_desc.mapcount++;
            }
            break;
        }
    }

    free_pool(Map);


    /* Load Screen Info */

    EFI_GRAPHICS_OUTPUT_PROTOCOL *mode = graph_get_mode();
    params->screen_info.framebuffer = (void*)mode->Mode->FrameBufferBase;
    params->screen_info.size = mode->Mode->FrameBufferSize;
    params->screen_info.width = mode->Mode->Info->HorizontalResolution;
    params->screen_info.height = mode->Mode->Info->VerticalResolution;
    params->screen_info.red_mask = mode->Mode->Info->PixelInformation.RedMask;
    params->screen_info.green_mask = mode->Mode->Info->PixelInformation.GreenMask;
    params->screen_info.blue_mask = mode->Mode->Info->PixelInformation.BlueMask;
    params->screen_info.reversed_mask = mode->Mode->Info->PixelInformation.ReservedMask;
    params->screen_info.pixel_format = mode->Mode->Info->PixelFormat;
    
    mode->SetMode(mode, mode->Mode->MaxMode);

    printv("mode->Mode->Info->PixelInformation: %p\n", mode->Mode->Info->PixelInformation);
    printv("mode->Mode->Info->PixelFormat: %d\n", mode->Mode->Info->PixelFormat);
    printv("mode->Mode->Info->PixelsPerScanLine: %d\n", mode->Mode->Info->PixelsPerScanLine);
    printv("mode->Mode->Info->HorizontalResolution: %d\n", mode->Mode->Info->HorizontalResolution);
    printv("mode->Mode->Info->VerticalResolution: %d\n", mode->Mode->Info->VerticalResolution);
    printv("mode->Mode->MaxMode: %d\n", mode->Mode->MaxMode);
    printv("framebuffer: %p, size: %d\n", params->screen_info.framebuffer, params->screen_info.size);

    gST->BootServices->ExitBootServices(gST, MapKey);


    print_bootparams();
    ((void(*)(void))KERNEL_START)();
}


void print_bootparams() {
    boot_params_t *params = (boot_params_t *)KERNEL_START;

    printv("memory desc: \n");
    printv("total memory: %q\n", params->mem_desc.total_memory);
    printv("available memory: %q\n", params->mem_desc.available_memory);
    printv("map count: %q\n", params->mem_desc.mapcount);
    printv("params->madt: %p\n", params->madt);
    print_madt(params->madt);
    
    printv("\n");
}
