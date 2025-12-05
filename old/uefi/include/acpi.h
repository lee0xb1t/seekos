#ifndef __acpi_h__
#define __acpi_h__

#include <efi.h>
#include <xtypes.h>


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
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__ ((packed)) sdt_header_t;


typedef struct {
    sdt_header_t header;
    char tstart[];
} acpi_sdt_t;


void acpi_init();

EFI_STATUS acpi_install(void *AcpiTableBuffer, UINTN AcpiTableBufferSize, UINTN *TableKey);
EFI_STATUS acpi_uninstall(UINTN TableKey);

//void acpi_info(ACPI_TABLE_HEADER* table);

bool acpi_checksum(sdt_header_t *header);

bool acpi_checksum2(void *ptr, int size);

rsdp_t *acpi_get_rsdp();

acpi_sdt_t *acpi_get_table(const char *signature, int index);

#endif //__acpi_h__