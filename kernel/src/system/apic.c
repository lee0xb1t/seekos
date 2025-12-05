#include <system/apic.h>
#include <system/cpu.h>
#include <system/cpuf.h>
#include <system/madt.h>
#include <ia32/msr.h>
#include <ia32/cpuinstr.h>
#include <log/klog.h>
#include <panic/panic.h>
#include <mm/mm.h>
#include <base/spinlock.h>


static volatile void *lapic_base = null;
static volatile bool is_x2apic_mode = false;
static volatile bool is_eoi_boardcast = false;

DECLARE_SPINLOCK(xapic_lock);
DECLARE_SPINLOCK(ioapic_lock);


static volatile ioapic_data_t ioapic_data[IOAPIC_NUM_MAX];



void _apic_enable();
void _apic_disable_lvt();
void _apic_disable_8259A();


uptr apic_base_addr() {
    msr_t data;
    data.quad = read_msr(IA32_APIC_BASE);
    return ((u64)data.lowpart & 0xfffff000) | (((u64)data.highpart & 0x0f) << 32);
}

uptr apic_msr_get_base() {
    msr_t data;
    data.quad = read_msr(IA32_APIC_BASE);
    return data.quad;
}

uptr apic_enable_apic() {
    msr_t data;
    data.quad = read_msr(IA32_APIC_BASE);
    data.quad |= 0x80;
    write_msr(IA32_APIC_BASE, data.quad);
}

void apic_init() {
    klog_debug("\n");

    uptr apic_base = apic_msr_get_base();

    bool is_bsp = (apic_base & APIC_MSR_BASE_BSP) == APIC_MSR_BASE_BSP;
    bool is_xapic_enable = (apic_base & APIC_MSR_BASE_XAPIC_ENABLE) == APIC_MSR_BASE_XAPIC_ENABLE;
    bool is_x2apic_enable = (apic_base & APIC_MSR_BASE_X2APIC_ENABLE) == APIC_MSR_BASE_X2APIC_ENABLE;

    bool is_apic = cpuid_check_feature(CPUID_FEATURE_APIC);

    bool is_x2apic = cpuid_check_feature(CPUID_FEATURE_x2APIC);

    if (!is_apic) {
        panic("apic: This cpu isn't support APIC by cpuid check!");
    }

    if (!is_xapic_enable) {
        apic_enable_apic();
        if (!(apic_msr_get_base() & APIC_MSR_BASE_XAPIC_ENABLE)) {
            panic("apic: Enable xAPIC failed!");
        }
    } else {
        if (is_x2apic && is_xapic_enable && is_x2apic_enable) {
            is_x2apic_mode = true;
        } else {
            is_x2apic_mode = false;
        }
    }

    if (is_x2apic_mode) {
        panic("apic: This kernel isn't support x2APIC mode!");
    }

    
    lapic_base = MI_DIRECT_MAPPING_RANGE_START + apic_base_addr();
    vmm_map(lapic_base, apic_base_addr(), PAGE_SIZE, VMM_FLAGS_MMIO);

    klog_debug("apic: local Apic base %p\n", lapic_base);
    
    _apic_enable();
    klog_debug("apic: SVR %p\n", apic_reg_read(APIC_SVR));

    _apic_disable_lvt();

    klog_debug("apic: EOI Boardcast %s\n", is_eoi_boardcast ? "enable" : "disable");
    klog_debug("apic: local apic id[%d] - version[%d] is initialized.\n", apic_reg_read(APIC_ID_REG), apic_reg_read(APIC_VER_REG) & 0xff);

}

void apic_load() {
    uptr apic_base = apic_msr_get_base();
    bool is_xapic_enable = (apic_base & APIC_MSR_BASE_XAPIC_ENABLE) == APIC_MSR_BASE_XAPIC_ENABLE;
    if (!is_xapic_enable) {
        apic_enable_apic();
        if (!(apic_msr_get_base() & APIC_MSR_BASE_XAPIC_ENABLE)) {
            panic("apic: Enable xAPIC failed!");
        }
    }
    _apic_enable();
    _apic_disable_lvt();
}


u32 apic_reg_read(u16 offet) {
    if (is_x2apic_mode) {
        klog_debug("apic: Not support x2APIC");
    } else {
        asm volatile("mfence");
        return *(volatile u32 *)((uptr)lapic_base + offet);
    }
}

