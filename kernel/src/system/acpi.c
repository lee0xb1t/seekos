#include <system/acpi.h>
#include <mm/mm.h>
#include <log/klog.h>
#include <lib/kmemory.h>


static volatile acpi_sdt_t *sdt = null;
static volatile bool use_xsdt = false;

void acpi_init(struct limine_rsdp_response *rdspr) {
    //vmm_map(rdsp->address, __VIRT_TO_PHYS(rdsp->address), );

    rsdp_t *rsdp = (rsdp_t *)rdspr->address;

    if (rsdp->revision >= 2) {
        sdt = (acpi_sdt_t *)__PHYS_TO_VIRT(rsdp->xsdt_address);
        use_xsdt = true;
    } else {
        sdt = (acpi_sdt_t *)__PHYS_TO_VIRT(rsdp->rsdt_address);
        use_xsdt = false;
    }

    klogi("ACPI Initialized.\n");
}

acpi_sdt_t *acpi_get_sdt(const char *sign) {
    u32 len = (sdt->hdr.length - sizeof(acpi_sdt_hdr_t)) / (use_xsdt ? 8 : 4);
    for (int i = 0; i < len; i++) {
        acpi_sdt_t *table = (acpi_sdt_t *)__PHYS_TO_VIRT(
            use_xsdt ? ((u64 *)sdt->data)[i] : ((u32 *)sdt->data)[i]
        );

        if (memcmp(table->hdr.signature, sign, 4) == 0) {
            return table;
        }
    }

    return null;
}
