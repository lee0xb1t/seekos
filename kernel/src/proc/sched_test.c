#ifdef _TEST_CASE
#include <proc/sched.h>
#include <proc/task.h>
#include <proc/kevent.h>
#include <log/kprint.h>
#include <log/klog.h>
#include <system/hpet.h>
#include <system/tsc.h>
#include <system/cpu.h>
#include <device/keyboard/keyboard.h>
#include <fs/ttyfs.h>
#include <lib/kstring.h>
#include <base/ksemaphore.h>
#include <base/kmutex.h>


task_t *task1;
task_t *task2;
task_t *task3;


void __test_task1();
void __test_task2();
void __test_task3();

DECLARE_MUTEX(test);
int count = 0;

DECLARE_MUTEX(sem);
int semph = 3;

void sched_test_init() {
    task1 = task_create("vfs_test", __test_task1, 255, TASK_KERNEL_MODE);
    // sched_add(task1);
    task2 = task_create("test2", __test_task2, 255, TASK_KERNEL_MODE);
    // sched_add(task2);
    task3 = task_create("task3", __test_task3, 255, TASK_KERNEL_MODE);
    // sched_add(task3);
}


void __test_task1() {
    // for(int i = 0; i < 1000000; i++) {
    //     kmutex_lock(&test);
    //     count++;
    //     kmutex_unlock(&test);
    // }

    // kmutex_lock(&sem);
    // semph--;
    // kmutex_unlock(&sem);

    // while (true) {
    //     int semph_temp = 0;

    //     kmutex_lock(&sem);
    //     semph_temp = semph;
    //     kmutex_unlock(&sem);

    //     if (semph_temp == 0) {
    //         if (count == 1000000*3) {
    //             kprintf("count is ok, %d\n", count);
    //         } else {
    //             kerrf("count is not 1000*3\n");
    //         }
    //         break;
    //     }
    // }

    for (;;) {
        sched_sleep(1000);
        kprintf("sched skeep: %d\n", percpu()->id);
    }
    

    // kprintf("task1\n");
    // hpet_sleep_millis(1000);
    // for(;;) {
    //     klog_debug("This is task1\n");
    //     hpet_sleep_millis(1000);
    // }


    // kevent_data_t data;
    // for(;;) {
    //     // 修改当前task为waiting
    //     // keyboard_data_t data;
    //     // data.flags = kevent_subscribe(EV_KEYBOARD);
    //     // klogd("recv keyboard: keycode -> %p, %s, %s\n", 
    //     //     data.key_code, 
    //     //     data.key_pressed ? "pressed" : "released", 
    //     //     data.key_pad ? "pad" : "no pad"
    //     // );

    //     char buff[1024];
    //     memset(buff, 0, 1024);
    //     ttyfs_read(null, 10, buff);
    //     //klogi("%s\n", buff);
    //     ttyfs_write(null, 10, buff);
    // }

    kprintf("task1 in cpu%d\n", percpu()->id);

    char buff[4096];
    
    memset(buff, 0, 1024);

    vfs_handle_t fh;

    fh = vfs_open("/rfc/output.txt", VFS_MODE_READWRITE);

    vfs_lseek(fh, 0, SEEK_END);
    char *s = "test";
    vfs_write(fh, strlen(s), s);

    vfs_lseek(fh, -10, SEEK_END);
    vfs_read(fh, 10, buff);

    vfs_close(fh);
    
    kprintf("%s\n", buff);
    
    kprintf("This is kprintf.\n");
    kerrf("This is kerrf.\n");
    

    // for(;;){__hang();}
}

void __test_task2() {
    for(int i = 0; i < 1000000; i++) {
        kmutex_lock(&test);
        count++;
        kmutex_unlock(&test);
    }

    kmutex_lock(&sem);
    semph--;
    kmutex_unlock(&sem);
}

void __test_task3() {
    for(int i = 0; i < 1000000; i++) {
        kmutex_lock(&test);
        count++;
        kmutex_unlock(&test);
    }

    kmutex_lock(&sem);
    semph--;
    kmutex_unlock(&sem);
}

#endif