void apic_reg_write(u16 offet, u32 val) {
    if (is_x2apic_mode) {
        klog_debug("apic: Not support x2APIC");
    } else {
        *(volatile u32 *)((uptr)lapic_base + offet) = val;
        asm volatile("mfence");
    }
}

void apic_send_ipi(u32 destination, u32 vector, u32 delivery) {
    u64 flags;
    spin_lock_irq(&xapic_lock, flags);

    apic_reg_write(APIC_ICR_HIGH, (u32)destination << 24);
    apic_reg_write(APIC_ICR_LOW, delivery | vector);

    spin_unlock_irq(&xapic_lock, flags);
}

void apic_send_eoi() {
    apic_reg_write(APIC_REG_EOI, 1);
}


/**
 * Enable APIC and Supress EOI boardcast
 * 
 * https://wiki.osdev.org/APIC#Spurious_Interrupt_Vector_Register
 */
void _apic_enable() {
    uptr svr_value = apic_reg_read(APIC_SVR);

    // 低8位位映射到的irq向量号
    svr_value |= 0xffff;
    
    // 启用APIC
    svr_value |= (1ul << 8);

    u32 version = apic_reg_read(APIC_VER_REG);
    bool support_eoib_superssion = ( (version >> 24) & 1 );
    if (support_eoib_superssion) {
        svr_value |= (1ul << 12);
        is_eoi_boardcast = true;
    } else {
        is_eoi_boardcast = false;
    }

    apic_reg_write(APIC_SVR, svr_value);
}

/**
 * 早期初始化阶段, 未给lvt编写处理程序, 所以暂时屏蔽lvt
 * lvt默认disable, 不需要手动disable
 * see Intel SDM Vol3.12.5.1 
 *  This flag is set to 1 on reset.
 */
void _apic_disable_lvt() {
}

/**
 * uefi启动默认屏蔽
 */
void _apic_disable_8259A() {
}


void ioapic_init() {
    klog_debug("\n");

    u16 num_ioapic = madt_get_num_ioapic();
    klogd("[IOAPIC] ioapic number: %d\n", num_ioapic);

    for (int i = 0; i < num_ioapic; i++) {
        madt_record_ioapic_t *ioapic_rec = madt_get_ioapic(i);

        ioapic_data[i].phys_addr = ioapic_rec->ioapic_address;

        void *ioapic_base = (void *)(MI_DIRECT_MAPPING_RANGE_START + ioapic_rec->ioapic_address);
        vmm_map(ioapic_base, ioapic_rec->ioapic_address, 1, VMM_FLAGS_MMIO);
        ioapic_data[i].virt_addr = (uptr)ioapic_base;

        ioapic_data[i].ioapic_id = ioapic_rec->ioapic_id;
        ioapic_data[i].id = (ioapic_reg_read(ioapic_base, 0) >> 24) & 0xf;
        ioapic_data[i].version = ioapic_reg_read(ioapic_base, 1) & 0xff;
        ioapic_data[i].redir_tbl_entry = ((ioapic_reg_read(ioapic_base, 1) >> 16) & 0xff) + 1;
        ioapic_data[i].gsi_base = ioapic_rec->gsi_base;

        klogd("[IOAPIC] ioapic(%d): id = %d, gsi_base = %d, rentrt = %d\n",
                i, ioapic_data[i].id, ioapic_data[i].gsi_base, ioapic_data[i].redir_tbl_entry
            );

        // for (int j = 0; j < ioapic_data[i].redir_tbl_entry; j++) {
        //     klog_debug("rte(%d): %p\n", j, ioapic_read_rte((uptr)ioapic_base, j));
        // }
    }
}

// void *ioapic_get_base() {
//     //TODO
//     madt_record_ioapic_t *ioapic_rec = madt_get_ioapic(0);
//     return (void *)(MI_DIRECT_MAPPING_RANGE_START + ioapic_rec->ioapic_address);
// }

u32 ioapic_reg_read(uptr ioapic_base, u8 offset) {
    u64 flags;
    u32 value;
    spin_lock_irq(&ioapic_lock, flags);

    /*IOREGSEL*/
    *(volatile u32 *)ioapic_base = offset;
    /*IOWIN*/
    value = *(volatile u32 *)(ioapic_base + 0x10);

    spin_unlock_irq(&ioapic_lock, flags);
    return value;
}

