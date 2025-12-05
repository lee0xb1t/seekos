#include <proc/task.h>
#include <lib/kstring.h>
#include <lib/kmemory.h>
#include <base/spinlock.h>
#include <base/kmutex.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <system/isr.h>
#include <system/gdt.h>
#include <panic/panic.h>


task_id_t global_id = 0;
DECLARE_SPINLOCK(global_id_lock);

DECLARE_SPINLOCK(task_lock);

extern void __sched_task_start();
extern void __sched_task_start_user();


task_t *task_create(const char *name, task_routine_t task_routine, 
                    task_priority_t task_priority, task_mode_t mode) {

    task_t *ntask = kzalloc(sizeof(task_t));

    spin_lock(&global_id_lock);
    ntask->tgid = ++global_id;
    spin_unlock(&global_id_lock);

    ntask->tid = ntask->tgid;

    ntask->mm = mm_create();

    ntask->priority = task_priority;
    ntask->mode = mode;
    ntask->state = TASK_READY;

    strncpy(&ntask->name[0], name, TASK_MAX_NAME_LEN);

    ntask->wakeup_event = (kevent_t *)kzalloc(sizeof(kevent_t));

    dlist_init(&ntask->open_files);

    if (mode == TASK_USER_MODE) {
        task_create_kstack_user(ntask, false);
        ntask->start_routine = task_routine;

    } else {
        uptr klimit = vmalloc(null, null, TASK_STACK_SIZE_32KB, VMM_FLAGS_DEFAULT);
        uptr kstack = ((uptr)klimit + TASK_STACK_SIZE_32KB);

        kstack -= sizeof(first_frame_t);
        first_frame_t *first_frame = (first_frame_t *)kstack;

        kstack -= sizeof(task_gr_t);
        task_gr_t *task_gr = (task_gr_t *)kstack;

        ntask->kstack = kstack;
        ntask->klimit = klimit;
        ntask->kstack_ptr = klimit;

        ntask->ustack = null;
        ntask->ulimit = null;
        ntask->ustack_ptr = null;

        ntask->tstack = kstack;
        ntask->tlimit = klimit;

        ntask->start_routine = task_routine;
        ntask->system_routine = __sched_task_start;


        first_frame->func = ntask->system_routine;
        first_frame->func0 = ntask->start_routine;
        first_frame->param0 = 0;

        ntask->frist_frame = first_frame;
        ntask->task_gr = task_gr;
    }

    /*
     * TODO kernel task有两条运行路径
     * 1. 创建后直接切换
     * 2. 中断进入后通过修改trapframe切换
     */
    

    return ntask;
}

void task_create_kstack_user(task_t *ntask, bool replace) {
    uptr klimit = vmalloc(null, null, TASK_STACK_SIZE_32KB, VMM_FLAGS_DEFAULT);
    uptr kstack = ((uptr)klimit + TASK_STACK_SIZE_32KB);

    // ustack
    // uptr ulimit = vmalloc(ntask->mm, null, TASK_STACK_SIZE_32KB, VMM_FLAGS_USER);
    // uptr ustack = ((uptr)ulimit + TASK_STACK_SIZE_32KB);

    // kernel enter
    kstack -= sizeof(trapframe_t);
    trapframe_t *trapf = (trapframe_t *)kstack;
    memset(trapf, 0, sizeof(trapframe_t));

    kstack -= sizeof(first_frame_t);
    first_frame_t *first_frame = (first_frame_t *)kstack;
    memset(first_frame, 0, sizeof(first_frame_t));

    kstack -= sizeof(task_gr_t);
    task_gr_t *task_gr = (task_gr_t *)kstack;
    memset(task_gr, 0, sizeof(task_gr_t));

    // user enter
    // ustack -= sizeof(first_frame_t);
    // first_frame_t *first_frame_user = (first_frame_t *)ustack;


    if (replace) {
        ntask->rstack = kstack;
        ntask->rlimit = klimit;
        ntask->rstack_ptr = klimit;
    } else {
        ntask->kstack = kstack;
        ntask->klimit = klimit;
        ntask->kstack_ptr = klimit;
    }

    ntask->tstack = kstack;
    ntask->tlimit = klimit;

    // ntask->ustack = ustack;
    // ntask->ulimit = ulimit;
    // ntask->ustack_ptr = ulimit;

    
    ntask->start_routine = 0;
    ntask->system_routine = __sched_task_start_user;

    // kernel steup
    first_frame->func = ntask->system_routine;
    first_frame->func0 = 0;
    first_frame->param0 = 0;

    ntask->frist_frame = first_frame;
    ntask->task_gr = task_gr;

    // user setup
    trapf->ss = USER_DATA_SELECTOR;
    // trapf->rsp = ustack;
    trapf->rflags = 0x202;
    trapf->cs = USER_CODE_SELECTOR;
    // trapf->rip = ntask->start_routine;

    // ntask->frist_frame_user = first_frame_user;
    ntask->trap_frame = trapf;
}

//TODO:  free on wait zombie
void task_free(task_t *task) {
    kfree(task->kstack_ptr);
    mm_free(task->mm);
    kfree(task);
}

