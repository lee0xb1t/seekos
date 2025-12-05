#ifndef __APIC_H
#define __APIC_H
#include <xtypes.h>
#include <acpi.h>

#define MADT_ICS_TYPE_LAPIC                  (0)
#define MADT_ICS_TYPE_IOAPIC                 (1)
#define MADT_ICS_TYPE_ISO                    (2)
#define MADT_ICS_TYPE_NMI_SOURCE             (3)
#define MADT_ICS_TYPE_LAPIC_NMI              (4)
#define MAX_MADT_ICS_TYPE                    (0x1B)

typedef struct {
    u8 type;
    u8 length;
} madt_record_header_t;

typedef struct {
    sdt_header_t header;
    u32 local_interrupt_controller_address;
#define MADT_FLAGS_PCAT_COMPAT          (1)
    u32 flags;

    u8 records[0];
} __attribute__((packed)) madt_t;

typedef struct {
    madt_record_header_t header;
    u8 acpi_processor_uid;
    u8 apic_id;
#define MADT_LAPIC_FLAGS_ENABLED        (1)
#define MADT_LAPIC_FLAGS_ONLINE_CAPABLE (1ul << 1)
    u32 flags;
} madt_lapic_t;

typedef struct {
    madt_record_header_t header;
    u8 ioapic_id;
    u8 reserved;
    u32 ioapic_address;
    u32 gsi_base;
} madt_ioapic_t;

typedef struct {
    madt_record_header_t header;
    u8 bus;
    u8 source;
    u32 gsi;

#define MADT_ISO_FLAGS_POLARITY_MASK            (0xC)
#define MADT_ISO_FLAGS_POLARITY_ZERO            (0)
#define MADT_ISO_FLAGS_POLARITY_ACTIVE_HIGH     (0x4)
#define MADT_ISO_FLAGS_POLARITY_RESERVED        (0x8)
#define MADT_ISO_FLAGS_POLARITY_ACTIVE_LOW      (0xC)

#define MADT_ISO_FLAGS_TRIGGER_MODE_MASK        (3)
#define MADT_ISO_FLAGS_TRIGGER_MODE_ZERO        (0)
#define MADT_ISO_FLAGS_TRIGGER_MODE_EDGE        (1)
#define MADT_ISO_FLAGS_TRIGGER_MODE_LEVEL       (3)
    u16 flags;  /*MPS INTI Flags*/
} madt_iso_t;

void apic_init();
madt_t *apic_get_madt(); 
u32 apic_get_madt_size(); 

void print_madt(madt_t *madt_);

#endif