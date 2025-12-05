#ifndef __ACPI_H
#define __ACPI_H
#include <lib/ktypes.h>
#include <limine.h>


typedef struct {
 char signature[8];
 uint8_t checksum;
 char oemid[6];
 uint8_t revision;
 uint32_t rsdt_address;      // deprecated since version 2.0
 
 uint32_t length;
 uint64_t xsdt_address;
 uint8_t extended_checksum;
 uint8_t reserved[3];
} __attribute__ ((packed)) rsdp_t;

typedef struct {
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    u8 oemid[6];
    u8 oem_table_id[8];
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;
} __attribute__ ((packed)) acpi_sdt_hdr_t;

typedef struct {
    acpi_sdt_hdr_t hdr;
    u8 data[0];
} __attribute__ ((packed)) acpi_sdt_t;


void acpi_init(struct limine_rsdp_response *);
acpi_sdt_t *acpi_get_sdt(const char *sign);

#endif