#include <hpet.h>


static hpet_t *hpet = null;

void print_hpet() {
    printv("hpet: signature: %c%c%c%c, ", hpet->header.signature[0], hpet->header.signature[1], hpet->header.signature[2], hpet->header.signature[3]);
    printv("hpet: length: %d, pci_vendor_id: %p\n", hpet->header.length, hpet->pci_vendor_id);
    printv("hpet: addr: %p\n", hpet->address.address);
}

void hpet_init() {
    hpet = (hpet_t *)acpi_get_table("HPET", 0);
}

hpet_t *hpet_get_hpet() {
    return hpet;
}
