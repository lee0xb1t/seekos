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


static kevent_t *_publish_pop() {
    linked_list_t *entry = null;
    dlist_remove_next(&pub_list, &entry);
    if (!entry) {
        return null;
    }
    return dlist_container_of(entry, kevent_t, list_entry);
}


void kevent_init() {
    dlist_init(&sub_list);
    dlist_init(&pub_list);
    klogi("kevent initialized.\n");
}

kevent_data_t kevent_subscribe(kevent_type_t type) {
    u64 flags;

    kevent_t *event = kzalloc(sizeof(kevent_t));
    if (!event) return EV_DATA_NULL;
    task_id_t tid = sched_get_tid();
    event->publisher = TASK_ID_EMPTY;
    event->subscriber = tid;
    event->type = type;
    event->data = EV_DATA_NULL;
    event->timestamp = hpet_get_nanos();

    spin_lock_irq(&sub_lock, flags);
    dlist_add_prev(&sub_list, &event->list_entry);
    spin_unlock_irq(&sub_lock, flags);

    sched_wait_event(event);

    kevent_data_t data = event->data;
    kfree(event);

    return data;
}

void kevent_publish(kevent_type_t type, kevent_data_t data) {
    u64 flags;

    kevent_t *event = kzalloc(sizeof(kevent_t));
    if (!event) return;
    task_id_t tid = sched_get_tid();
    event->publisher = tid;
    event->subscriber = TASK_ID_EMPTY;
    event->type = type;
    event->data = data;
    event->timestamp = hpet_get_nanos();

    spin_lock_irq(&pub_lock, flags);
    dlist_add_prev(&pub_list, &event->list_entry);
    spin_unlock_irq(&pub_lock, flags);

    kevent_dispatch();
}

void kevent_dispatch() {
    u64 flags_s;
    u64 flags_p;

    spin_lock_irq(&sub_lock, flags_s);
    spin_lock_irq(&pub_lock, flags_p);

    while (pub_list.next != &pub_list) {
        kevent_t *pub_event = _publish_pop();
        if (!pub_event)
            break;

        kevent_t *sub_event = null;

        dlist_foreach(&sub_list, sub_entry) {
            kevent_t *candidate = dlist_container_of(sub_entry, kevent_t, list_entry);
            if (candidate->type == pub_event->type) {
                sub_event = candidate;
                break;
            }
        }

        if (sub_event) {
            if (sched_resume_event(pub_event)) {
                dlist_remove_entry(&sub_event->list_entry);
                kfree(pub_event);
            } else {
                kfree(pub_event);
            }
        } else {
            kfree(pub_event);
        }
    }

    spin_unlock_irq(&pub_lock, flags_p);
    spin_unlock_irq(&sub_lock, flags_s);
}
