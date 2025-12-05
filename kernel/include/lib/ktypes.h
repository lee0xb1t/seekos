#ifndef __kdef_h__
#define __kdef_h__

#define null            ((void*)0)
#define true            ((unsigned char)1)
#define false           ((unsigned char)0)

typedef unsigned char       bool;
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef int                 i32;
typedef unsigned long       u64;
typedef long                i64;
typedef unsigned long       size_t;
typedef unsigned long       uptr;

#define U32_MAX         ((u32)0xffffffff)
#define U64_MAX         ((u64)0xffffffffffffffff)

#define I32_MAX         ((i32)0x7fffffff)
#define I64_MAX         ((i32)0x7fffffffffffffff)

#define BITS_PER_U64    (64)
#define BYTES_PER_U64   (64 / 8)

#endif //__xdef_h__