void task_free_phase0(task_t *task) {
    if (task->mode == TASK_USER_MODE) {
        vfree(task->mm, task->ustack_ptr);

        if (task->u_argv)
            kfree(task->u_argv);

    }

    // free open files
    while (!dlist_is_empty(&task->open_files)) {
        linked_list_t *next = null;
        dlist_get_next(&task->open_files, &next);

        if (next) {
            vfs_file_t *vfsfile = dlist_container_of(next, vfs_file_t, open_list_entry);
            if (vfsfile->is_console) {
                vfs_close_console(vfsfile->f_handle);
            } else {
                vfs_close(vfsfile->f_handle);
            }
        }
        next = null;
    }

    kfree(task->wakeup_event);
}

void task_setup_ustack(task_t *t, void *ustack, u32 size) {
    t->ustack_ptr = ustack;
    t->ustack = (void *)((u8 *)ustack + size);
    t->ulimit = ustack;
    t->trap_frame->rsp = t->ustack;
}

void task_setup_routine(task_t *t, task_routine_t task_routine) {
    t->start_routine = task_routine;
    t->trap_frame->rip = task_routine;
}

void task_setup_path(task_t *t, const char *path) {
    memcpy(t->execve_path, path, strlen(path));
}

void task_setup_argv(task_t *t, int argc, char **argv) {
    if (!argc) {
        return;
    }

    t->u_argc = argc;
    t->u_argv = (char *)kzalloc(argc * TASK_MAX_ARGV_LEN);
    for (int i = 0; i < argc; i++) {
        char *arg = task_argv(t, 0);    
        memcpy(arg, argv[i], strlen(argv[i]));
    }
}

void task_setup_cwd(task_t *t, char *cwd) {
    memcpy(t->cwd, cwd, strlen(cwd));
}

task_t *task_fork(task_t *p) {
    u64 flags;

    spin_lock_irq(&task_lock, flags);

    if (p->mode == TASK_KERNEL_MODE) {
        spin_unlock_irq(&task_lock, flags);
        panic("cannot fork kernel task");
    }

    task_t *ntask = kzalloc(sizeof(task_t));
    memcpy(ntask, p, sizeof(task_t));

    spin_lock(&global_id_lock);
    ntask->tid = ++global_id;
    spin_unlock(&global_id_lock);

    // mm copy
    ntask->mm = mm_copy(p->mm);

    dlist_init(&ntask->open_files);
    
    // copy args
    ntask->u_argc = p->u_argc;
    ntask->u_argv = (char *)kzalloc(p->u_argc * TASK_MAX_ARGV_LEN);
    memcpy(ntask->u_argv, p->u_argv, p->u_argc * TASK_MAX_ARGV_LEN);


    ntask->state = TASK_READY;
    ntask->wakeup_event = (kevent_t *)kzalloc(sizeof(kevent_t));

    task_create_kstack_user(ntask, false);

    //
    ntask->is_fork = true;

    spin_unlock_irq(&task_lock, flags);

    return ntask;
}

task_t *task_replace(task_t *ntask, const char *name) {
    u64 flags;

    spin_lock_irq(&task_lock, flags);
    
    if (ntask->mode == TASK_KERNEL_MODE) {
        spin_unlock_irq(&task_lock, flags);
        panic("cannot replace kernel task");
    }

    kfree(ntask->wakeup_event);
    ntask->wakeup_event = (kevent_t *)kzalloc(sizeof(kevent_t));

    //memset(ntask->kstack_ptr, 0, TASK_STACK_SIZE_32KB);
    // ntask->tstack = ntask->kstack;
    // ntask->tlimit = ntask->klimit;

    ntask->mm = mm_replace(ntask->mm);
    // ntask->rmm = mm_create();

    ntask->state = TASK_READY;

    strncpy(&ntask->name[0], name, TASK_MAX_NAME_LEN);

    ntask->wakeup_event = (kevent_t *)kzalloc(sizeof(kevent_t));

    // TODO fs_struct
    while (!dlist_is_empty(&ntask->open_files)) {
        linked_list_t *next = null;
        dlist_get_next(&ntask->open_files, &next);

        if (next) {
            vfs_file_t *vfsfile = dlist_container_of(next, vfs_file_t, open_list_entry);
            if (vfsfile->is_console) {
                vfs_close_console(vfsfile->f_handle);
            } else {
                vfs_close(vfsfile->f_handle);
            }
        }
        next = null;
    }

    task_create_kstack_user(ntask, true);

    //
    ntask->is_fork = false;

    spin_unlock_irq(&task_lock, flags);
}

void task_init_ustack(task_t *t) {
    void *argv;
    uptr *argv_ptr;

    uptr *argvp;
    uptr *argcp;

    int argc = t->u_argc+1;
    u8 *ustack = t->ustack;


    ustack -= (argc * TASK_MAX_ARGV_LEN);
    argv = ustack;

    ustack -= (argc * sizeof(void*));
    argv_ptr = ustack;

    ustack -= (sizeof(uptr));
    argvp = ustack;

    ustack -= (sizeof(uptr));
    argcp = ustack;

    // index 0 is path
    memcpy((u8*)argv, t->execve_path, TASK_MAX_ARGV_LEN);
    argv_ptr[0] = (u8*)argv;

    for (int i = 1; i < argc-1; i++) {
        memcpy((u8*)argv + i * TASK_MAX_ARGV_LEN, t->u_argv[i*TASK_MAX_ARGV_LEN], TASK_MAX_ARGV_LEN);
        argv_ptr[i] = (u8*)argv + i * TASK_MAX_ARGV_LEN;
    }

    *argvp = argv_ptr;
    *argcp = argc;

    t->ustack = ustack;
    t->trap_frame->rsp = t->ustack;
}
