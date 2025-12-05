#include <acpi.h>
#include <xlib.h>

#define EFI_ACPI_TABLE_PROTOCOL_GUID \
  {0xffe06bdd, 0x6107, 0x46a6,\
    {0x7b, 0xb2, 0x5a, 0x9c, 0x7e, 0xc5, 0x27, 0x5c}}


struct EFI_ACPI_TABLE_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_TABLE_INSTALL_ACPI_TABLE) (
  IN void                   *This,
  IN VOID                                      *AcpiTableBuffer,
  IN UINTN                                     AcpiTableBufferSize,
  OUT UINTN                                    *TableKey
);

typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_TABLE_UNINSTALL_ACPI_TABLE) (
  IN void             *This,
  IN UINTN                               TableKey
);

typedef struct _EFI_ACPI_TABLE_PROTOCOL {
  EFI_ACPI_TABLE_INSTALL_ACPI_TABLE       InstallAcpiTable;
  EFI_ACPI_TABLE_UNINSTALL_ACPI_TABLE     UninstallAcpiTable;
}  EFI_ACPI_TABLE_PROTOCOL;


EFI_GUID AcpiGuid = EFI_ACPI_TABLE_PROTOCOL_GUID;


EFI_ACPI_TABLE_PROTOCOL *AcpiTableProt;


extern EFI_SYSTEM_TABLE *gST;

void acpi_init() {
    gST->BootServices->LocateProtocol(&AcpiGuid, NULL, (void **)&AcpiTableProt);
}

EFI_STATUS acpi_install(void *AcpiTableBuffer, UINTN AcpiTableBufferSize, UINTN *TableKey) {
    return AcpiTableProt->InstallAcpiTable(AcpiTableProt, AcpiTableBuffer, AcpiTableBufferSize, TableKey);
}

EFI_STATUS acpi_uninstall(UINTN TableKey) {
    return AcpiTableProt->UninstallAcpiTable(AcpiTableProt, TableKey);
}



// void acpi_info(ACPI_TABLE_HEADER* table) {
//     puts("Signature: ");
//     puts(table->Signature);
//     puts("\r\n");

//     puts("Length: ");
//     putd(table->Length, 10);
//     puts("\r\n");

//     puts("Revision: ");
//     putd(table->Revision, 10);
//     puts("\r\n");

//     puts("Checksum: ");
//     putd(table->Checksum, 10);
//     puts("\r\n");

//     puts("OemId: ");
//     puts(table->OemId);
//     puts("\r\n");
    
//     puts("OemTableId: ");
//     puts(table->OemTableId);
//     puts("\r\n");

//     puts("OemRevision: ");
//     putd(table->OemRevision, 10);
//     puts("\r\n");

//     puts("AslCompilerId: ");
//     puts(table->AslCompilerId);
//     puts("\r\n");

//     puts("AslCompilerRevision: ");
//     putd(table->AslCompilerRevision, 10);
//     puts("\r\n");
// }


bool acpi_checksum(sdt_header_t *header)
{
    unsigned char sum = 0;
    for (uint32_t i = 0; i < header->length; i++) {
        sum += ((char *) header)[i];
    }
    return sum == 0;
}

bool acpi_checksum2(void *ptr, int size)
{
    unsigned char sum = 0, *_ptr = ptr;
    for (int i = 0; i < size; i++) {
        sum += _ptr[i];
    }
    return sum == 0;
}

rsdp_t *acpi_get_rsdp() {
    void *rsdp = null;

    EFI_GUID rsdp_guid = ACPI_TABLE_GUID;
    EFI_GUID xsdp_guid = ACPI_20_TABLE_GUID;
    
    for (UINTN i = 0; i < gST->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *configtable = &gST->ConfigurationTable[i];
        
        bool is_xsdp = memcmp(&xsdp_guid, &configtable->VendorGuid, sizeof(EFI_GUID)) == 0;
        bool is_rsdp = memcmp(&rsdp_guid, &configtable->VendorGuid, 20) == 0;
        
        if (!is_rsdp && !is_xsdp) {
            continue;
        }

        if ((is_xsdp && !acpi_checksum2(configtable->VendorTable, sizeof(acpi_sdt_t))) ||
            (is_rsdp && !acpi_checksum2(configtable->VendorTable, 20))) {
                continue;
        }

        if (is_xsdp) {
            rsdp = configtable->VendorTable;
            break;
        } else {
            rsdp = configtable->VendorTable;
        }
    }

    return rsdp;
}

acpi_sdt_t *acpi_get_table(const char *signature, int index) {
    rsdp_t *rsdp = acpi_get_rsdp();

    bool use_xsdt = false;
    if (rsdp->revision >= 2 && rsdp->xsdt_address)
        use_xsdt = true;

    acpi_sdt_t *sdt;
    int entry_count;

    if (use_xsdt) {
        sdt = (acpi_sdt_t *)rsdp->xsdt_address;
        entry_count = (sdt->header.length - sizeof(sdt_header_t)) / 8;
    } else {
        sdt = (acpi_sdt_t *)((uptr)rsdp->rsdt_address);
        entry_count = (sdt->header.length - sizeof(sdt_header_t)) / 4;
    }

    int count = 0;

    for (int i = 0; i < entry_count; i++) {
        sdt_header_t *entry = ((sdt_header_t **)(sdt->tstart))[i];

        if (memcmp(signature, entry->signature, 4) == 0 && 
            acpi_checksum(entry) &&
            count++ == index)
        {
            printv("acpi: Found \"%s\" at %p\n", signature, entry);
            return entry;
        }
    }

    return null;
}
