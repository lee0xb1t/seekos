#ifndef __kstatus_h__
#define __kstatus_h__

#include <lib/ktypes.h>

typedef u32 kstatus_t;


#define KSUCCESS(st) \
        (((kstatus_t)(st)) >= 0)


#define KSTATUS_SUCCESS                         ((u32)0)

#define KSTATUS_PHYSICAL_BITMAP_NOT_SET         ((u32)0x80000001)
#define KSTATUS_PHYSICAL_BITMAP_NOT_CLEAR       ((u32)0x80000002)

#endif //__kstatus_h__