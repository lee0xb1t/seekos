#include <libc/string.h>


#define str_to_ltype(type, s, l, ret) \
    do {    \
        type c = 0;     \
        for (int i = 0; i < l; i++) {       \
            if (ctol(s[i]) == -1) {     \
                return c;       \
            }       \
            c = c * 10 + ctol(s[i]);        \
        }       \
        ret = c;    \
    } while (0)

int ctol(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    return -1;
}
i32 strtol(const char *s) {
    i32 c;
    str_to_ltype(i32, s, strlen(s), c);
    return c;
}
i64 strtoll(const char *s) {
    i64 c;
    str_to_ltype(i64, s, strlen(s), c);
    return c;
}
u32 strtoul(const char *s) {
    u32 c;
    str_to_ltype(u32, s, strlen(s), c);
    return c;
}
u64 strtoull(const char *s) {
    u64 c;
    str_to_ltype(u64, s, strlen(s), c);
    return c;
}


size_t strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

char *strcpy(char *dest, const char *src) {
    char *sdest = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return sdest;
}

size_t strlen(char *s) {
    const char *p = s;
    while (*p) p++;
    return p - s;
}

char *strchr(char *str, char c) {
    while (*str) {
        if (*str == c) {
            return str;
        }
        str++;
    }

    if (c == '\0') {
        return str;
    }

    return null;
}

void memset(void* dst, u8 val, size_t size) {
    for (size_t i = 0; i < size; i++) {
        ((u8*)dst)[i] = val;
    }
}

void memcpy(void* dst, const void* src, uptr size) {
    if (size==0)return;
    
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
