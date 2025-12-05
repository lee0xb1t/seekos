#ifndef __KMALLOC_H
#define __KMALLOC_H
#include <lib/ktypes.h>


void *kmalloc(size_t size_of_bytes);
void *kzalloc(size_t size_of_bytes);
void kfree(void *ptr);

void *krealloc(void *, size_t old_size, size_t new_size);

#endif