void ioapic_reg_write(uptr ioapic_base, u8 offset, u32 val) {
    u64 flags;
    spin_lock_irq(&ioapic_lock, flags);
    
    /*IOREGSEL*/
    *(volatile u32 *)ioapic_base = offset;
    /*IOWIN*/
    *(volatile u32 *)(ioapic_base + 0x10) = val;

    spin_unlock_irq(&ioapic_lock, flags);
}

u64 ioapic_read_rte(uptr ioapic_base, int irq_no) {
    u64 flags;
    // spin_lock_irq(&ioapic_lock, flags);

    u64 val = 0;
    val |= ioapic_reg_read(ioapic_base, IOAPIC_REDTBL(irq_no) + 1);
    val <<= 32;
    val |= ioapic_reg_read(ioapic_base, IOAPIC_REDTBL(irq_no));

    // spin_unlock_irq(&ioapic_lock, flags);
    return val;
}

void ioapic_write_rte(uptr ioapic_base, int irq_no, u64 val) {
    u64 flags;
    // spin_lock_irq(&ioapic_lock, flags);

    ioapic_reg_write(ioapic_base, IOAPIC_REDTBL(irq_no), val & 0xffffffff);
    ioapic_reg_write(ioapic_base, IOAPIC_REDTBL(irq_no) + 1, (val >> 32) & 0xffffffff);

    // spin_unlock_irq(&ioapic_lock, flags);
}

ioapic_data_t *ioapic_get_by_irq(int irqno) {
    u64 flags;
    spin_lock_irq(&ioapic_lock, flags);

    for (int i = 0; i < IOAPIC_NUM_MAX; i++) {
        if (irqno >= ioapic_data[i].gsi_base && 
            irqno < ioapic_data[i].redir_tbl_entry) {
            
            spin_unlock_irq(&ioapic_lock, flags);
            return &ioapic_data[i];
        }
    }

    spin_unlock_irq(&ioapic_lock, flags);
    return null;
}

ioapic_rte_t ioapic_get_rtentry(int irqno) {
    ioapic_rte_t rte;
    ioapic_data_t *ioapic_data = ioapic_get_by_irq(irqno); 
    rte.flags = ioapic_read_rte(ioapic_data->virt_addr, irqno);
    return rte;
}

void ioapic_set_rtentry(int irqno, ioapic_rte_t val) {
    ioapic_data_t *ioapic_data = ioapic_get_by_irq(irqno);
    ioapic_write_rte(ioapic_data->virt_addr, irqno, val.flags);
}

int ioapic_get_max_irq() {
    int max_irq = 0;
    for (int i = 0; i < IOAPIC_NUM_MAX; i++) {
        max_irq += ioapic_data[i].redir_tbl_entry;
    }
    return max_irq;
}

void ioapic_set_irq_vector(int irqno, int vector) {
    ioapic_rte_t rte = ioapic_get_rtentry(irqno);
    rte.trigger_mode = IOAPIC_EDGE;
    rte.vector = vector;
    // TODO get current cpu id

    ioapic_set_rtentry(irqno, rte);
}

void ioapic_set_isa_irq_flags(int irqno, int vector, int flags) {
    ioapic_rte_t rte = ioapic_get_rtentry(irqno);
    rte.vector = vector;
    rte.dest_mode = IOAPIC_DEST_PHYSICAL;
    rte.destination = 0;
    rte.mask = 1;

    u8 polarity = flags & 0x3;
    if (polarity == 0b00 || polarity == 0b11) {
        rte.pin_polarity = IOAPIC_ACTIVE_LOW;
    } else if (polarity == 0b01) {
        rte.pin_polarity = IOAPIC_ACTIVE_HIGH;
    } else {
        panic("[IOAPIC] ioapic_set_isa_irq_flags: invalid polarity %d", polarity);
    }

    u8 trigger_mode = (flags >> 2) & 0x3;
    if (trigger_mode == 0b00 || trigger_mode == 0b01) {
        rte.trigger_mode = IOAPIC_EDGE;
    } else if (trigger_mode == 0b11) {
        rte.trigger_mode = IOAPIC_LEVEL;
    } else {
        panic("[IOAPIC] ioapic_set_isa_irq_flags: invalid trigger mode %d", trigger_mode);
    }

    ioapic_set_rtentry(irqno, rte);
}

void ioapic_set_irq_mask(int irqno, u8 mask) {
    ioapic_rte_t rte = ioapic_get_rtentry(irqno);
    rte.mask = mask;
    ioapic_set_rtentry(irqno, rte);
}
