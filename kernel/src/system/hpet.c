/**
 * https://wiki.osdev.org/HPET
 */

#include <system/hpet.h>
#include <system/acpi.h>
#include <mm/mm.h>
#include <lib/kmemory.h>
#include <panic/panic.h>


static volatile void *hpet_base = null;
static bool hpet_count_size_cap = false;

static u64 hpet_freq_khz = 0;
static u64 hpet_period_ns_ = 0;


void hpet_init() {
    hpet_sdt_t *hpet_sdt = (hpet_sdt_t *)acpi_get_sdt("HPET");
    // memcpy(&hpet_boot, &bootparams->hpet_event_timer_block, 20);

    if (!hpet_sdt->address.address) {
        return;
    }

    klog_debug("\nhpet: base address = %p\n", hpet_sdt->address.address);
    klog_debug("hpet: addr space id = %d  bit width = %d  bit offset = %d\n", 
        hpet_sdt->address.address_space_id, 
        hpet_sdt->address.register_bit_width, 
        hpet_sdt->address.register_bit_offset);

    hpet_base = (void *)(MI_DIRECT_MAPPING_RANGE_START + hpet_sdt->address.address);
    vmm_map(hpet_base, hpet_sdt->address.address, 1, VMM_FLAGS_MMIO);
    klog_debug("hpet: hpet base = %p\n", hpet_base);

    u64 cap = hpet_reg_read(HPET_GENERAL_CAP_ID);

    hpet_count_size_cap = (cap >> 13) & 0x1;
    klog_debug("hpet: count size = %d\n", hpet_count_size_cap ? 64 : 32);

    u64 counter_clk_period = (cap >> 32) & 0xffffffff;
    klog_debug("hpet: clock period = %d fs\n", counter_clk_period);

    /* Calculate HPET frequency 10^15 fs = 1s */
    hpet_freq_khz = 1000000000000000ull / counter_clk_period / 1000;
    hpet_period_ns_ = counter_clk_period / 1000000;


    klog_debug("hpet: hpet freq = %d khz\n", hpet_freq_khz);

    /* Enable main counter */
    hpet_reg_write(HPET_GENERAL_CONF, hpet_reg_read(HPET_GENERAL_CONF) | 0x1);

    klog_debug("HPET Initialized.\n");
}


inline u64 hpet_reg_read(u32 offset) {
    return *((volatile u64 *) ((u64)hpet_base + offset));
}

inline void hpet_reg_write(u32 offset, u64 value) {
    *((volatile u64 *) ((u64)hpet_base + offset)) = value;
}

inline u32 hpet_reg_read32(u32 offset) {
    return *((volatile u64 *) ((u64)hpet_base + offset));
}

inline void hpet_reg_write32(u32 offset, u32 value) {
    *((volatile u32 *) ((u64)hpet_base + offset)) = value;
}

inline u64 hpet_get() {
    return hpet_reg_read(HPET_MCV);
}

inline u64 hpet_get_nanos() {
    return hpet_get() * hpet_period_ns_;
}

inline u64 hpet_get_millis() {
    return hpet_get_nanos() / 1000000;
}

inline void hpet_sleep_nanos(u64 nanos) {
    u64 start = hpet_get_nanos();
    for(;;) {
        u64 now = hpet_get_nanos();
        if (now - start >= nanos) {
            break;
        }
        asm volatile ("rep; nop");
    }
}

inline void hpet_sleep_millis(u64 millis) {
    hpet_sleep_nanos(millis * 1000000);
}

inline bool hpet_enable() {
    return hpet_base != null;
}

inline u64 hpet_period_ns() {
    return hpet_period_ns_;
}

u64 hpet_period() {
    return hpet_period_ns_ * 1000000;
}
