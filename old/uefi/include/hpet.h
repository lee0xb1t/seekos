#ifndef __HPET_H
#define __HPET_H
#include <xtypes.h>
#include <acpi.h>


typedef struct _hpet_addr_t {
    uint8_t address_space_id;    // 0 - system memory, 1 - system I/O
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t address;
} __attribute__((packed)) hpet_addr_t;

typedef struct _hpet_t
{
    sdt_header_t header;

    u8 hardware_rev_id;
    u8 comparator_count:5;
    u8 counter_size:1;
    u8 reserved:1;
    u8 legacy_replacement:1;
    u16 pci_vendor_id;
    hpet_addr_t address;
    u8 hpet_number;
    u16 minimum_tick;
    u8 page_protection;
} __attribute__((packed)) hpet_t;


void hpet_init();

void print_hpet();

hpet_t *hpet_get_hpet();

#endif