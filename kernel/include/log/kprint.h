#ifndef __kprint_h__
#define __kprint_h__

#include <lib/ktypes.h>
#include <stdarg.h>


typedef enum {
    KLOG_SERIAL,
    KLOG_TERM
} klog_mode_t;

typedef enum {
    KL_TRACE = 1,
    KL_DEBUG,
    KL_INFO,
    KL_WARN,
    KL_ERROR,
} klog_level_t;


void klog_set_mode(klog_mode_t mode);
void klog_set_level(klog_level_t lv);
void klog_serial_putch(char ch);


int ksnprintf(char *buf, size_t n, const char *fmt, ...);
int kvsnprintf(char *buf, size_t n, const char *fmt, va_list args);

int kprintf(const char *fmt, ...);
int kprintf_color(u32 color, const char *fmt, ...);

void klog_write(klog_level_t lv, const char *fmt, ...);
void klog_vprintf(u32 color, const char *fmt, va_list args);
int  kprint_core(u32 color, const char *fmt, va_list args);
u32  klog_level_color(klog_level_t lv);

#define klogd(...)       klog_write(KL_DEBUG, __VA_ARGS__)
#define klogi(...)       klog_write(KL_INFO,  __VA_ARGS__)
#define klogw(...)       klog_write(KL_WARN,  __VA_ARGS__)
#define kloge(...)       klog_write(KL_ERROR, __VA_ARGS__)

#define klog_debug(...)  klog_write(KL_DEBUG, __VA_ARGS__)
#define klog_info(...)   klog_write(KL_INFO,  __VA_ARGS__)
#define klog_warn(...)   klog_write(KL_WARN,  __VA_ARGS__)
#define klog_error(...)  klog_write(KL_ERROR, __VA_ARGS__)

#define kerrf(...)       klog_write(KL_ERROR, __VA_ARGS__)


void kpanic_printf(const char *fmt, ...);
void kpanic_clear(void);


#endif
