AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
MAKE		= make

ARCH := x86_64

# Build to dist
dist := $(CURDIR)

# Create dist dir
ifeq ($(wildcard ~$(dist).*),)
    $(shell mkdir -p $(dist))
endif


# Global include
INCLUDE_CFLAGS := -I$(CURDIR)/include -I$(CURDIR)/limine-protocol/include

HDD_IAMGE := release/seekos.img

# QEMU FLAGS
QEMU_CPU_HOST := -cpu host -enable-kvm -smp 2,cores=2,threads=1,sockets=1
QEMU_CPU_EMU := -cpu core2duo-v1 -smp 2,cores=2,threads=1,sockets=1
QEMU_AUDIO := -audiodev pa,id=speaker -machine pcspk-audiodev=speaker
QEMU_MEM := -m 2g
QEMU_DISK := -hda $(HDD_IAMGE)
# QEMU_FLAGS := -net none -M q35 -bios ovmf-$(ARCH)/OVMF.fd -drive file=fat:rw:boot -serial mon:stdio -rtc base=localtime -no-reboot -no-shutdown -d cpu_reset -D ./log.txt -m 2g -audiodev pa,id=speaker -machine pcspk-audiodev=speaker
QEMU_FLAGS := -net none -serial mon:stdio -rtc base=localtime -no-reboot -no-shutdown -d cpu_reset -D ./log.txt 


debug := on
release := on


.PHONY: all
all: kernel_release initrd limine $(HDD_IAMGE) user


# uefi
# .PHONY: uefi
# uefi:
# 	$(MAKE) dist=$(dist) INCLUDE_CFLAGS=$(INCLUDE_CFLAGS) $(dist)/bootx64.efi -C uefi -f Makefile


# kernel
.PHONY: kernel_debug
kernel_debug:
	$(MAKE) -C kernel -f Makefile \
		dist=$(dist) \
		debug=$(debug) \
		INCLUDE_CFLAGS="$(INCLUDE_CFLAGS)" \
		$(dist)/kernel.bin


.PHONY: kernel_release
kernel_release:
	$(MAKE) -C kernel -f Makefile \
		dist=$(dist) \
		release=$(release) \
		INCLUDE_CFLAGS="$(INCLUDE_CFLAGS)" \
		$(dist)/kernel.bin


# initrd
initrd:
	mkdir -p ramdisk
	cp -r sysroot/* ramdisk
	tar --format=ustar -cvf ./initrd -C ramdisk .


# limine
limine:
	git clone https://github.com/limine-bootloader/limine.git --branch v10.x-binary --depth=1
	git clone https://github.com/limine-bootloader/limine-protocol.git --branch trunk --depth=1
	make -C limine

.PHONY: user
user:
	$(MAKE) -C user -f Makefile all
	cp user/shell sysroot/bin/
	cp user/test1 sysroot/bin/


# disk image
$(HDD_IAMGE): user
	mkdir -p release
	-rm -rf $(HDD_IAMGE)
	qemu-img create -f raw $(HDD_IAMGE) 512m

	parted -s $(HDD_IAMGE) mklabel msdos
	parted -s $(HDD_IAMGE) mkpart primary fat32 1MiB 100%
	parted -s $(HDD_IAMGE) set 1 boot on

	./limine/limine bios-install $(HDD_IAMGE)

	echo "drive z: file=\"$(HDD_IAMGE)\" partition=1" > ~/.mtoolsrc
	mformat -i $(HDD_IAMGE)@@1M -F ::
	mcopy -i $(HDD_IAMGE)@@1M sysroot/* ::
	mcopy -i $(HDD_IAMGE)@@1M kernel/kernel.elf ::/boot/
	mcopy -i $(HDD_IAMGE)@@1M initrd ::/boot/
	mcopy -i $(HDD_IAMGE)@@1M limine.conf limine/limine-bios.sys ::/boot
#	...
	-rm ~/.mtoolsrc


.PHONY: run
run: clean kernel_debug initrd limine $(HDD_IAMGE)
ifeq ($(ARCH),x86_64)
	qemu-system-$(ARCH) $(QEMU_FLAGS) $(QEMU_CPU_HOST) $(QEMU_AUDIO) $(QEMU_MEM) $(QEMU_DISK)
else
	echo "Unknown Arch"
endif


.PHONY: debug
debug: clean kernel_debug initrd limine $(HDD_IAMGE)
ifeq ($(ARCH),x86_64)
	qemu-system-$(ARCH) $(QEMU_FLAGS) $(QEMU_CPU_EMU) $(QEMU_AUDIO) $(QEMU_MEM) $(QEMU_DISK) -s -S
else
	echo "Unknown Arch"
endif


.PHONY: release 
release: clean kernel_release initrd limine $(HDD_IAMGE)
ifeq ($(ARCH),x86_64)
	qemu-system-$(ARCH) $(QEMU_FLAGS) $(QEMU_CPU_HOST) $(QEMU_AUDIO) $(QEMU_MEM) $(QEMU_DISK)
else
	echo "Unknown Arch"
endif


.PHONY: clean
clean:
	-rm -rf $(HDD_IAMGE)
# 	$(MAKE) clean dist=$(dist) -C uefi -f Makefile
	$(MAKE) clean dist=$(dist) -C kernel -f Makefile
	-rm -rf ramdisk initrd release kernel.elf.txt
	$(MAKE) clean -C user -f Makefile
	-rm sysroot/bin/*


.PYONY: odumpk
odumpk:
	objdump -D kernel/kernel.elf > kernel.elf.txt
