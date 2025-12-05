#ifndef __SCHED_H
#define __SCHED_H
#include <proc/task.h>
#include <ia32/cpuinstr.h>


#define sched_lock_timer        __cli

#define sched_unlock_timer      __sti
    

void sched_init();
void __active_task_fifo_push(task_t *);
task_t *__active_task_fifo_pop();
task_t *__active_task_next_available();
void __active_task_add_to_cleanup(task_t *);

void sched_again();

// void sched_add_to_cleanup(task_t *);

task_id_t sched_get_tid();
task_t *sched_get_task();

void sched_wait_event(kevent_t *in_event);
bool sched_resume_event(kevent_t *in_event);
bool sched_resume_event_test();

#ifdef _TEST_CASE
void sched_test_init();
#endif

void sched_add(task_t *);
void sched_execve(const char *path, int argc, char **argv, char *cwd);
void sched_exit(i32 exitcode);
i32 sched_get_errno();
void sched_set_errno(i32);

void sched_sleep(u64 millis);
u32 sched_fork();
i32 sched_wait(u32);
u32 sched_spawn(const char *path, int argc, char **argv);

#endif