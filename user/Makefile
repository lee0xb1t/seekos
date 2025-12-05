
CC			= $(CROSS_COMPILE)gcc
LD			= $(CROSS_COMPILE)ld
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

CFLAGS := 
ASFLAGS := 
LDFLAGS := 


CFLAGS		+= -m64 -march=x86-64  \
			-Wall -Wextra \
			-std=gnu11 \
			-nostdlib 	\
			-fno-builtin -ffreestanding \
			-mgeneral-regs-only	\
			-mno-red-zone \
			-I. -I..
			

ASFLAGS		+= -march=x86-64

LDFLAGS		+= -m elf_x86_64	\
			-nostdlib 	\
			-fno-builtin -ffreestanding	\
			-fno-pie	\
			-z noexecstack


LIBC_OBJS := $(patsubst ../libc/%, ../libc/obj/%, $(addsuffix .o, $(wildcard ../libc/*.c)))

# USER_OBJS := $(patsubst ./%, obj/%, $(addsuffix .o, $(wildcard ./*.c)))
USER_OBJS += $(patsubst ./%, obj/%, $(addsuffix .o, $(wildcard ./*.S)))


ELFS := shell test1


../libc/obj/%.c.o: ../libc/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.S.o: ./%.S
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@


obj/shell.c.o: ./shell.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

shell: %: $(LIBC_OBJS) $(USER_OBJS) obj/shell.c.o
	$(info $<)
	$(LD) $(LD_FLAGS) -T linker.lds -e _start $(LIBC_OBJS) $(USER_OBJS) obj/shell.c.o -o $@


obj/test1.c.o: ./test1.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test1: %: $(LIBC_OBJS) $(USER_OBJS) obj/test1.c.o
	$(info $<)
	$(LD) $(LD_FLAGS) -T linker.lds -e _start $(LIBC_OBJS) $(USER_OBJS) obj/test1.c.o -o $@


$(info $(LIBC_OBJS))
$(info $(USER_OBJS))

.PHONY: all clean

all: $(ELFS) $(LIBC_OBJS) $(USER_OBJS)

clean:
	-rm -rf ../libc/obj
	-rm -rf obj
	-rm $(ELFS)