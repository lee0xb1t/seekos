#include <apic.h>
#include <xlib.h>

madt_t *madt = null;

void apic_init() {
    madt = (madt_t *)acpi_get_table("APIC", 0);
}

void print_madt(madt_t *madt_) {
    if (madt_ == null) {
        madt_ = madt;
    }

    printv("signature: %c%c%c%c, ", madt_->header.signature[0], madt_->header.signature[1], madt_->header.signature[2], madt_->header.signature[3]);
    printv("length: %d, lica: %p\n", madt_->header.length, madt_->local_interrupt_controller_address);

    int size = madt_->header.length - sizeof(madt_t);

    for (int i = 0; i < size;) {
        madt_record_header_t *record = (madt_record_header_t *)(madt_->records + i);

        switch (record->type)
        {
        case MADT_ICS_TYPE_LAPIC: 
             {
                printv("type: MADT_ICS_TYPE_LAPIC: \n");
                madt_lapic_t *lapic = (madt_lapic_t *) record;
                printv("\tacpi_processor_uid->%d apic_id->%d\n", lapic->acpi_processor_uid, lapic->apic_id);
             }
            break;

        case MADT_ICS_TYPE_IOAPIC: 
             {
                printv("type: MADT_ICS_TYPE_IOAPIC: \n");
                madt_ioapic_t *ioapic = (madt_ioapic_t *) record;
                printv("\tioapic_id->%d ioapic_address->%p gsi_base->%d\n", ioapic->ioapic_id, ioapic->ioapic_address, ioapic->gsi_base);
             }
            break;

        case MADT_ICS_TYPE_ISO: 
             {
                printv("type: MADT_ICS_TYPE_ISO: \n");
                madt_iso_t *iso = (madt_iso_t *) record;
                printv("\tbus->%d source->%d gsi->%d flags->%d\n", iso->bus, iso->source, iso->gsi, iso->flags);
             }
            break;
        
        default:
            printv("other type: %p\n", record->type);
            break;
        }

        i += record->length;
    }
}

madt_t *apic_get_madt() {
    return madt;
}

u32 apic_get_madt_size() {
    return madt->header.length;
}
