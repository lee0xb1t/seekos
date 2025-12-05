#ifndef __kmemory_h__
#define __kmemory_h__

#include <lib/ktypes.h>


void memset(void* dst, u8 val, uptr size);

int memcmp(void* m1, void* m2, uptr size);

void memcpy(void* dst, const void* src, uptr size);

#endif