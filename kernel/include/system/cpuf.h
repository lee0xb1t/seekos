#ifndef __CPUID_H
#define __CPUID_H
#include <lib/ktypes.h>
#include <ia32/ia32.h>


typedef struct {
    u32 leaf;
    u32 subleaf;
    enum cpuid_reg_e reg;
    u32 mask; 
}cpuid_feature_t;

static const cpuid_feature_t CPUID_FEATURE_APIC = {
    .leaf = 1,
    .subleaf = 0,
    .reg = CPUID_REG_EDX,
    .mask = 1 << 9
};

static const cpuid_feature_t CPUID_FEATURE_x2APIC = {
    .leaf = 1,
    .subleaf = 0,
    .reg = CPUID_REG_EDX,
    .mask = 1 << 21
};

typedef struct _cpu_info_t {
    u8 x86_family;
    u8 x86_model;
    u8 x86_vendor;
    u8 brand_index;
    u8 clflush_size;
    u8 x86_max_id;
    u8 x86_lapicid;
    u32 x86_feature_ecx;
    u32 x86_feature_edx;
    
    u32 x86_pwm;
    u32 x86_pwm_extend;
} cpu_info_t;


#define X86_VENDOR_INTEL    0
#define X86_VENDOR_AMD      1
#define X86_VENDOR_UNKNOW   0xff

#define X86_VENDOR_INTEL_EBX    0x756E6547      /*Genu*/
#define X86_VENDOR_INTEL_ECX    0x6C65746E      /*ntel*/
#define X86_VENDOR_INTEL_EDX    0x49656E69      /*ineI*/

#define X86_VENDOR_AMD_EBX      'htuA'
#define X86_VENDOR_AMD_ECX      'itne'
#define X86_VENDOR_AMD_EDX      'DMAc'



/*
 * edx << 32
 * ecx
 */
#define X86_FEATURE_SSE3        0x1

#define X86_FEATURE_FPU         (0x1ull << 32)


#define X86_PWM_NOSTOP_TSC      0x04
#define X86_PWMEXT_INVARIANT_TSC   0x10


bool cpuid_check_feature();

void cpuf_info_init();

cpu_info_t *cpuf_get();

static inline bool cpuf_check_feature(cpu_info_t *cpu_info, u64 mask) {
    if (mask > U32_MAX) {
        return (cpu_info->x86_feature_edx & mask);
    } else {
        return (cpu_info->x86_feature_ecx & mask);
    }
}

static inline bool cpuf_check_pwm(cpu_info_t *cpu_info, u64 mask) {
    return ( cpu_info->x86_pwm & mask );
}

static inline bool cpuf_check_pwm_ext(cpu_info_t *cpu_info, u64 mask) {
    return ( cpu_info->x86_pwm_extend & mask );
}

#endif