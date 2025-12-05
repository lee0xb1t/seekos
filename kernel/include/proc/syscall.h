#ifndef SYSCALL_H
#define SYSCALL_H
#include <lib/ktypes.h>


#define X86_EFLAGS_CF   0x00000001      /* Carry Flag */
#define X86_EFLAGS_PF   0x00000004      /* Parity Flag */
#define X86_EFLAGS_AF   0x00000010      /* Auxillary carry Flag */
#define X86_EFLAGS_ZF   0x00000040      /* Zero Flag */
#define X86_EFLAGS_SF   0x00000080      /* Sign Flag */
#define X86_EFLAGS_TF   0x00000100      /* Trap Flag */
#define X86_EFLAGS_IF   0x00000200      /* Interrupt Flag */
#define X86_EFLAGS_DF   0x00000400      /* Direction Flag */
#define X86_EFLAGS_OF   0x00000800      /* Overflow Flag */
#define X86_EFLAGS_IOPL 0x00003000      /* IOPL mask */
#define X86_EFLAGS_NT   0x00004000      /* Nested Task */
#define X86_EFLAGS_RF   0x00010000      /* Resume Flag */
#define X86_EFLAGS_VM   0x00020000      /* Virtual Mode */
#define X86_EFLAGS_AC   0x00040000      /* Alignment Check */
#define X86_EFLAGS_VIF  0x00080000      /* Virtual Interrupt Flag */
#define X86_EFLAGS_VIP  0x00100000      /* Virtual Interrupt Pending */
#define X86_EFLAGS_ID   0x00200000      /* CPUID detection flag */


#define SYSCALL_DEBUGLOG        0
#define SYSCALL_USEREXIT        1
#define SYSCALL_OPEN            2
#define SYSCALL_CLOSE           3
#define SYSCALL_READ            4
#define SYSCALL_WRITE           5
#define SYSCALL_LSEEK           6
#define SYSCALL_USER_PANIC      7
#define SYSCALL_VMALLOC         8
#define SYSCALL_VFREE           9
#define SYSCALL_READDIR         10
#define SYSCALL_FORK            11
#define SYSCALL_SLEEP           12
#define SYSCALL_EXECVE          13
#define SYSCALL_WAIT            14
#define SYSCALL_GETCWD          15
#define SYSCALL_CHDIR           16
#define SYSCALL_SPAWN           17


typedef i32 (*syscall_func_t)();


void syscall_init();

i32 sys_debug_log();
i32 sys_user_exit();


#define O_READ              0
#define O_WRITE             1
#define O_READWRITE         2

i32 sys_open(const char *path, int mode);
i32 sys_close(i32 handle);
i32 sys_read(i32 h, i32 len, char* buff);
i32 sys_write(i32 h, i32 len, const char* buff);
i32 sys_lseek(i32 h, i32 offset , i32 wence);
i32 sys_user_panic(char *s, i32 errno);
void *sys_vmalloc(void *ptr, int sz);
void sys_vfree(void *ptr);
i32 sys_readdir(i32 h, void *data, int sz);
u32 sys_fork();
i32 sys_sleep(i32 millis);
void sys_execve(const char *path, int argc, char **argv);
i32 sys_wait(u32 id);
void sys_getcwd(char *buf, size_t len);
i32 sys_chdir(char *path);
u32 sys_spawn(const char *path, int argc, char **argv);

#endif