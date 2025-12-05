#ifndef __taskstate_h__
#define __taskstate_h__

#include <lib/ktypes.h>


typedef struct {
    u32 reversed0;

    union {
        struct {
            u32 rsp0_low;
            u32 rsp0_high;
        };
        u64 rsp0;
    };

    union {
        struct {
            u32 rsp1_low;
            u32 rsp1_high;
        };
        u64 rsp1;
    };

    union {
        struct {
            u32 rsp2_low;
            u32 rsp2_high;
        };
        u64 rsp2;
    };
    
    u32 reversed1;
    u32 reversed2;

    union {
        struct {
            u32 ist1_low;
            u32 ist1_high;
        };
        u64 ist1;
    };

    union {
        struct {
            u32 ist2_low;
            u32 ist2_high;
        };
        u64 ist2;
    };

    union {
        struct {
            u32 ist3_low;
            u32 ist3_high;
        };
        u64 ist3;
    };

    union {
        struct {
            u32 ist4_low;
            u32 ist4_high;
        };
        u64 ist4;
    };

    union {
        struct {
            u32 ist5_low;
            u32 ist5_high;
        };
        u64 ist5;
    };

    union {
        struct {
            u32 ist6_low;
            u32 ist6_high;
        };
        u64 ist6;
    };

    union {
        struct {
            u32 ist7_low;
            u32 ist7_high;
        };
        u64 ist7;
    };

    u32 reversed3;
    u32 reversed4;
    u16 reversed5;
    u16 iomap_base;
    
} __attribute__((packed)) tss64_t;

#endif //__taskstate_h__
