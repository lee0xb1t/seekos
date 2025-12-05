#ifndef __KEVENT_H
#define __KEVENT_H
#include <lib/ktypes.h>
#include <proc/task.h>


void kevent_init();
kevent_data_t kevent_subscribe(kevent_type_t type);
void kevent_publish(kevent_type_t type, kevent_data_t data);
void kevent_dispatch();

#endif