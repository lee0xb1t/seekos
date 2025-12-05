#ifndef __klayout_h__
#define __klayout_h__

#include <lib/ktypes.h>


extern u8 __text_start[], __text_end[];
extern u8 __data_start[], __end_start[];
extern u8 __rodata_start[], __rodata_end[];
extern u8 __bss_start[], __bss_end[];

extern u8 __kernel_start[], __kernel_end[];

#endif //__klayout_h__