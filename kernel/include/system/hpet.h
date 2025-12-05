#ifndef __HPET_H
#define __HPET_H
#include <lib/ktypes.h>
#include <system/acpi.h>


typedef struct _hpet_address_t
{
    u8 address_space_id;    // 0 - system memory, 1 - system I/O
    u8 register_bit_width;
    u8 register_bit_offset;
    u8 reserved;
    u64 address;
} __attribute__((packed)) hpet_address_t;


typedef struct _hpet_t {
    acpi_sdt_hdr_t hdr;
    u8 hardware_rev_id;
    u8 comparator_count:5;
    u8 counter_size:1;
    u8 reserved:1;
    u8 legacy_replacement:1;
    u16 pci_vendor_id;
    hpet_address_t address;
    u8 hpet_number;
    u16 minimum_tick;
    u8 page_protection;
} __attribute__((packed)) hpet_sdt_t;


#define HPET_GENERAL_CAP_ID         0x000ul
#define HPET_GENERAL_CONF           0x010ul
#define HPET_GENERAL_INTR_STATUS    0x020ul
#define HPET_MCV                    0x0f0ul

#define HPET_TIMER0_CONF_CAP        0x100ul
#define HPET_TIMER0_COMP_VAL        0x108ul
#define HPET_TIMER0_INTR_ROUTE      0x110ul

#define HPET_TIMER1_CONF_CAP        0x120ul
#define HPET_TIMER1_COMP_VAL        0x128ul
#define HPET_TIMER1_INTR_ROUTE      0x130ul

#define HPET_TIMER2_CONF_CAP        0x140ul
#define HPET_TIMER2_COMP_VAL        0x148ul
#define HPET_TIMER2_INTR_ROUTE      0x150ul

#define HPET_TIMERn_CONF_CAP(n)     (0x20ul * (n) + 0x100ul)
#define HPET_TIMERn_COMP_VAL(n)     (0x20ul * (n) + 0x108ul)
#define HPET_TIMERn_INTR_ROUTE(n)   (0x20ul * (n) + 0x110ul)



void hpet_init();

u64 hpet_reg_read(u32 offset);
void hpet_write_write(u32 offset, u64 value);

u32 hpet_reg_read32(u32 offset);
void hpet_write_write32(u32 offset, u32 value);

u64 hpet_period();
u64 hpet_period_ns();

u64 hpet_get();
u64 hpet_get_nanos();
u64 hpet_get_millis();

void hpet_sleep_nanos(u64 nanos);
void hpet_sleep_millis(u64 millis);

u64 hpet_period();

bool hpet_enable();

#endif