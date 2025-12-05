#include <proc/sched.h>
#include <proc/task.h>
#include <proc/elf.h>
#include <ia32/cpuinstr.h>
#include <system/cpu.h>
#include <system/timer.h>
#include <system/apic.h>
#include <system/tsc.h>
#include <log/klog.h>
#include <panic/panic.h>
#include <lib/kmemory.h>
#include <lib/kstring.h>


volatile task_t *idle_tasks[MAX_CPUS] = { 0 };
volatile u64 sched_count[MAX_CPUS] = { 0 };
volatile task_t *running_tasks[MAX_CPUS] = { 0 };

linked_list_t active_tasks;
DECLARE_SPINLOCK(active_tasks_lock);

linked_list_t cleanup_tasks;
DECLARE_SPINLOCK(cleanup_tasks_lock);

DECLARE_SPINLOCK(sched_lock);


extern void __switch_to(void *new_task, void *old_task, uptr cr3, bool is_replace);
extern void __sched_task_start();

/* TODO noreturn macro */
void __idle_proc(void *);
void __sched_kernel_end(i32 exitcode);
void __timer_switch_context();

void _sched_do_execve();


void sched_init() {
    if (percpu()->is_bsp) {
        dlist_init(&active_tasks);
        dlist_init(&cleanup_tasks);
    }

    idle_tasks[percpu()->id] = task_create("idle", __idle_proc, 0, TASK_KERNEL_MODE);

    apic_timer_init();
    apic_timer_set_mode(TIMER_PERIODIC);
    apic_timer_set_period_ms(1);
    if (percpu()->is_bsp) {
        apic_timer_set_handler(__timer_switch_context);
    }
    apic_timer_start();
}


void __idle_proc(void *) {
    for (;;) {
        // klogd("idle\n");
        __hang();
    }
}

void __timer_switch_context(trapframe_t *trapframe) {
    // klog_debug("timer, %p\n", trapframe);
    apic_send_eoi();
    sched_again();
}


void __active_task_fifo_push(task_t *task) {
    u64 flags;
    spin_lock_irq(&active_tasks_lock, flags);
    dlist_add_prev(&active_tasks, &task->list);
    spin_unlock_irq(&active_tasks_lock, flags);
}

task_t *__active_task_fifo_pop() {
    u64 flags;
    linked_list_t *next = null;
    task_t *pop_task = null;

    spin_lock_irq(&active_tasks_lock, flags);
    dlist_remove_next(&active_tasks, &next);
    spin_unlock_irq(&active_tasks_lock, flags);

    if (next)
        pop_task = dlist_container_of(next, task_t, list);
        
    return pop_task;
}

task_t *__active_task_next_available() {
    u64 flags;
    // linked_list_t *next = null;
    task_t *pop_task = null;

    spin_lock_irq(&active_tasks_lock, flags);
    //dlist_remove_next(&active_tasks, &next);
    dlist_foreach(&active_tasks, entry) {
        pop_task = dlist_container_of(entry, task_t, list);
        if (pop_task->state == TASK_READY) {
            break;
        } else if (pop_task->state == TASK_WAITING) {
            if (pop_task->wakeup_event->type == EV_SLEEP &&
                pop_task->wakeup_event->timestamp > 0 && 
                tsc_get_millis() >= pop_task->wakeup_event->timestamp) {
                    
                pop_task->wakeup_event->type = EV_UNKNOWN;
                pop_task->wakeup_event->timestamp = 0;
                break;
            } // else if ...
        }
        pop_task = null;
    }

    if (pop_task) {
        dlist_remove_entry(&pop_task->list);
    }
    spin_unlock_irq(&active_tasks_lock, flags);
        
    return pop_task;
}

