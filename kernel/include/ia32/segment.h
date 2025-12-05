#ifndef __segment_h__
#define __segment_h__

#include <lib/ktypes.h>


//
// macros
//

//
// types
//

typedef struct {
    union
    {
        u16 flags;
        
        struct {
            u16 accessed: 1;
            u16 write_enable: 1;
            u16 expansion_direction: 1;
            u16 code_segment: 1;
            u16 descriptor_type: 1;
            u16 descriptor_privilege_level: 2;
            u16 present: 1;
            u16 limit_high: 4;
            u16 reversed: 1;
            u16 long_mode: 1;
            u16 default_big: 1;
            u16 granularity: 1;
        };
    };
}__attribute__((packed)) segment_attr_t;

typedef struct {
    u16    limit_low;
    u16    base_low;
    u8     base_middle;
    segment_attr_t attr;
    u8     base_high;
}__attribute__((packed)) gdt_entry_t;

typedef struct {
    u16     limit_low;
    u16     base_low;
    u8      base_middle;
    segment_attr_t attr;
    u8      base_high;
    u32     base_high32;
    u32     reverse;
}__attribute__((packed)) gdt_sysentry_t;

#endif //__segment_h__