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


static task_id_t global_id = 0;
DECLARE_SPINLOCK(global_id_lock);
DECLARE_SPINLOCK(task_lock);

extern void __sched_task_start(void);
extern void __sched_task_start_user(void);


static task_id_t _alloc_tid(void) {
    task_id_t id;
    u64 flags;
    spin_lock_irq(&global_id_lock, flags);
    id = ++global_id;
    spin_unlock_irq(&global_id_lock, flags);
    return id;
}

static void _task_setup_kstack(task_t *t) {
    uptr klimit = vmalloc(null, null, TASK_STACK_SIZE_32KB, VMM_FLAGS_DEFAULT);
    uptr kstack = klimit + TASK_STACK_SIZE_32KB;

    kstack -= sizeof(first_frame_t);
    first_frame_t *ff = (first_frame_t *)kstack;

    kstack -= sizeof(task_gr_t);
    task_gr_t *gr = (task_gr_t *)kstack;

    t->kstack      = (void *)kstack;
    t->klimit       = (void *)klimit;
    t->kstack_ptr   = (void *)klimit;
    t->tstack       = (void *)kstack;
    t->tlimit       = (void *)klimit;
    t->task_gr      = gr;
    t->frist_frame  = ff;

    ff->func   = t->system_routine;
    ff->func0  = (uptr)t->start_routine;
    ff->param0 = 0;
}

static void _task_setup_kstack_user(task_t *t, bool replace) {
    uptr klimit = vmalloc(null, null, TASK_STACK_SIZE_32KB, VMM_FLAGS_DEFAULT);
    uptr kstack = klimit + TASK_STACK_SIZE_32KB;

    kstack -= sizeof(trapframe_t);
    trapframe_t *tf = (trapframe_t *)kstack;
    memset(tf, 0, sizeof(trapframe_t));

    kstack -= sizeof(first_frame_t);
    first_frame_t *ff = (first_frame_t *)kstack;
    memset(ff, 0, sizeof(first_frame_t));

    kstack -= sizeof(task_gr_t);
    task_gr_t *gr = (task_gr_t *)kstack;
    memset(gr, 0, sizeof(task_gr_t));

    if (replace) {
        t->rstack      = (void *)kstack;
        t->rlimit       = (void *)klimit;
        t->rstack_ptr   = (void *)klimit;
    } else {
        t->kstack      = (void *)kstack;
        t->klimit       = (void *)klimit;
        t->kstack_ptr   = (void *)klimit;
    }
    t->tstack          = (void *)kstack;
    t->tlimit          = (void *)klimit;
    t->task_gr         = gr;
    t->frist_frame     = ff;
    t->trap_frame      = tf;

    ff->func   = t->system_routine;
    ff->func0  = 0;
    ff->param0 = 0;

    tf->ss     = USER_DATA_SELECTOR;
    tf->rflags = 0x202;
    tf->cs     = USER_CODE_SELECTOR;
}


task_t *task_create(const char *name, task_routine_t routine,
                    task_priority_t priority, task_mode_t mode) {
    task_t *t = kzalloc(sizeof(task_t));

    t->tgid = t->tid = _alloc_tid();
    t->priority = priority;
    t->mode     = mode;
    t->state    = TASK_READY;

    strncpy(t->name, name, TASK_MAX_NAME_LEN - 1);
    t->name[TASK_MAX_NAME_LEN - 1] = '\0';

    t->mm = mm_create();
    dlist_init(&t->open_files);

    t->wakeup_event = (kevent_t *)kzalloc(sizeof(kevent_t));

    if (mode == TASK_USER_MODE) {
        t->start_routine  = routine;
        t->system_routine = __sched_task_start_user;
        _task_setup_kstack_user(t, false);
    } else {
        t->start_routine  = routine;
        t->system_routine = __sched_task_start;
        _task_setup_kstack(t);
    }

    return t;
}


static void _task_close_all_files(task_t *t) {
    while (!dlist_is_empty(&t->open_files)) {
        linked_list_t *entry = t->open_files.next;
        dlist_remove_entry(entry);

        vfs_file_t *f = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (f->is_console)
            vfs_close_console(f->f_handle);
        else
            vfs_close(f->f_handle);
    }
}

void task_free(task_t *t) {
    task_free_phase0(t);
    if (t->kstack_ptr)
        vfree(null, t->kstack_ptr);
    mm_free(t->mm);
    kfree(t);
}

