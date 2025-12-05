#ifndef SYSFUNC_H
#define SYSFUNC_H
#include <libc/syscallu.h>
#include <libc/errno.h>
#include <libc/types.h>


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


void sys_debug_log();
void sys_user_exit();

i32 sys_open(const char *path, int mode);
i32 sys_close(i32 handle);
i32 sys_read(i32 h, i32 len, char* buff);
i32 sys_write(i32 h, i32 len, const char* buff);
i32 sys_lseek(i32 h, i32 offset , i32 wence);
i32 sys_panic(char *s, i32 errno);
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