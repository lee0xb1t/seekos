#ifndef __serial_h__
#define __serial_h__

#include <lib/ktypes.h>

#define COM1_BASE       0x3F8

bool serial_init();

u8 serial_read();
void serial_write(u8 data);

#endif //__serial_h__