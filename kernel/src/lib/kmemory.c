#include <lib/kmemory.h>

void memset(void* dst, u8 val, uptr size) {
    for (uptr i = 0; i < size; i++) {
        ((u8*)dst)[i] = val;
    }
}

int memcmp(void* m1, void* m2, uptr size) {
    u8 *m1_temp = (u8 *)m1;
    u8 *m2_temp = (u8 *)m2;

    int sum = 0;
    
    for (uptr i = 0; i < size; i++) {
        if (m1_temp[i] - m2_temp[i] != 0) {
            return m1_temp[i] - m2_temp[i];
        }
    }
    return 0;
}

void memcpy(void* dst, const void* src, uptr size) {
    size_t range = dst - src;
    if (range <= 8) {
        goto byte_copy;
    }

    if (size >= 32) {
        uptr *dst_temp = (uptr *)dst;
        uptr *src_temp = (uptr *)src;
        uptr temp_size =  size / sizeof(uptr);
        uptr remain_size = size % sizeof(uptr);

        for (uptr i = 0; i < temp_size; i++) {
            dst_temp[i] = src_temp[i];
        }

        if (remain_size > 0) {
            u8 *dst_remain = (u8 *)dst + temp_size * sizeof(uptr);
            u8 *src_remain = (u8 *)src + temp_size * sizeof(uptr);

            for (uptr i = 0; i < remain_size; i++) {
                dst_remain[i] = src_remain[i];
            }
        }
        return;
    } else {
        goto byte_copy;
    }

byte_copy:
    u8 *dst_temp = (u8 *)dst;
    u8 *src_temp = (u8 *)src;

    for (uptr i = 0; i < size; i++) {
        dst_temp[i] = src_temp[i];
    }
}