void sched_again() {
    u64 flags;
    task_t *curr;
    bool curr_is_running = false;

    cpu_ctrl_t *cpu = percpu();

    kevent_dispatch();

    spin_lock_irq(&sched_lock, flags);
    
    curr = running_tasks[cpu->id];
    if (curr) {
        if (curr != idle_tasks[cpu->id]) {
            if (curr->state != TASK_RUNNING) {
                __active_task_fifo_push(curr);
                curr_is_running = false;
            } else {
                curr_is_running = true;
            }
        }
        // curr->tstack = cpu->tss->rsp0;
    }

    task_t *next = __active_task_next_available();
    if (next) {
        if (curr && curr_is_running) {
            curr->state = TASK_READY;
            __active_task_fifo_push(curr);
        }
    } else {
        next = (curr && curr_is_running) ? curr : idle_tasks[cpu->id];
    }

    next->state = TASK_RUNNING;
    running_tasks[cpu->id] = next;

    void *old_kstack_ptr = null;
    void *old_mm = null;
    if (curr && curr->rstack) {
        old_kstack_ptr = curr->kstack_ptr;

        curr->kstack = curr->rstack;
        curr->klimit = curr->rlimit;
        curr->kstack_ptr = curr->rstack_ptr;
        
        curr->rstack = null;
        curr->rlimit = null;
        curr->rstack_ptr = null;

        curr->old_kstack_ptr = old_kstack_ptr;

        //
        // old_mm = curr->mm;
        // curr->mm = curr->rmm;
    }

    cpu->tss->rsp0 = next->kstack;
    cpu->kstack = next->tstack;

    uptr cr3 = next->mm->page_dir_pa;

    spin_unlock_irq(&sched_lock, flags);

    // __switch_to(next, curr == next ? null : curr, cr3);
    __switch_to(next, curr, cr3, 
        curr && curr->old_kstack_ptr != null
    );
    __barrier();
}

void __sched_kernel_end(i32 exitcode) {
    u64 flags;

    cpu_ctrl_t *cpu = percpu();
    
    spin_lock_irq(&sched_lock, flags);

    task_t *curr_task = running_tasks[cpu->id];

    curr_task->exitcode = exitcode;
    
    if (curr_task != null) {
        if(curr_task->tgid == curr_task->tid) {
            /* destory all child task */
        }

        curr_task->tstack = null;
        curr_task->tlimit = null;

        task_free_phase0(curr_task);
        
        // TODO
        /*task free on wait zombie*/

        curr_task->state = TASK_ZOMBIE;
        __active_task_add_to_cleanup(curr_task);
        running_tasks[cpu->id] = null;
    }

    spin_unlock_irq(&sched_lock, flags);

    sched_again();
}

void __active_task_add_to_cleanup(task_t *task) {
    u64 irq;
    spin_lock_irq(&cleanup_tasks_lock, irq);
    dlist_add_prev(&cleanup_tasks, &task->list);
    spin_unlock_irq(&cleanup_tasks_lock, irq);
}

task_id_t sched_get_tid() {
    task_t *task = running_tasks[percpu()->id];
    return task->tid;
}

task_t *sched_get_task() {
    task_t *task = running_tasks[percpu()->id];
    return task;
}

void sched_wait_event(kevent_t *in_event) {
    u64 flags;
    spin_lock_irq(&sched_lock, flags);

    task_t *curr_task = running_tasks[percpu()->id];
    curr_task->state = TASK_WAITING;
    curr_task->wakeup_event = in_event;
    spin_unlock_irq(&sched_lock, flags);

    sched_again();

    in_event->data = curr_task->wakeup_event->data;
    curr_task->wakeup_event = null;
}

bool sched_resume_event(kevent_t *in_event) {
    bool is_dispatch = false;
    u64 flags;
    u64 flags2;
    spin_lock_irq(&sched_lock, flags2);

    spin_lock_irq(&active_tasks_lock, flags);

    dlist_foreach(&active_tasks, active_entry) {
        task_t *task = dlist_container_of(active_entry, task_t, list);
        
        if (task->state == TASK_WAITING) {
            if (task->wakeup_event) {
                if (task->wakeup_event->type == in_event->type) {
                    task->wakeup_event->data = in_event->data;
                    task->state = TASK_READY;
                    is_dispatch = true;
                } else {
                    // ...
                }
            }
        }
    }
    spin_unlock_irq(&active_tasks_lock, flags);

    spin_unlock_irq(&sched_lock, flags2);

    return is_dispatch;
}

#ifdef _TEST_CASE
bool sched_resume_event_test() {
    u64 flags;
    spin_lock_irq(&active_tasks_lock, flags);
    dlist_foreach(&active_tasks, entry) {
        task_t *task = dlist_container_of(entry, task_t, list);
        if (task->state == TASK_WAITING) {
            task->state = TASK_READY;
        }
    }
    spin_unlock_irq(&active_tasks_lock, flags);
    return false;
}
#endif

void sched_add(task_t *t) {
    u64 flags;
    spin_lock_irq(&sched_lock, flags);
    __active_task_fifo_push(t);
    spin_unlock_irq(&sched_lock, flags);
    
}

