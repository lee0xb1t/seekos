#ifndef __LIST_H
#define __LIST_H
#include <lib/ktypes.h>
#include <lib/kmemory.h>


#define LIST_RESIZE_FACTOR      2

#define list_def(type) \
    struct _list_t {        \
        size_t len;        \
        size_t capacity;    \
        type *data;          \
    }

#define list_decl(type, name)   \
    list_def(type) name={0}

#define list_extern(type, name)     \
    extern list_def(type) name

#define list_new(type, name)    \
    do {    \
        name = (list_def(type) *)kzalloc(sizeof(list_def(type))); \
    } while(0)


#define list_push(l, elem)    \
    do {    \
        (l)->len++;    \
        if ((l)->capacity < (l)->len * sizeof(elem)) {    \
            (l)->capacity = (l)->len * sizeof(elem) * LIST_RESIZE_FACTOR;   \
            (l)->data = krealloc((l)->data, (l)->len-1, (l)->capacity); \
        }   \
        (l)->data[(l)->len - 1] = elem; \
    } while(0)


#define list_length(l)  \
    (l)->len


#define list_at(l, i)   \
    (l)->data[i]


#define list_erase(l, i)    \
    do {    \
        if ((l)->len > 1) { \
            memcpy( (l)->data, (l)->data + i + 1, ((l)->len - i - 1) * sizeof((l)->data[0]) );  \
            memset( (l)->data + ((l)->len - 1), 0, sizeof((l)->data[0]) );  \
            (l)->size--;    \
            if ((l)->len == 0) {   \
                kfree((l)->data);   \
                (l)->data = null;   \
                (l)->capacity = 0;  \
            }   \
        }   \
    } while(0)

#define list_clear(l)   \
    do {    \
        kfree((l)->data);   \
        (l)->len = 0;       \
        (l)->capacity = 0;  \
    } while(0)


// #define list_foreach() \
//     do {

//     } while(0)


#endif