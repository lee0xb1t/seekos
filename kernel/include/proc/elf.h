/**
 * https://uclibc.org/docs/elf-64-gen.pdf
 */

#ifndef ELF_H
#define ELF_H
#include <proc/task.h>
#include <lib/ktypes.h>


enum elf_ident {
	EI_MAG0		= 0, // 0x7F
	EI_MAG1		= 1, // 'E'
	EI_MAG2		= 2, // 'L'
	EI_MAG3		= 3, // 'F'
	EI_CLASS	= 4, // Architecture (32/64)
	EI_DATA		= 5, // Byte Order
	EI_VERSION	= 6, // ELF Version
	EI_OSABI	= 7, // OS Specific
	EI_ABIVERSION	= 8, // OS Specific
	EI_PAD		= 9  // Padding
};

#define ELFMAG0	0x7F // e_ident[EI_MAG0]
#define ELFMAG1	'E'  // e_ident[EI_MAG1]
#define ELFMAG2	'L'  // e_ident[EI_MAG2]
#define ELFMAG3	'F'  // e_ident[EI_MAG3]
#define ELFMAGIC       0x464C457FU


# define ELFDATA2LSB	(1)  // Little Endian
# define ELFCLASS32	(1)  // 32-bit Architecture

enum elf_type {
	ET_NONE		= 0, // Unkown Type
	ET_REL		= 1, // Relocatable File
	ET_EXEC		= 2,  // Executable File
    ET_DYN      = 3,
    ET_CORE		= 4,
};

#define PT_LOAD         0x00000001      /* Loadable program segment */
#define PT_INTERP       0x00000003      /* Program interpreter */
#define PT_PHDR         0x00000006      /* Entry for header table itself */

#define ABI_SYSV        0x00
#define ARCH_X86_64     0x3e


# define ELF_NIDENT	16

typedef struct {
	u8		e_ident[ELF_NIDENT];
	u16  	e_type;
	u16	    e_machine;
	u32	    e_version;
	u64	    e_entry;
	u64	    e_phoff;
	u64	    e_shoff;
	u32	    e_flags;
	u16	    e_ehsize;
	u16	    e_phentsize;
	u16	    e_phnum;
	u16	    e_shentsize;
	u16	    e_shnum;
	u16	    e_shstrndx;
} elf64_ehdr_t;


#define PT_NULL         0
#define PT_LOAD         1
#define PT_DNY          2
#define PT_INTERP       3
#define PT_NOTE         4

#define PT_SHLIB        5
#define PT_PHDR         6
#define PT_LOOS         0x60000000
#define PT_HIOS         0x6FFFFFFF
#define PT_LOPROC       0x70000000
#define PT_HIPROC       0x7FFFFFFF

#define PF_X            1
#define PF_W            2
#define PF_R            4

typedef struct {
    uint32_t type;              /* Segment type */
    uint32_t flags;             /* Segment-dependent flags */
    uint64_t offset;            /* Segment offset in the file image */
    uint64_t vaddr;             /* Segment virtual address in memory */
    uint64_t paddr;             /* Reserved for segment physical address */
    uint64_t filesz;            /* Segment size in file image */
    uint64_t memsz;             /* Segment size in memory */
    uint64_t align;             /* 0 and 1 specify no alignment. Otherwise should be a
                                 * positive, integral power of 2, with p_vaddr equating
                                 * p_offset modulus p_align.
                                 */
} elf64_phdr_t;


#define SHT_NULL            0   /* sh_type */
#define SHT_PROGBITS        1
#define SHT_SYMTAB          2
#define SHT_STRTAB          3
#define SHT_RELA            4
#define SHT_HASH            5
#define SHT_DYNAMIC         6
#define SHT_NOTE            7
#define SHT_NOBITS          8
#define SHT_REL             9
#define SHT_SHLIB           10
#define SHT_DYNSYM          11
#define SHT_UNKNOWN12       12
#define SHT_UNKNOWN13       13
#define SHT_INIT_ARRAY      14
#define SHT_FINI_ARRAY      15
#define SHT_PREINIT_ARRAY   16
#define SHT_GROUP           17
#define SHT_SYMTAB_SHNDX    18
#define SHT_NUM             19

typedef struct {
	u32	sh_name;
	u32	sh_type;
	u64	sh_flags;
	u64	sh_addr;
	u64	sh_offset;
	u64	sh_size;
	u32	sh_link;
	u32	sh_info;
	u64	sh_addralign;
	u64	sh_entsize;
} elf64_shdr_t;

i32 elf_load(task_t *, const char* path);
elf64_shdr_t *elf_section(elf64_ehdr_t *ehdr, int idx);
elf64_phdr_t *elf_program(elf64_ehdr_t *ehdr, int idx);
void *elf_program_data(elf64_ehdr_t *ehdr, int idx);

#endif