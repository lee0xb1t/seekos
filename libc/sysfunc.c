#include <libc/sysfunc.h>
#include <libc/types.h>


void sys_debug_log() {
    i32 ret;
    syscall_0(SYSCALL_DEBUGLOG, ret);
}

void sys_user_exit() {
    i32 ret;
    syscall_0(SYSCALL_USEREXIT, ret);
    
    // noreturn
    asm volatile("hlt");
}

i32 sys_open(const char *path, int mode) {
    i32 ret;
    syscall_2(SYSCALL_OPEN, path, mode, ret);
    return ret;
}

i32 sys_close(i32 handle) {
    i32 ret;
    syscall_1(SYSCALL_CLOSE, handle, ret);
    return ret;
}

i32 sys_read(i32 h, i32 len, char* buff) {
    i32 ret;
    syscall_3(SYSCALL_READ, h, len, buff, ret);
    return ret;
}

i32 sys_write(i32 h, i32 len, const char* buff) {
    i32 ret;
    syscall_3(SYSCALL_WRITE, h, len, buff, ret);
    return ret;
}

i32 sys_lseek(i32 h, i32 offset , i32 wence) {
    i32 ret;
    syscall_3(SYSCALL_LSEEK, h, offset, wence, ret);
    return ret;
}

i32 sys_panic(char *s, i32 errno) {
    i32 ret;
    syscall_2(SYSCALL_USER_PANIC, s, errno, ret);
    return ret;
}

void *sys_vmalloc(void *ptr, int sz) {
    void *ret;
    syscall_2(SYSCALL_VMALLOC, ptr, sz, ret);
    return ret;
}

void sys_vfree(void *ptr) {
    i32 ret;
    syscall_1(SYSCALL_VFREE, ptr, ret);
    return ret;
}

i32 sys_readdir(i32 h, void *data, int sz) {
    i32 ret;
    syscall_3(SYSCALL_READDIR, h, data, sz, ret);
    return ret;
}

u32 sys_fork() {
    u32 ret;
    syscall_0(SYSCALL_FORK, ret);
    return ret;
}

i32 sys_sleep(i32 millis) {
    i32 ret;
    syscall_1(SYSCALL_SLEEP, millis, ret);
    return ret;
}

void sys_execve(const char *path, int argc, char **argv) {
    i32 ret;
    syscall_3(SYSCALL_EXECVE, path, argc, argv, ret);
}

i32 sys_wait(u32 id) {
    i32 ret;
    syscall_1(SYSCALL_WAIT, id, ret);
    return ret;
}

void sys_getcwd(char *buf, size_t len) {
    i32 ret;
    syscall_2(SYSCALL_GETCWD, buf, len, ret);
}

i32 sys_chdir(char *path) {
    i32 ret;
    syscall_1(SYSCALL_CHDIR, path, ret);
    return ret;
}

u32 sys_spawn(const char *path, int argc, char **argv) {
    u32 ret;
    syscall_3(SYSCALL_SPAWN, path, argc, argv, ret);
    return ret;
}