void task_free_phase0(task_t *t) {
    if (t->mode == TASK_USER_MODE && t->ustack_ptr) {
        vfree(t->mm, t->ustack_ptr);
        t->ustack_ptr = null;
    }

    if (t->u_argv) {
        kfree(t->u_argv);
        t->u_argv = null;
    }

    if (t->u_envp) {
        kfree(t->u_envp);
        t->u_envp = null;
    }

    _task_close_all_files(t);

    if (t->wakeup_event) {
        kfree(t->wakeup_event);
        t->wakeup_event = null;
    }
}


void task_setup_ustack(task_t *t, void *ustack, u32 size) {
    t->ustack_ptr = ustack;
    t->ustack     = (void *)((u8 *)ustack + size);
    t->ulimit     = ustack;
    t->trap_frame->rsp = (uptr)t->ustack;
}

void task_setup_routine(task_t *t, task_routine_t routine) {
    t->start_routine = routine;
    t->trap_frame->rip = (uptr)routine;
}

void task_setup_path(task_t *t, const char *path) {
    size_t len = strlen(path);
    if (len >= VFS_MAX_PATH_LENGTH) len = VFS_MAX_PATH_LENGTH - 1;
    memcpy(t->execve_path, path, len);
    t->execve_path[len] = '\0';
}

void task_setup_argv(task_t *t, int argc, char **argv) {
    if (argc <= 0) return;
    t->u_argc = argc;
    t->u_argv = (char *)kzalloc(argc * TASK_MAX_ARGV_LEN);
    for (int i = 0; i < argc; i++) {
        char *dst = task_argv(t, i);
        size_t len = strlen(argv[i]);
        if (len >= TASK_MAX_ARGV_LEN) len = TASK_MAX_ARGV_LEN - 1;
        memcpy(dst, argv[i], len);
        dst[len] = '\0';
    }
}

void task_setup_cwd(task_t *t, char *cwd) {
    size_t len = strlen(cwd);
    if (len >= VFS_MAX_PATH_LENGTH) len = VFS_MAX_PATH_LENGTH - 1;
    memcpy(t->cwd, cwd, len);
    t->cwd[len] = '\0';
}

void task_setup_env(task_t *t, int envc, char **envp) {
    if (envc <= 0) return;
    t->u_envc = envc;
    t->u_envp = (char *)kzalloc(envc * TASK_MAX_ARGV_LEN);
    for (int i = 0; i < envc; i++) {
        char *dst = task_envp(t, i);
        size_t len = strlen(envp[i]);
        if (len >= TASK_MAX_ARGV_LEN) len = TASK_MAX_ARGV_LEN - 1;
        memcpy(dst, envp[i], len);
        dst[len] = '\0';
    }
}


task_t *task_fork(task_t *p) {
    u64 flags;
    spin_lock_irq(&task_lock, flags);

    if (p->mode == TASK_KERNEL_MODE)
        panic("cannot fork kernel task");

    task_t *c = kzalloc(sizeof(task_t));
    if (!c) {
        spin_unlock_irq(&task_lock, flags);
        return null;
    }
    memcpy(c, p, sizeof(task_t));

    c->tgid = p->tgid;
    c->tid = _alloc_tid();
    c->mm = mm_copy(p->mm);
    if (!c->mm) {
        kfree(c);
        spin_unlock_irq(&task_lock, flags);
        return null;
    }

    dlist_init(&c->open_files);

    if (p->u_argc > 0 && p->u_argv) {
        c->u_argc = p->u_argc;
        c->u_argv = (char *)kzalloc(p->u_argc * TASK_MAX_ARGV_LEN);
        memcpy(c->u_argv, p->u_argv, p->u_argc * TASK_MAX_ARGV_LEN);
    }

    if (p->u_envc > 0 && p->u_envp) {
        c->u_envc = p->u_envc;
        c->u_envp = (char *)kzalloc(p->u_envc * TASK_MAX_ARGV_LEN);
        memcpy(c->u_envp, p->u_envp, p->u_envc * TASK_MAX_ARGV_LEN);
    }

    c->rstack = null;
    c->rlimit = null;
    c->rstack_ptr = null;
    c->old_kstack_ptr = null;
    dlist_init(&c->list);
    dlist_init(&c->child_list);

    c->state = TASK_READY;
    c->wakeup_event = (kevent_t *)kzalloc(sizeof(kevent_t));
    c->is_fork = true;

    _task_setup_kstack_user(c, false);

    uptr p_top = (uptr)p->klimit + TASK_STACK_SIZE_32KB;

    c->trap_frame->rip    = *(u64 *)(p_top - 8);
    c->trap_frame->rflags = *(u64 *)(p_top - 16);
    c->trap_frame->rsp    = (u64)percpu()->ustack;

    c->trap_frame->rcx = *(u64 *)(p_top - 24);
    c->trap_frame->rdx = *(u64 *)(p_top - 32);
    c->trap_frame->rbx = *(u64 *)(p_top - 40);
    c->trap_frame->rbp = *(u64 *)(p_top - 48);
    c->trap_frame->rsi = *(u64 *)(p_top - 56);
    c->trap_frame->rdi = *(u64 *)(p_top - 64);
    c->trap_frame->r8  = *(u64 *)(p_top - 72);
    c->trap_frame->r9  = *(u64 *)(p_top - 80);
    c->trap_frame->r10 = *(u64 *)(p_top - 88);
    c->trap_frame->r11 = *(u64 *)(p_top - 96);
    c->trap_frame->r12 = *(u64 *)(p_top - 104);
    c->trap_frame->r13 = *(u64 *)(p_top - 112);
    c->trap_frame->r14 = *(u64 *)(p_top - 120);
    c->trap_frame->r15 = *(u64 *)(p_top - 128);

    spin_unlock_irq(&task_lock, flags);
    return c;
}


