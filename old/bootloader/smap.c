#include "code16gcc.h"
#include "loader.h"


// https://wiki.osdev.org/Detecting_Memory_(x86)
// SMAP entry structure
typedef struct SMAP_entry {
 
	u32 BaseL; // base address uint64_t
	u32 BaseH;
	u32 LengthL; // length uint64_t
	u32 LengthH;
	u32 Type; // entry Type
	u32 ACPI; // extended
 
}__attribute__((packed)) SMAP_entry_t;

// https://wiki.osdev.org/Detecting_Memory_(x86)
// load memory map to buffer - note: regparm(3) avoids stack issues with gcc in real mode
int __attribute__((noinline)) __attribute__((regparm(3))) detectMemory(SMAP_entry_t* buffer, int maxentries) {
	u32 contID = 0;
	int entries = 0, signature, bytes;
	do 
	{
		__asm__ __volatile__ ("int  $0x15;" 
				: "=a"(signature), "=c"(bytes), "=b"(contID)
				: "a"(0xE820), "b"(contID), "c"(24), "d"(0x534D4150), "D"(buffer));
		if (signature != 0x534D4150) 
			return -1; // error
		if (bytes > 20 && (buffer->ACPI & 0x0001) == 0) {
			// ignore this entry
		}
		else {
			buffer++;
			entries++;
		}
	}
	while (contID != 0 && entries < maxentries);
	return entries;
}

void detect_memory(void) {
    SMAP_entry_t smap_entries[10];
    int detect_ok = detectMemory(smap_entries, 10);

    if (detect_ok == -1) {
        puts("[detect_memory] signature not match!\n");
        halt();
    }
    else if (detect_ok <= 0) {
        puts("[detect_memory] smap_entry is empty!\n");
        halt();
    }
    
    boot_info.ram_region_count = 0;

    for (int i = 0; i < 10; i++) {
        SMAP_entry_t* samp_entry = &smap_entries[i];

        if (samp_entry->Type == 1) {
            boot_info.ram_region[boot_info.ram_region_count].start = samp_entry->BaseL;
            boot_info.ram_region[boot_info.ram_region_count].size = samp_entry->LengthL;
            boot_info.ram_region_count++;

            puts("[detect_memory] samp_entry->BaseL: ");
            put_dword_number(samp_entry->BaseL);
            puts(", ");
            puts("samp_entry->LengthL: ");
            put_dword_number(samp_entry->LengthL);
            puts("\n");
        }
    }

    puts("detect memory ok!\n");
}