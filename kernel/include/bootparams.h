//
// DEPRECATED
//

#ifndef __bootparams_h__
#define __bootparams_h__

#include <lib/ktypes.h>

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
} __attribute__((packed)) memory_maps_desc_t;

typedef struct {
    i32 mapcount;
    u32 __dummy;
    u64 available_memory;
    u64 total_memory;
    u64 max_phys_addr;
    memory_maps_desc_t mem_maps[MAX_MEMORY_MAP_SIZE];
} __attribute__((packed)) memory_maps_t;

typedef struct {
    void *framebuffer;
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
    u64 kernel_start;
    screen_info_t screen_info;
    memory_maps_t mem_desc;
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

#endif //__bootparams_h__