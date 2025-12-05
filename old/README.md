开源操作系统内核

https://wiki.osdev.org/Creating_an_Operating_System

```shell
gcc qemu-system
```

uefi开发环境



fedora:

```shell
sudo dnf install gnu-efi mingw64-gcc.x86_64
```

---

Fonts:
```shell
bdf2psf gohufont/gohufont-14.bdf /usr/share/bdf2psf/standard.equivalents /usr/share/bdf2psf/ascii.set 512 gohufont-14.psf
```
And see: https://github.com/lucy/tewi-font/issues/7


安装ovmf, 支持qemu

```text
https://docs.fedoraproject.org/en-US/quick-docs/uefi-with-qemu/
```

创建映像:

```shell
qemu-img create -f raw xbitos.img 128m
```

写入硬盘:

bootloader:

```shell
dd if=bootloader.bin of=xbitos.img bs=512 count=262144 seek=0 conv=notrunc
dd if=kernel.bin of=xbitos.img bs=512 count=262044 seek=100 conv=notrunc
```

objdump -mx86-64 -b binary -D kernel/kernel.elf > kernel.elf.txt
objdump -D -Mintel,x86-64 -mi386 -b binary kernel.bin

qemu:

i386:
```shell
qemu-system-i386 -m 128m -hda xbitos.img -vga std -display gtk
qemu-system-i386 -m 128m -hda xbitos.img -vga std -display gtk -s -S
```

x86_64:
```shell
qemu-system-x86_64 --bios OVMF.fd -m 128m -hda xbitos.img -vga std -display gtk -net none
qemu-system-x86_64 --bios OVMF.fd -m 128m -hda xbitos.img -vga std -display gtk -net none -s -S
```

qemu-system-x86_64 -net none -M q35 -bios uefi/ovmf-x86_64/OVMF.fd -drive file=fat:rw:boot -m 1024m
qemu-system-x86_64 -net none -kernel ./bzImage -initrd initrd.img  -hda linux.img -append "root=/dev/sda"
qemu-system-x86_64 -net none -kernel ./bzImage -initrd initrd.img -append "root=/dev/sda1 init=/bin/bash" -hda rootfs.tar
qemu-system-x86_64 -net none -kernel ./bzImage -initrd rootfs.cpio -append "init=/linuxrc"

qemu-img create -f raw linux.img -o size=100M
mkfs ext3 -F linux.img
sudo mkdir -p /tmp/mount_tmp/ && sudo mount -o loop,rw,sync linux.img /tmp/mount_tmp


https://brokenthorn.com/Resources/OSDev12.html