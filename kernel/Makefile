defualt_dist:= .
ifdef dist
	override defualt_dist := $(dist)
endif

CFLAGS :=
ASFLAGS :=
LDFLAGS :=

ifdef debug
	CFLAGS += -g -O0
	ASFLAGS += -g -O0
else
	ifdef release
		CFLAGS += -O3 -DNDEBUG
		ASFLAGS += -O3 -DNDEBUG
		LDFLAGS += -s
	else
		CFLAGS += -O2
		ASFLAGS += -O2
	endif
endif


LD			= $(CROSS_COMPILE)ld
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump


CFLAGS		+= -m64 -march=x86-64  \
			-Wall -Wextra \
			-std=gnu11 \
			-static -nostdlib 	\
			-mgeneral-regs-only	\
			-fno-builtin -ffreestanding \
			-fno-PIC -mno-red-zone \
			-mcmodel=kernel 	\
			-Iinclude \
			$(INCLUDE_CFLAGS)	\
			-D_TEST_CASE

ASFLAGS		+= -march=x86-64 -mcmodel=kernel -fno-PIC

LDFLAGS		+= -m elf_x86_64 \
			-static \
			-z max-page-size=0x1000 


objs 		:= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/*.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/lib/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/lib/*.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/log/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/log/*.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/com/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/com/*.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/system/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/system/gdt_support.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/system/idt_support.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/exception/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/exception/*.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/panic/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/panic/*.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/mm/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/mm/*.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/device/display/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/device/display/*.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/base/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/base/*.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/proc/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/proc/*.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/device/keyboard/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/device/keyboard/*.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/fs/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/fs/*.S)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/device/storage/*.c)))
objs 		+= $(patsubst src/%, obj/%, $(addsuffix .o, $(wildcard src/device/storage/*.S)))



obj/%.c.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.S.o: src/%.S
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

obj/%/%.c.o: src/%/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

obj/%/%.S.o: src/%/%.S
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

obj/system/ap_trampoline: src/system/ap_trampoline.S
	mkdir -p $(dir $@)
	$(CC) -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -m16 -Werror -fno-omit-frame-pointer -Wno-array-bounds -Wno-infinite-recursion -fno-pic -nostdinc -fno-stack-protector  -I. -c src/system/ap_trampoline.S -o obj/system/ap_trampoline.S.o
	$(LD) -m elf_i386 -N -e _ap_trampoline -Ttext 0x01000 -o obj/system/ap_trampoline.o obj/system/ap_trampoline.S.o
	$(OBJCOPY) -S -O binary -j .text obj/system/ap_trampoline.o ap_trampoline.bin
	$(OBJDUMP) -M i8086 -S obj/system/ap_trampoline.o > obj/system/ap_trampoline.o.asm.16
	$(OBJDUMP) -M i386 -S obj/system/ap_trampoline.o > obj/system/ap_trampoline.o.asm.32


LDFLAGS_kernel.elf := -z noexecstack -T
# kernel.elf: kernel.lds $(objs) obj/system/ap_trampoline
kernel.elf: kernel.lds $(objs) obj/system/ap_trampoline
	$(LD) $(LDFLAGS_kernel.elf) kernel.lds $(objs) -b binary ap_trampoline.bin -o $@

OBJCOPYFLAGS_kernel.bin	:= -O binary
$(defualt_dist)/kernel.bin: kernel.elf
	$(OBJCOPY) $(OBJCOPYFLAGS) $(OBJCOPYFLAGS_kernel.bin) $< $@


.PHONY: clean
clean:
	-rm $(defualt_dist)/kernel.bin kernel.elf $(objs) ap_trampoline.bin
	-rm -rf obj
 