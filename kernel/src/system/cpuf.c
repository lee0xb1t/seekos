#include <system/cpuf.h>
#include <ia32/cpuinstr.h>
#include <log/klog.h>


static cpu_info_t cpu_info;

bool cpuid_check_feature(cpuid_feature_t cpuid_feature) {
    cpuid_t maxfunc;
    __cpuid(0, 0, &maxfunc);

    if (maxfunc.eax < cpuid_feature.leaf) {
        klog_debug("cpuid leaf %x not support!", cpuid_feature.leaf);
        return false;
    }

    cpuid_t c;
    __cpuid(cpuid_feature.leaf, cpuid_feature.subleaf, &c);

    if (c.regs[cpuid_feature.reg] & cpuid_feature.mask) {
        return true;
    }

    return false;
}

void cpuf_info_init() {
    memset(&cpu_info, 0, sizeof(cpu_info_t));

    cpuid_t maxfunc;
    __cpuid(0, 0, &maxfunc);

    switch (maxfunc.ebx)
    {
    case X86_VENDOR_INTEL_EBX:
        cpu_info.x86_vendor = X86_VENDOR_INTEL;
        break;
    case X86_VENDOR_AMD_EBX:
        cpu_info.x86_vendor = X86_VENDOR_AMD;
        break;
    default:
        cpu_info.x86_vendor = X86_VENDOR_UNKNOW;
    }

    klog_debug("\ncpuid.0.eax = %p  ebx = %d\n", maxfunc.eax, cpu_info.x86_vendor);


    cpuid_t cpuid_feature;
    __cpuid(1, 0, &cpuid_feature);
    klog_debug("cpuid.1.eax = %p  ebx = %p  ecx = %p  edx = %p\n", cpuid_feature.eax, cpuid_feature.ebx, cpuid_feature.ecx, cpuid_feature.edx);

    cpu_info.x86_model = cpuid_feature.eax & 0xff;
    cpu_info.x86_family = (cpuid_feature.eax >> 8) & 0xff;

    cpu_info.brand_index = cpuid_feature.ebx & 0xff;
    cpu_info.clflush_size = (cpuid_feature.ebx >> 8) & 0xff;
    cpu_info.x86_max_id = (cpuid_feature.ebx >> 16) & 0xff;
    cpu_info.x86_lapicid = (cpuid_feature.ebx >> 24) & 0xff;

    cpu_info.x86_feature_ecx = cpuid_feature.ecx;
    cpu_info.x86_feature_edx = cpuid_feature.edx;

    cpuid_t cpuid_0x6;
    __cpuid(6, 0, &cpuid_0x6);
    klog_debug("cpuid.6.eax = %p  ebx = %p  ecx = %p  edx = %p\n", cpuid_0x6.eax, cpuid_0x6.ebx, cpuid_0x6.ecx, cpuid_0x6.edx);
    cpu_info.x86_pwm = cpuid_0x6.eax;

    cpuid_t cpuid_pwm_extend;
    __cpuid(0x80000007, 0, &cpuid_pwm_extend);
    klog_debug("cpuid.0x80000007.eax = %p  ebx = %p  ecx = %p  edx = %p\n", cpuid_pwm_extend.eax, cpuid_pwm_extend.ebx, cpuid_pwm_extend.ecx, cpuid_pwm_extend.edx);
    cpu_info.x86_pwm_extend = cpuid_pwm_extend.edx;


    cpuid_t cpuid_;
    __cpuid(0x15, 0, &cpuid_);
    klog_debug("cpuid.0x15.eax = %p  ebx = %p  ecx = %p  edx = %p\n", cpuid_.eax, cpuid_.ebx, cpuid_.ecx, cpuid_.edx);

    __cpuid(0x16, 0, &cpuid_);
    klog_debug("cpuid.0x16.eax = %p  ebx = %p  ecx = %p  edx = %p\n", cpuid_.eax, cpuid_.ebx, cpuid_.ecx, cpuid_.edx);
    
    klog_debug("\n");
}

inline cpu_info_t *cpuf_get() {
    return &cpu_info;
}
