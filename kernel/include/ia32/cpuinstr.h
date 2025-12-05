#ifndef __instr_h__
#define __instr_h__

#include <lib/ktypes.h>
#include <ia32/ia32.h>
#include <ia32/segment.h>
#include <ia32/interrupt.h>


static inline u8 port_inb(u16 port) {
    u8 data = 0;
    asm volatile(
        "inb    %1, %0"
        : "=a" (data)
        : "d" (port)
    );
    return data;
}

static inline void port_outb(u16 port, u8 data) {
    asm volatile(
        "outb    %0, %1"
        :
        : "a" (data), "d" (port)
    );
}

static inline u16 port_inw(u16 port) {
    u16 data = 0;
    asm volatile(
        "inw    %1, %0"
        : "=a" (data)
        : "d" (port)
    );
    return data;
}

static inline void port_outw(u16 port, u16 data) {
    asm volatile(
        "outw    %0, %1"
        :
        : "a" (data), "d" (port)
    );
}

static inline u16 port_ind(u16 port) {
    u32 data = 0;
    asm volatile(
        "inl    %1, %0"
        : "=a" (data)
        : "d" (port)
    );
    return data;
}

static inline void port_outd(u16 port, u32 data) {
    asm volatile(
        "outl    %0, %1"
        :
        : "a" (data), "d" (port)
    );
}

static inline void port_insw(u16 port, void *addr, u32 count) {
    asm volatile(
        "rep insw;"
        : "+D" (addr), "+c" (count)
        : "d" (port)
    );
}


static inline void __lgdt(const void *base, int size) {
    struct {
        u16 limit;
        u64 base;
        //u16 base_high;
    } __attribute__((packed)) gdt_ptr;

    gdt_ptr.base = (u64)base;
    gdt_ptr.limit = (sizeof(gdt_entry_t) * size) - 1;

    asm volatile (
        "lgdt %[g]"
        :
        : [g] "m" (gdt_ptr)
    );
}

static inline void __lidt(const void *base, u16 size) {
    struct {
        u16 limit;
        u64 base;
    } __attribute__((packed)) idt_ptr;
    idt_ptr.base = (u64)base;
    idt_ptr.limit = (sizeof(idt_entry_t) * size) - 1;

    asm volatile (
        "lidt %[g]"
        :
        : [g] "m" (idt_ptr)
    );
}


static inline void __ltr(u16 selector) {
    asm volatile (
        "ltr %%ax;"
        :
        : "a" (selector)
    );
}

static inline u64 __readcr0() {
    u64 cr0 = 0;
    asm volatile (
        "movq %%cr0, %[cr0]"
        : [cr0] "=r" (cr0)
    );
    return cr0;
}

static inline void __writecr0(u64 cr0) {
    asm volatile (
        "movq %[cr0], %%cr0"
        :
        : [cr0] "r" (cr0)
    );
}

static inline u64 __readcr3() {
    u64 cr3 = 0;
    asm volatile (
        "movq %%cr3, %[cr3]"
        : [cr3] "=r" (cr3)
    );
    return cr3;
}

static inline void __writecr3(u64 cr3) {
    asm volatile (
        "movq %[cr3], %%cr3"
        :
        : [cr3] "r" (cr3)
    );
}

static inline u64 __readcr4() {
    u64 cr4 = 0;
    asm volatile (
        "movq %%cr4, %[cr4]"
        : [cr4] "=r" (cr4)
    );
    return cr4;
}

static inline void __writecr4(u64 cr4) {
    asm volatile (
        "movq %[cr4], %%cr4"
        :
        : [cr4] "r" (cr4)
    );
}

static inline u64 __readcr8() {
    u64 cr8 = 0;
    asm volatile (
        "movq %%cr8, %[cr8]"
        : [cr8] "=r" (cr8)
    );
    return cr8;
}

static inline void __writecr8(u64 cr8) {
    asm volatile (
        "movq %[cr8], %%cr8"
        :
        : [cr8] "r" (cr8)
    );
}

static inline void __cpuid(u32 leaf, u32 subleaf, cpuid_t *cpuid) {
    cpuid->eax = 0;
    cpuid->ebx = 0;
    cpuid->ecx = 0;
    cpuid->edx = 0;
    asm volatile (
        "movl %4, %%eax;"
        "movl %5, %%ecx;"
        "cpuid;"
        : "=a" (cpuid->eax), "=b" (cpuid->ebx), "=c" (cpuid->ecx), "=d" (cpuid->edx)
        : "m" (leaf), "m" (subleaf)
    );
}

static inline void write_msr(u32 msr, u64 data) {
    u32 edx = (data >> 32) & 0xffffffff;
    u32 eax = data & 0xffffffff;

    asm volatile (
        "wrmsr"
        :
        : "d" (edx), "a" (eax), "c" (msr)
    );
}

static inline u64 read_msr(u32 msr) {
    u64 data;
    u32 edx;
    u32 eax;
    asm volatile (
        "rdmsr"
        : "=a" (eax), "=d" (edx)
        : "c" (msr)
    );
    data = (((u64)edx) << 32) | eax;
    return data;
}


static inline void __hang() {
    asm volatile (
        "hlt"
    );
}

static inline void __cli() {
    asm volatile (
        "cli"
    );
}

static inline void __sti() {
    asm volatile (
        "sti"
    );
}

static inline void __invlpg(void *addr) {
    asm volatile(
        "invlpg (%0)"
        :
        : "r" (addr)
        : "memory"
    );
}

static inline void __pause() {
    asm volatile("pause");
}

static inline u64 rdtsc() {
    u32 lo = 0, hi = 0;
    asm volatile(
        "rdtsc;"
        : "=a"(lo), "=d"(hi)
    );
    return ( (((u64)hi) << 32) | lo );
}

static inline void __lfence() {
    asm volatile(
        "lfence"
        :
        :
        : "memory"
    );
}

static inline void __barrier() {
    asm volatile(
        "mfence"
        :
        :
        : "memory"
    );
}

static inline u64 __rflags() {
    u64 rflags;
    asm volatile(
        "pushfq;"
        "popq %0;"
        : "=r" (rflags)
        :
        : "memory"
    );
    return rflags;
}


#endif //__instr_h__