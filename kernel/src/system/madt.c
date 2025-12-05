#include <system/madt.h>
#include <system/cpu.h>
#include <mm/mm.h>
#include <log/klog.h>


madt_t *madt = null;

u16 num_lapics;
madt_record_lapic_t *lapics[MAX_CPUS];

u16 num_ioapics;
madt_record_ioapic_t *ioapics[MADT_IOAPIC_NUM_MAX];

u16 num_ioapic_iso;
madt_record_ioapic_t *ioapic_iso[MADT_ISA_IRQ_MAX];


void madt_init() {
    num_lapics = 0;
    num_ioapics = 0;

    madt = acpi_get_sdt("APIC");

    // madt = (madt_t *)(MI_DIRECT_MAPPING_RANGE_START + boot_params->madt);
    // size_t madt_size = PAGE_ALIGN_UP_IF(boot_params->madt_size);
    // vmm_map(madt, (uptr)boot_params->madt, SIZE_TO_PAGE(madt_size), VMM_FLAGS_MMIO);

    u32 size = madt->header.length - sizeof(madt_t);
    for (u32 i = 0; i < size;) {
        madt_record_header_t *record = (madt_record_header_t *)(madt->records + i);

        switch (record->type)
        {
        case MADT_ICS_TYPE_LAPIC:
            {
                /* Only support 256 cpus */
                if (num_lapics >= 256) {
                    break;
                }
                lapics[num_lapics++] = (madt_record_lapic_t *)record;
            }
            break;
        case MADT_ICS_TYPE_IOAPIC:
            {
                /* Only support 2 ioapics */
                if (num_ioapics > 2) {
                    break;
                }
                ioapics[num_ioapics++] = (madt_record_ioapic_t *)record;
            }
            break;

        case MADT_ICS_TYPE_ISO:
            {
                ioapic_iso[num_ioapic_iso++] = (madt_record_iso_t *)record;
            }
            break;
        
        default:
            break;
        }

        i += record->length;
    }
    
    klog_debug("MADT Initialized.\n");
}

u16 madt_get_num_lapic() {
    return num_lapics;
}

madt_record_lapic_t *madt_get_lapic(int idx) {
    if (idx >= num_lapics)
        return null;

    return lapics[idx];
}

u16 madt_get_num_ioapic() {
    return num_ioapics;
}

madt_record_ioapic_t *madt_get_ioapic(int idx) {
    if (idx >= num_ioapics)
        return null;

    return ioapics[idx];
}

u16 madt_get_num_ioapic_iso() {
    return num_ioapic_iso;
}

madt_record_iso_t *madt_get_ioapic_iso(int idx) {
    return ioapic_iso[idx];
}
