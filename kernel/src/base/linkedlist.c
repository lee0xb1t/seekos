#include <base/linkedlist.h>
#include <lib/ktypes.h>


void dlist_remove_entry(linked_list_t *entry) {
    linked_list_t *prev = entry->prev;
    linked_list_t *next = entry->next;

    prev->next = next;
    next->prev = prev;

    entry->prev = entry;
    entry->next = entry;
}

void dlist_add_prev(linked_list_t *list, linked_list_t *entry) {
    entry->next = list;
    entry->prev = list->prev;

    entry->prev->next = entry;
    entry->next->prev = entry;
}

void dlist_add_next(linked_list_t *list, linked_list_t *entry) {
    entry->prev = list;
    entry->next = list->next;

    entry->next->prev = entry;
    entry->prev->next = entry;
}


void dlist_remove_prev(linked_list_t *list, linked_list_t **entry) {
    if (entry == null)
        return;

    *entry = null;

    linked_list_t* temp_entry = list->prev;

    if (temp_entry == list) {
        return;
    }

    list->prev = temp_entry->prev;
    temp_entry->prev->next = list;

    temp_entry->next = temp_entry;
    temp_entry->prev = temp_entry;

    *entry = temp_entry;
}

void dlist_remove_next(linked_list_t *list, linked_list_t **entry) {
    if (entry == null) {
        return;
    }

    *entry = null;

    linked_list_t* temp_entry = list->next;

    if (temp_entry == list) {
        return;
    }

    list->next = temp_entry->next;
    temp_entry->next->prev = list;

    temp_entry->next = temp_entry;
    temp_entry->prev = temp_entry;

    *entry = temp_entry;
}

void dlist_get_prev(linked_list_t *list, linked_list_t **entry) {
    if (entry == null) {
        return;
    }

    *entry = null;

    linked_list_t* temp_entry = list->prev;

    if (temp_entry == list) {
        return;
    }

    *entry = temp_entry;
}

void dlist_get_next(linked_list_t *list, linked_list_t **entry) {
    if (entry == null) {
        return;
    }

    *entry = null;

    linked_list_t* temp_entry = list->next;

    if (temp_entry == list) {
        return;
    }

    *entry = temp_entry;
}
