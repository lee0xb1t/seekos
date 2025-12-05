objects := boot.o loader.o tty.o smap.o halt.o loader_16.o loader_32.o video.o

LD		= $(CROSS_COMPILE)ld
OBJCOPY		= $(CROSS_COMPILE)objcopy

ASFLAGS		:= -m32 -Wall -Wextra -ggdb -O0 -nostdlib -fno-builtin -fno-pic
CFLAGS		:= $(KCFLAGS) -m32 -Wall -Wextra -ggdb -O0 -nostdlib -fno-builtin -fno-pic


LDFLAGS_boot.elf := -m elf_i386 -T
bootloader.elf: ./bootloader.lds $(objects)
	$(LD) $(LDFLAGS_boot.elf) $^ -o $@

OBJCOPYFLAGS_boot.bin	:= -O binary
$(dist)/bootloader.bin: bootloader.elf
	$(OBJCOPY) $(OBJCOPYFLAGS) $(OBJCOPYFLAGS_boot.bin) $< $@


.PHONY: clean
clean:
	-rm $(dist)/bootloader.bin bootloader.elf $(objects)
 