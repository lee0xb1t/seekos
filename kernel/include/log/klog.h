#ifndef __klog_h__
#define __klog_h__

#include <lib/ktypes.h>
#include <stdarg.h>

typedef enum {
    KLOG_SERIAL,
    KLOG_TERM
} klog_mode_t;

typedef enum {
    KL_TRACE = 1,
    KL_DEBUG,
    KL_WARN,
    KL_INFO,
    KL_ERROR,
} klog_level_t;


void klog_char(u32 color, char ch);
void klog_string(u32 color, const char* str);
void klog_int32(u32 color, i32 number);
void klog_uint32(u32 color, u32 number);
void klog_int64(u32 color, i64 number);
void klog_uint64(u32 color, u64 number);
void klog_hex(u32 color, u64 number, i32 wordcase);
void klog_binary(u32 color, u64 number);
void klog_vprintf(u32 color, const char* format, va_list args);

void klog_set_mode(klog_mode_t mode);
void klog_serial_putch(char ch);
void klog_set_level(klog_level_t l);


#define klogd klog_debug
#define klogi klog_info
#define klogw klog_warn
#define kloge klog_error

void klog_debug(const char* format, ...);
void klog_info(const char* format, ...);
void klog_warn(const char* format, ...);
void klog_error(const char* format, ...);

#endif //__klog_h__