void sched_execve(const char *path, int argc, char **argv, char *cwd) {
    u64 flags;
    char *exec_name;

    char temp_path[VFS_MAX_PATH_LENGTH] = {0};
    memcpy(temp_path, path, strlen(path));

    for (int i = 0; i < strlen(temp_path); i++) {
        if (temp_path[i] == '/') {
            exec_name = &temp_path[i+1];
        }
    }

    spin_lock_irq(&sched_lock, flags);

    task_t *now = sched_get_task();
    if (now) {
        task_replace(now, exec_name);
        task_setup_path(now, temp_path);
        task_setup_argv(now, argc, argv);
        // use current cwd
        spin_unlock_irq(&sched_lock, flags);
        sched_again();
    } else {
        task_t *task = task_create(exec_name, null, 255, TASK_USER_MODE);
        task_setup_path(task, path);
        task_setup_argv(task, argc, argv);
        task_setup_cwd(task, cwd);
        spin_unlock_irq(&sched_lock, flags);
        sched_add(task);
    }
}

void _sched_do_execve() {

    task_t *task = sched_get_task();

    if (task->old_kstack_ptr) {
        vfree(null, task->old_kstack_ptr);
        task->old_kstack_ptr = null;

        // mm_free(task->rmm);
        // task->rmm = null;
    }

    vfs_open_console("/dev/ttyfs", VFS_STDIN);
    vfs_open_console("/dev/ttyfs", VFS_STDOUT);
    vfs_open_console("/dev/ttyfs", VFS_STDERR);

    if (task->is_fork) {
        vfs_copy(&task->open_files); //TODO
        void *ustack = vmalloc(task->mm, null, TASK_STACK_SIZE_32KB, VMM_FLAGS_USER);
        task_setup_ustack(task, ustack, TASK_STACK_SIZE_32KB);
        task_setup_routine(task, task->auxv.entry);
    } else {
        elf_load(task, task->execve_path);
    }

    task_init_ustack(task);
}

void sched_exit(i32 exitcode) {
    __sched_kernel_end(exitcode);
}

i32 sched_get_errno() {
    task_t *t = sched_get_task();
    return t->errno;
}

void sched_set_errno(i32 errno) {
    u64 flags;
    task_t *t = sched_get_task();
    spin_lock_irq(&sched_lock, flags);
    t->errno = errno;
    spin_unlock_irq(&sched_lock, flags);
}

void sched_sleep(u64 millis) {
    u64 flags;
    task_t *t = sched_get_task();

    if (!t) {
        tsc_sleep_millis(millis);
        return;
    }

    spin_lock_irq(&sched_lock, flags);
    t->wakeup_event->type = EV_SLEEP;
    t->wakeup_event->timestamp = tsc_get_millis()+millis;
    t->state =TASK_WAITING;
    spin_unlock_irq(&sched_lock, flags);

    sched_again();
}

u32 sched_fork() {
    u64 flags;
    spin_lock_irq(&sched_lock, flags);

    task_t *t = sched_get_task();

    if (t->is_fork) {
        spin_unlock_irq(&sched_lock, flags);
        return 0;
    }

    task_t *n = task_fork(t);

    vfs_copy(&n->open_files);

    spin_unlock_irq(&sched_lock, flags);

    sched_add(n);

    return n->tid;
}

i32 sched_wait(u32 tid) {
    u64 flags;
    bool find = false;
    i32 exitcode = 0;

    task_t *t = sched_get_task();

    while (true) {
        spin_lock_irq(&cleanup_tasks_lock, flags);

        dlist_foreach(&cleanup_tasks, entry) {
            task_t *zombie = dlist_container_of(entry, task_t, list);
                
            if (zombie->state != TASK_ZOMBIE) {
                panic("task %d is not zombie on wait", zombie->tid);
            }

            if (zombie->tgid == t->tgid && zombie->tid == tid) {
                exitcode = zombie->exitcode;
                dlist_remove_entry(entry);
                task_free(zombie);
                find = true;
                break;
            }
        }

        spin_unlock_irq(&cleanup_tasks_lock, flags);

        if (find) break;
        sched_again();
    }

    return exitcode;
}

u32 sched_spawn(const char *path, int argc, char **argv) {
    u64 flags;
    char *exec_name;

    char temp_path[VFS_MAX_PATH_LENGTH] = {0};
    memcpy(temp_path, path, strlen(path));

    for (int i = 0; i < strlen(temp_path); i++) {
        if (temp_path[i] == '/') {
            exec_name = &temp_path[i+1];
        }
    }

    spin_lock_irq(&sched_lock, flags);

    task_t *t = sched_get_task();
    
    task_t *task = task_create(exec_name, null, 255, TASK_USER_MODE);
    task_setup_path(task, path);
    task_setup_argv(task, argc, argv);
    task_setup_cwd(task, t->cwd);
    task->tgid = t->tid;
    spin_unlock_irq(&sched_lock, flags);
    sched_add(task);

    return task->tid;
}
