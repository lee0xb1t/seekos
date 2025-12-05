#include <proc/kevent.h>
#include <proc/sched.h>
#include <base/spinlock.h>
#include <system/hpet.h>
#include <log/klog.h>
#include <mm/kmalloc.h>


static linked_list_t sub_list;
DECLARE_SPINLOCK(sub_lock);

static linked_list_t pub_list;
DECLARE_SPINLOCK(pub_lock);


kevent_t *_subscribe_pop();
kevent_t *_publish_pop();


void kevent_init() {
    dlist_init(&sub_list);
    dlist_init(&pub_list);
    klogi("kevent initialized.\n");
}

kevent_data_t kevent_subscribe(kevent_type_t type) {
    u64 flags;
    kevent_t *event = kzalloc(sizeof(kevent_t));
    event->publisher = sched_get_tid();
    event->subscriber = TASK_ID_EMPTY;
    event->type = type;
    event->data = EV_DATA_NULL;
    event->timestamp = hpet_get_nanos();

    spin_lock_irq(&sub_lock, flags);
    dlist_add_prev(&sub_list, &event->list_entry);
    spin_unlock_irq(&sub_lock, flags);

    // wait event
    sched_wait_event(event);
    
    // get event data
    kevent_data_t data = event->data;
    kfree(event);

    return data;
}

void kevent_publish(kevent_type_t type, kevent_data_t data) {
    u64 flags;
    kevent_t *event = kzalloc(sizeof(kevent_t));
    event->publisher = sched_get_tid();
    event->subscriber = TASK_ID_EMPTY;
    event->type = type;
    event->data = data;
    event->timestamp = hpet_get_nanos();

    spin_lock_irq(&pub_lock, flags);
    dlist_add_prev(&pub_list, &event->list_entry);
    spin_unlock_irq(&pub_lock, flags);

    // klogd("[KEVENT] publish a event(%d), publisher = %d, data = %p, timestamp = %uq\n", 
    //     event->type,
    //     event->publisher,
    //     event->data,
    //     event->timestamp
    // );
}

void kevent_dispatch() {
    u64 flags;
    u64 flags1;
    spin_lock_irq(&sub_lock, flags);
    spin_lock_irq(&pub_lock, flags1);

    kevent_t *pub_event = _publish_pop();
    if (!pub_event) {
        goto _end;
    }

    kevent_t *sub_event = null;

    dlist_foreach(&sub_list, sub_entry) {
        sub_event = dlist_container_of(sub_entry, kevent_t, list_entry);
        if (sub_event) {
            if (sub_event->type == pub_event->type) {
                break;
            }
        }
        sub_event = null;
    }

    if (sub_event) {
        if (sched_resume_event(pub_event)) {
            kfree(pub_event);
            dlist_remove_entry(&sub_event->list_entry);
        }
    }

_end:
    spin_unlock_irq(&pub_lock, flags1);
    spin_unlock_irq(&sub_lock, flags);
}

kevent_t *_subscribe_pop() {
    linked_list_t *entry;
    dlist_remove_next(&sub_list, &entry);
    return dlist_container_of(entry, kevent_t, list_entry);
}

kevent_t *_publish_pop() {
    linked_list_t *entry = null;
    dlist_remove_next(&pub_list, &entry);
    if (!entry) {
        return null;
    }
    return dlist_container_of(entry, kevent_t, list_entry);
}
