#ifndef __msr_h__
#define __msr_h__

#define IA32_APIC_BASE          0x1b
#define IA32_FS_BASE_MSR        0xC0000100
#define IA32_GS_BASE_MSR        0xC0000101
#define IA32_KERNEL_GS_BASE     0xC0000102

#define IA32_SYSENTER_CS        0x174
#define IA32_SYSENTER_ESP       0x175
#define IA32_SYSENTER_EIP       0x176

#define IA32_STAR               0xC0000081
#define IA32_LSTAR              0xC0000082
#define IA32_CSTAR              0xC0000083
#define IA32_SFMASK             0xC0000084
#define IA32_EFER               0xC0000080

#endif //__msr_h__