#ifndef __apic_h__
#define __apic_h__
#include <lib/ktypes.h>


#define APIC_ID_REG              0x020
#define APIC_VER_REG             0x030

#define APIC_TPR                 0x080
#define APIC_PPR                 0x0A0
#define APIC_REG_EOI             0x0B0
#define APIC_SVR                 0x0F0

#define APIC_ICR_LOW             0x300
#define APIC_ICR_HIGH            0x310

#define APIC_REG_LVT_TIMER       0x320

#define APIC_TIMER_REG_DCR       0x3e0
#define APIC_TIMER_REG_ICR       0x380
#define APIC_TIMER_REG_CCR       0x390


#define APIC_LVT_CMCI_REG        0x02F0

// #define APIC_ISR_31_0            0x100
// #define APIC_ISR_31_0            0x100


#define APIC_MSR_BASE_BSP               0x100
#define APIC_MSR_BASE_XAPIC_ENABLE      0x800
#define APIC_MSR_BASE_X2APIC_ENABLE     0x400


#define APIC_ICR_DELIVERY_INIT          0x500
#define APIC_ICR_DELIVERY_STARTUP       0x600
#define APIC_ICR_LEVEL_ASSERT           0x4000
#define APIC_ICR_TRIGGER_LEVEL          0x00008000


#define APIC_LVT_MASK_FLAG              (1 << 16)


#define IOAPIC_REDTBL(n)                (0x10 + n * 2)

#define IOAPIC_NUM_MAX                  4


typedef struct _ioapic_data_t{
    uptr phys_addr;
    uptr virt_addr;
    u8 ioapic_id;
    u8 id;
    u8 version;
    u8 redir_tbl_entry;
    u32 gsi_base;
} ioapic_data_t;

typedef struct _ioapic_rte_t{
    union {
        struct {
            u64 vector          : 8;
            u64 delivery_mode   : 3;
#define IOAPIC_DEST_PHYSICAL    0
#define IOAPIC_DEST_LOGICAL     1
            u64 dest_mode       : 1;
            u64 delivery_status : 1;
#define IOAPIC_ACTIVE_HIGH      0
#define IOAPIC_ACTIVE_LOW       1
            u64 pin_polarity    : 1;
            u64 remote_irr      : 1;
#define IOAPIC_EDGE             0
#define IOAPIC_LEVEL            1
            u64 trigger_mode    : 1;
            u64 mask            : 1;
            u64 reserved        : 39;
            u64 destination     : 8;
        };
        u64 flags;
    };
} ioapic_rte_t;


void apic_init();
void apic_load();

u32 apic_reg_read(u16 offet);
void apic_reg_write(u16 offet, u32 val);
void apic_send_ipi(u32 destination, u32 vector, u32 delivery);
void apic_send_eoi();


void ioapic_init();
u32 ioapic_reg_read(uptr ioapic_base, u8 offset);
void ioapic_reg_write(uptr ioapic_base, u8 offset, u32 val);

ioapic_data_t *ioapic_get_by_irq(int irqno);
ioapic_rte_t ioapic_get_rtentry(int irqno);
void ioapic_set_rtentry(int irqno, ioapic_rte_t val);

u64 ioapic_read_rte(uptr ioapic_base, int irq_no);
void ioapic_write_rte(uptr ioapic_base, int irq_no, u64 val);

void ioapic_set_irq_vector(int irqno, int vector);
void ioapic_set_isa_irq_flags(int irqno, int vector, int flags);
void ioapic_set_irq_mask(int irqno, u8 mask);

int ioapic_get_max_irq();

#endif //__apic_h__