#ifndef __xbit_h__
#define __xbit_h__

#include <efi.h>
#include <xtypes.h>

void clear_screen();


//
// Screen IO
//

void print_char(char ch);

void print_string(const char *str);

void print_int32(int32_t number);

void print_uint32(uint32_t number);

void print_int64(int64_t number);

void print_uint64(uint64_t number);

void print_hex(uint64_t number, int wordcase);

void print_binary(uint64_t number);

void print_char16(CHAR16 *str);

void printv(const char *format, ...);

int gets(char *buffer, int length);



//
// Memory
//

void memzero(void *buffer, int length);
int memcmp(const void *mem1, const void *mem2, int len);
void memcpy(void *dest, void *src, size_t len);

int strlen(const char *buffer);
int strcmp(const char *buffer1, const char *buffer2);



//
// Allocator
//

EFI_STATUS alloc_pool(
    IN EFI_MEMORY_TYPE            PoolType,
    IN UINTN                      Size,
    OUT VOID                      **Buffer
);

EFI_STATUS free_pool(
    IN VOID           *Buffer
);

EFI_STATUS alloc_pages(
    IN EFI_ALLOCATE_TYPE            Type,
    IN EFI_MEMORY_TYPE              MemoryType,
    IN UINTN                        Pages,
    IN OUT EFI_PHYSICAL_ADDRESS     *Memory
);

EFI_STATUS free_pages(
    IN EFI_PHYSICAL_ADDRESS    Memory,
    IN UINTN                   Pages
);



//
// Utils
//
void c2w(CHAR16* wbuffer, char* buffer, int size);
void w2c(char* buffer, CHAR16* wbuffer, int size);



#endif //__xbit_h__