#include <xlib.h>
#include <display/shell.h>
#include <display/graphics.h>
#include <pointer.h>
#include <fs.h>
#include <acpi.h>
#include <efi.h>

EFI_SYSTEM_TABLE *gST;
EFI_HANDLE gImageHandle;

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    gImageHandle = ImageHandle;
    gST = SystemTable;

    graph_init();
    shell_init();
    pointer_init();
    fs_init();
    acpi_init();
    apic_init();

    shell();
    
    return EFI_SUCCESS;
}
