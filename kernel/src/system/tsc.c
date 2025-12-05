#include <system/tsc.h>
#include <system/hpet.h>
#include <system/cpuf.h>
#include <ia32/cpuinstr.h>
#include <panic/panic.h>
#include <bootparams.h>



#define PIT_TICK_RATE 1193182
#define MAX_QUICK_PIT_MS 25
#define MAX_QUICK_PIT_ITERATIONS (MAX_QUICK_PIT_MS * PIT_TICK_RATE / 1000 / 256)



static u64 tsc_freq_khz = 0;


u64 __calibrate_tsc();

bool __pit_verify_msb(u8 val);
bool __pit_expect_msb(u8 val, u64 *tscp, u64 *deltap);
u64 __quick_pit_calibrate_tsc();

u64 __hpet_calibrate_tsc();


void tsc_init() {
    /*constant_tsc == 1 → 频率不随 P-State 变；
    invariant_tsc == 1 → C-State 深睡也继续走；
    两者都为 1 才放心把 TSC 当 wall-clock。*/
    if (!cpuf_check_pwm_ext(cpuf_get(), X86_PWMEXT_INVARIANT_TSC)) {
            klog_debug("tsc: TSC unstable\n");
    } else {
        klog_debug("tsc: TSC is realiable\n");
    }

    tsc_freq_khz = __calibrate_tsc();

    if (!tsc_freq_khz) {
        panic("tsc: Cannot get tsc frequency\n");
    }

    klog_debug("tsc: Detected %uq.%uq MHz processor\n", 
        tsc_freq_khz / 1000,
        tsc_freq_khz % 1000);

}


u64 __quick_pit_calibrate_tsc() {
    int i;
    u64 tsc = 0, delta = 0, d1 = 0, d2 = 0;


    port_outb(0x61, (port_inb(0x61) & ~0x2) | 0x1);

    /* counter 2, lo/hibyte, mode 0 */
    port_outb(0x43, 0xb0);

    port_outb(0x42, 0xff);  /*lobyte*/
    port_outb(0x42, 0xff);  /*hibyte*/

    __pit_verify_msb(0);

    if (__pit_expect_msb(0xff, &tsc, &d1)) {
        for (i = 1; i < MAX_QUICK_PIT_ITERATIONS; i++) {
            if (!__pit_expect_msb(0xff - i, &delta, &d2))
                break;

            /* 1 >> 11 == 1 / 2048 -> 488ppm */
            delta -= tsc;
            if (d1 + d2 >= delta >> 11)
                continue;

            if (!__pit_verify_msb(0xfe - i))
                break;
            
            goto _success;
        }
    }

    klog_debug("tsc: Failed to quick calibrate tsc using pit\n");
    return 0;

    _success:
    /*
     * kHz = ticks / time-in-seconds / 1000;
	 * kHz = (t2 - t1) / (I * 256 / PIT_TICK_RATE) / 1000
	 * kHz = ((t2 - t1) * PIT_TICK_RATE) / (I * 256 * 1000)
     */

    delta += (d2 - d1) / 2;
    delta *= PIT_TICK_RATE;
    delta /= (i * 256 * 1000);
    klog_debug("tsc: Quick pit calibrate tsc success\n");
    return delta;
}

inline bool __pit_verify_msb(u8 val) {
    port_inb(0x42);
    return port_inb(0x42) == val;
}

inline u8 __pit_verify_msb1(u8 val) {
    port_inb(0x42);
    return  port_inb(0x42);
}

inline bool __pit_expect_msb(u8 val, u64 *tscp, u64 *deltap) {
    int count;
    u64 tsc = 0;

    for (count = 0; count < 50000; count++) {
        if (!__pit_verify_msb(val)) {
            break;
        }
        
        tsc = rdtsc();
    }

    *deltap = rdtsc() - tsc;
    *tscp = tsc;

    return count > 5;
}


/*
 * qemu is not support cpiid.0x15,
 * so use hpet to calibrate tsc
 */
#define CALIBRATE_TICK_COUNT 100000000

u64 __hpet_calibrate_tsc() {
    u64 tsc_start, tsc_now;
    u64 hpet_start, hpet_now;

    if (!hpet_enable())
        return 0;

    tsc_start = rdtsc();
    hpet_start = hpet_get();

    do {
        hpet_now = hpet_get();
        tsc_now = rdtsc();
        asm volatile("rep; nop");
    } while((tsc_now - tsc_start) < CALIBRATE_TICK_COUNT || 
            (hpet_now - hpet_start) < CALIBRATE_TICK_COUNT);

    u64 tsc_cycles = tsc_now - tsc_start;
    u64 hpet_cycles = hpet_now - hpet_start;
    u64 hpet_times_ns = hpet_cycles * hpet_period() / 1000000;  /*fs -> ns*/
    return tsc_cycles * 1000000 / hpet_times_ns;
}

u64 __calibrate_tsc() {
    u64 tsc_khz = 0;

    tsc_khz = __quick_pit_calibrate_tsc();
    if (tsc_khz)
        return tsc_khz;

    tsc_khz = __hpet_calibrate_tsc();

    return tsc_khz;
}

/* TODO cpu 迁移问题 */
void tsc_delay(u64 xloops) {
    __lfence();
    u64 start = rdtsc(), now;
    for(;;) {
        __lfence();
        now = rdtsc();

        if (now - start >= xloops) {
            break;
        }

        __pause();
    }
}

void tsc_sleep_millis(u64 millis) {
    tsc_delay(tsc_freq_khz * millis);
}

u64 tsc_get_nanos() {
    __lfence();
    return rdtsc() / tsc_freq_khz / 1000000;
}

u64 tsc_get_millis() {
    __lfence();
    return rdtsc() / tsc_freq_khz;
}
