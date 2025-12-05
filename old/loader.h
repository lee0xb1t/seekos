#ifndef __LOADER_H__
#define __LOADER_H__

#include "types.h"
#include "tty.h"
#include <common/boot_info.h>
#include <arch/segment.h>


#define SECTOR_SIZE 512

// > 1mb region
// 0x1000000
#define SYS_KERNEL_LOAD_ADDR (1024 * 1024 * 16)

///////////////////////////////////////
//
// Types
//


///////////////////////////////////////
//
// Variables
//

extern boot_info_t boot_info;


///////////////////////////////////////
//
// Prototypes
//

//
// entry 
//

void loader_main(void);
void loader32_main(void);


//
// memory
//
void detect_memory(void);


void halt(void);


#endif // __LOADER_H__