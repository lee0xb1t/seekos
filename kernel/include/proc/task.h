#ifndef __TASK_H
#define __TASK_H
#include <lib/ktypes.h>
#include <system/isr.h>
#include <fs/vfs.h>
#include <mm/mm.h>


#define TASK_ID_EMPTY         U64_MAX

#define TASK_MAX_NAME_LEN   64
#define TASK_MAX_ARGV_LEN   32

#define TASK_STACK_SIZE_4KB     (4 * 1024)
#define TASK_STACK_SIZE_16KB     (16 * 1024)
#define TASK_STACK_SIZE_32KB     (32 * 1024)

typedef u64 task_id_t;
typedef u8 task_priority_t;

typedef void (* task_routine_t)(void *);


typedef struct {
    uptr func;
    uptr func0;
    uptr param0;
} __attribute__((packed)) first_frame_t;

typedef struct {
    u64 rflags;
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    u64 rdi;
    u64 rsi;
    u64 rbp;
    u64 rbx;
    u64 rdx;
    u64 rcx;
    u64 rax;
} __attribute__((packed)) task_gr_t;

typedef enum {
    TASK_KERNEL_MODE,
    TASK_USER_MODE
} task_mode_t;

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_WAITING,
    TASK_STOPPED,
    TASK_ZOMBIE,
    TASK_DEAD,
} task_state_t;


#define EV_TYPE_MIN     EV_UNKNOWN
#define EV_TYPE_MAX     EV_MAX

typedef enum _kevent_type_t{
    EV_UNKNOWN = 0,
    EV_KEYBOARD,
    EV_SLEEP,
    //...
    EV_MAX
} kevent_type_t;


#define EV_DATA_NULL        0
typedef u64 kevent_data_t;

typedef struct _kevent_t {
    task_id_t publisher;
    task_id_t subscriber;
    kevent_type_t type;
    kevent_data_t data;
    u64 timestamp;

    linked_list_t list_entry;
} kevent_t;


typedef struct _auxv_t {
    uptr entry;
    uptr phdr;
    u32 phent;
    u32 phnum;
} auxv_t;

typedef struct _task_t{
    task_id_t tgid;
    task_id_t tid;

    void *kstack;
    void *klimit;

    void *ustack;
    void *ulimit;
    
    void *tstack;
    void *tlimit;

    mm_struct_t *mm;

    task_priority_t priority;
    task_mode_t mode;
    task_state_t state;

    task_routine_t start_routine;
    void* system_routine;

    char name[TASK_MAX_NAME_LEN];

    linked_list_t list; /*active task list*/

    void *ustack_ptr;
    void *kstack_ptr;

    kevent_t *wakeup_event;

    linked_list_t open_files;   /*file handles*/

    linked_list_t child_list; // TODO
    
    // kernel enter
    task_gr_t *task_gr;
    first_frame_t *frist_frame;

    // user enter
    trapframe_t *trap_frame;
    first_frame_t *frist_frame_user;

    auxv_t auxv;

    // user variables
    char execve_path[VFS_MAX_PATH_LENGTH];
    int u_argc;
    char *u_argv;

    char cwd[VFS_MAX_PATH_LENGTH];

    i32 exitcode;
    i32 errno;
    bool is_fork;

    void *rstack;
    void *rlimit;
    void *rstack_ptr;

    // mm_struct_t *rmm;

    void *old_kstack_ptr;
    // void *old_mm;
} task_t;

#define task_argv(task, idx)  \
    &(task)->u_argv[(idx) * TASK_MAX_ARGV_LEN]


void task_init();

task_t *task_create(const char *name, task_routine_t task_routine, 
    task_priority_t task_priority, task_mode_t mode);
void task_create_kstack_user(task_t *, bool replace);

void task_free(task_t *task);
void task_free_phase0(task_t *task);

void task_setup_ustack(task_t *, void *ustack, u32 size);
void task_setup_routine(task_t *, task_routine_t task_routine);
void task_setup_path(task_t *, const char *path);
void task_setup_argv(task_t *, int argc, char **argv);
void task_setup_cwd(task_t *, char *cwd);

task_t *task_fork(task_t *p);
task_t *task_replace(task_t *n, const char *name);

void task_init_ustack(task_t *t);

#endif