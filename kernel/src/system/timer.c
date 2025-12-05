#include <system/timer.h>
#include <system/isr.h>
#include <system/apic.h>
#include <system/hpet.h>
#include <system/cpu.h>


static u8 divisor;
static u32 timer_freq_khz;

//extern void timer_handler();
void __timer_handler(trapframe_t *frame) {
    klog_debug("This is timer handler.\n");
    apic_send_eoi();
}


void apic_timer_init() {
    // isr_set_handler(APIC_TIMER_IRQ, __timer_handler, ISR_TYPE_LAPIC_TIMER);
    int vector = irq_to_vector(APIC_TIMER_IRQ);

    isr_register_irq(APIC_TIMER_IRQ);
    
    isr_set_handler_type(vector, ISR_TYPE_LAPIC_TIMER);

    apic_reg_write(APIC_REG_LVT_TIMER, APIC_LVT_MASK_FLAG | vector);
    apic_reg_write(APIC_TIMER_REG_DCR, 1);
    divisor = 4;

    apic_reg_write(APIC_TIMER_REG_ICR, U32_MAX);

    hpet_sleep_millis(100);

    timer_freq_khz = (U32_MAX - apic_reg_read(APIC_TIMER_REG_CCR)) / 100;

    klog_debug("APIC timer frequency: %d kHz, Divisor: 4, IRQ %d\n",
          timer_freq_khz, vector);
}

void apic_timer_set_period_ms(u64 millis) {
    u64 period = millis * timer_freq_khz;
    klog_debug("APIC timer period is %uq\n", period);
    apic_reg_write(APIC_TIMER_REG_ICR, period);
}

void apic_timer_set_handler(void (*h)(void *)) {
    isr_setup_irq_handler(APIC_TIMER_IRQ, h);
}

void apic_timer_stop() {
    apic_reg_write(APIC_REG_LVT_TIMER, APIC_LVT_MASK_FLAG | apic_reg_read(APIC_REG_LVT_TIMER));
}

void apic_timer_start() {
    apic_reg_write(APIC_REG_LVT_TIMER, (~APIC_LVT_MASK_FLAG) & apic_reg_read(APIC_REG_LVT_TIMER));
}

void apic_timer_set_mode(int mode) {
    apic_reg_write(APIC_REG_LVT_TIMER, apic_reg_read(APIC_REG_LVT_TIMER) | ((u32)mode << 17));
}