task_t *task_replace(task_t *t, const char *name) {
    u64 flags;
    spin_lock_irq(&task_lock, flags);

    if (t->mode == TASK_KERNEL_MODE)
        panic("cannot replace kernel task");

    if (t->wakeup_event) {
        kfree(t->wakeup_event);
        t->wakeup_event = null;
    }

    if (t->u_argv) {
        kfree(t->u_argv);
        t->u_argv = null;
    }

    if (t->u_envp) {
        kfree(t->u_envp);
        t->u_envp = null;
    }

    t->mm = mm_replace(t->mm);
    t->state = TASK_READY;

    strncpy(t->name, name, TASK_MAX_NAME_LEN - 1);
    t->name[TASK_MAX_NAME_LEN - 1] = '\0';

    t->wakeup_event = (kevent_t *)kzalloc(sizeof(kevent_t));

    _task_close_all_files(t);
    _task_setup_kstack_user(t, true);

    t->is_fork = false;
    spin_unlock_irq(&task_lock, flags);
    return t;
}


void task_init_ustack(task_t *t) {
    int argc = t->u_argc + 1;
    int envc = t->u_envc;
    u8 *sp = (u8 *)t->ustack;

    sp -= argc * TASK_MAX_ARGV_LEN;
    u8 *argv_area = sp;

    sp -= envc * TASK_MAX_ARGV_LEN;
    u8 *envp_area = sp;

    sp -= argc * sizeof(uptr);
    uptr *argv_ptr = (uptr *)sp;

    sp -= (envc + 1) * sizeof(uptr);
    uptr *envp_ptr = (uptr *)sp;
    envp_ptr[envc] = 0;

    sp -= sizeof(uptr);
    uptr *envpp = (uptr *)sp;

    sp -= sizeof(uptr);
    uptr *argvp = (uptr *)sp;

    sp -= sizeof(uptr);
    uptr *argcp = (uptr *)sp;

    memcpy(argv_area, t->execve_path, TASK_MAX_ARGV_LEN);
    argv_ptr[0] = (uptr)argv_area;

    for (int i = 1; i < argc; i++) {
        char *src = task_argv(t, i);
        u8 *dst = argv_area + i * TASK_MAX_ARGV_LEN;
        memcpy(dst, src, TASK_MAX_ARGV_LEN);
        argv_ptr[i] = (uptr)dst;
    }

    for (int i = 0; i < envc; i++) {
        char *src = task_envp(t, i);
        u8 *dst = envp_area + i * TASK_MAX_ARGV_LEN;
        memcpy(dst, src, TASK_MAX_ARGV_LEN);
        envp_ptr[i] = (uptr)dst;
    }

    *envpp = (uptr)envp_ptr;
    *argvp = (uptr)argv_ptr;
    *argcp = (uptr)argc;

    t->ustack = sp;
    t->trap_frame->rsp = (uptr)sp;
}
