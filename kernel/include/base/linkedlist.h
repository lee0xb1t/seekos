#ifndef __xlinkedlist_h__
#define __xlinkedlist_h__
#include <lib/ktypes.h>


typedef struct _linked_list_t{
    struct _linked_list_t* prev;
    struct _linked_list_t* next;
} linked_list_t;

#define dlist_decl(name)     \
    linked_list_t name = {      \
        .next = &name,          \
        .prev = &name,          \
    }

#define dlist_decl_static(name)     \
    static dlist_decl(name)


#define dlist_new(name) \
    do {                \
        name = (linked_list_t *)kzalloc(sizeof(linked_list_t));     \
        dlist_init(name);   \
    } while(0)


#define dlist_init(list) \
    ((struct _linked_list_t*)list)->prev = list; \
    ((struct _linked_list_t*)list)->next = list;


#define dlist_container_of(entry, type, field) \
    ((void *)((uptr)(entry)) - ((uptr)(&((type*)0)->field)))

#define dlist_foreach(list, entry) \
    for (linked_list_t *entry = (list)->next; entry != (list); entry = entry->next)

#define dlist_foreach_prev(list, entry) \
    for (linked_list_t *entry = (list)->prev; entry != (list); entry = entry->prev)



void dlist_remove_entry(linked_list_t *entry);

void dlist_add_prev(linked_list_t *list, linked_list_t *entry);
void dlist_add_next(linked_list_t *list, linked_list_t *entry);

void dlist_remove_prev(linked_list_t *list, linked_list_t **entry);
void dlist_remove_next(linked_list_t *list, linked_list_t **entry);

void dlist_get_prev(linked_list_t *list, linked_list_t **entry);
void dlist_get_next(linked_list_t *list, linked_list_t **entry);

static inline bool dlist_is_empty(linked_list_t *list) {
    return list == list->next && list == list->prev;
}


#endif //__xlinkedlist_h__