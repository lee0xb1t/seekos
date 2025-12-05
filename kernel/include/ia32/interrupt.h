#ifndef __interrupt_h__
#define __interrupt_h__

#include <lib/ktypes.h>


typedef struct {
	u16    isr_low;      // The lower 16 bits of the ISR's address
	u16    selector;     // The GDT segment selector that the CPU will load into CS before calling the ISR
	u8	   ist;          // The IST in the TSS that the CPU will load into RSP; set to zero for now
	u8     attributes;   // Type and attributes; see the IDT page
	u16    isr_mid;      // The higher 16 bits of the lower 32 bits of the ISR's address
	u32    isr_high;     // The higher 32 bits of the ISR's address
	u32    reserved;     // Set to zero
} __attribute__((packed)) idt_entry_t;

#endif //__interrupt_h__