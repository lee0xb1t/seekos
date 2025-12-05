#include <proc/syscall.h>
#include <proc/sched.h>
#include <system/gdt.h>
#include <system/isr.h>
#include <ia32/cpuinstr.h>
#include <ia32/msr.h>
#include <log/klog.h>
#include <log/kprint.h>
#include <fs/vfs.h>
#include <mm/kmalloc.h>
#include <lib/kmath.h>
#include <lib/kstring.h>
#include <lib/errno.h>
#include <lib/kmemory.h>


extern void (*syscall_handler0)();


volatile syscall_func_t syscall_funs[] = {
    [SYSCALL_DEBUGLOG] = (syscall_func_t)sys_debug_log,
    [SYSCALL_USEREXIT] = (syscall_func_t)sys_user_exit,
    [SYSCALL_OPEN] = (syscall_func_t)sys_open,
    [SYSCALL_CLOSE] = (syscall_func_t)sys_close,
    [SYSCALL_READ] = (syscall_func_t)sys_read,
    [SYSCALL_WRITE] = (syscall_func_t)sys_write,
    [SYSCALL_LSEEK] = (syscall_func_t)sys_lseek,
    [SYSCALL_USER_PANIC] = (syscall_func_t)sys_user_panic,
    [SYSCALL_VMALLOC] = (syscall_func_t)sys_vmalloc,
    [SYSCALL_VFREE] = (syscall_func_t)sys_vfree,
    [SYSCALL_READDIR] = (syscall_func_t)sys_readdir,
    [SYSCALL_FORK] = (syscall_func_t)sys_fork,
    [SYSCALL_SLEEP] = (syscall_func_t)sys_sleep,
    [SYSCALL_EXECVE] = (syscall_func_t)sys_execve,
    [SYSCALL_WAIT] = (syscall_func_t)sys_wait,
    [SYSCALL_GETCWD] = (syscall_func_t)sys_getcwd,
    [SYSCALL_CHDIR] = (syscall_func_t)sys_chdir,
    [SYSCALL_SPAWN] = (syscall_func_t)sys_spawn,
};


void syscall_init() {
    /*Enable syscall/sysret*/
    write_msr(IA32_EFER, read_msr(IA32_EFER) | 0x01);

    write_msr(IA32_STAR, ((u64)KERNEL_CODE_SELECTOR << 32) | ((u64)(USER_DATA_SELECTOR - 8) << 48));
    write_msr(IA32_LSTAR, (uptr)&syscall_handler0);
    write_msr(IA32_SFMASK, X86_EFLAGS_TF | X86_EFLAGS_DF | X86_EFLAGS_IF 
                            | X86_EFLAGS_IOPL | X86_EFLAGS_AC | X86_EFLAGS_NT);

    //

    if (percpu()->is_bsp) {
        klogi("SYSACALL Initialized.\n");
    }
}

i32 sys_debug_log() {
    klogi("This is test debug log while syscall.\n");
}

i32 sys_user_exit(i32 exitcode) {
    sched_exit(exitcode);
}

#define O_READ          0
#define O_WRITE         1
#define O_READWRITE     0

i32 sys_open(const char *path, int mode) {
    int pathlen = min(strlen(path), VFS_MAX_PATH_LENGTH);
    char *temp_path = (char *)kzalloc(pathlen+1);
    memcpy(temp_path, path, pathlen);
    vfs_handle_t fh = vfs_open(temp_path, mode);
    kfree(temp_path);
    return fh;
}

i32 sys_close(i32 handle) {
    return vfs_close((vfs_handle_t)handle);
}

i32 sys_read(i32 h, i32 len, char* buff) {
    return vfs_read(h, len, buff);
}

i32 sys_write(i32 h, i32 len, const char* buff) {
    return vfs_write(h, len, buff);
}

i32 sys_lseek(i32 h, i32 offset , i32 wence) {
    return vfs_lseek(h, offset, wence);
}

i32 sys_user_panic(char *s, i32 errno) {
    task_t *t = sched_get_task();
    kerrf("[%s](%d) %s\n", t->name, errno, s);
    sched_exit(errno);
}

void *sys_vmalloc(void *ptr, int sz) {
    task_t *t = sched_get_task();
    return vmalloc(t->mm, ptr, sz, VMM_FLAGS_USER);
}

void sys_vfree(void *ptr) {
    task_t *t = sched_get_task();
    vfree(t->mm, ptr);
}

struct dirent {
    char name[256];
#define DIRENT_DIRECTORY        0
#define DIRENT_FILE             1
    int type;
    u32 sz;
};

i32 sys_readdir(i32 h, void *data, int sz) {
    int filecnt = 0;
    vfs_dirent_t *dirent = null;

    if (sz < filecnt * sizeof(vfs_dirent_t)) {
        return -ENOBUFS;
    }

    vfs_iterate(h, &filecnt, &dirent);

    if (dirent == null) {
        return -ENOBUFS;
    }

    struct dirent *dp = (struct dirent *)data;

    for (int i = 0; i < filecnt; i++) {
        memcpy(dp[i].name, dirent[i].name, strlen(dirent[i].name));
        dp[i].sz = dirent[i].sz;
        if (dirent[i].sz == VFS_NODE_FILE) {
            dp[i].type = DIRENT_FILE;
        } else if (dirent[i].sz == VFS_NODE_DIRECTOR) {
            dp[i].type = DIRENT_DIRECTORY;
        }
    }

    kfree(dirent);

    return filecnt;
}

u32 sys_fork() {
    return sched_fork();
}

i32 sys_sleep(i32 millis) {
    sched_sleep(millis);
    return 0;
}

void sys_execve(const char *path, int argc, char **argv) {
    sched_execve(path, argc, argv, null);
}

i32 sys_wait(u32 id) {
    return sched_wait(id);
}

void sys_getcwd(char *buf, size_t len) {
    task_t *t = sched_get_task();
    memcpy(buf, t->cwd, min(strlen(t->cwd), len));
}

i32 sys_chdir(char *path) {
    i32 r = 0;
    char full_path[VFS_MAX_PATH_LENGTH] = {0};
    vfs_handle_t fh = vfs_open(path, VFS_MODE_READWRITE);
    r = vfs_get_full_path(fh, VFS_MAX_PATH_LENGTH, full_path);
    if (r == -1) {
        vfs_close(fh);
        return r;
    }
    task_t *t = sched_get_task();
    memset(t->cwd, 0, VFS_MAX_PATH_LENGTH);
    memcpy(t->cwd, full_path, strlen(full_path));
    vfs_close(fh);
    return r;
}

u32 sys_spawn(const char *path, int argc, char **argv) {
    return sched_spawn(path, argc, argv);
}
