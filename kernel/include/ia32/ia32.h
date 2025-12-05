#ifndef __ia32_h__
#define __ia32_h__

#include <lib/ktypes.h>


enum cpuid_reg_e{
    CPUID_REG_EDX = 0,
    CPUID_REG_ECX,
    CPUID_REG_EBX,
    CPUID_REG_EAX,
};

typedef struct {
    union {
        struct {
            u32 edx;
            u32 ecx;
            u32 ebx;
            u32 eax;
        };
        u32 regs[4];
    };
} cpuid_t;

typedef struct {
    union {
        u64 quad;
        
        struct {
            u32 lowpart;
            u32 highpart;
        };
    };
} msr_t;


#endif //__ia32_h__