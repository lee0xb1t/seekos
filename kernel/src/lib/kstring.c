#include <lib/kstring.h>
#include <lib/kmath.h>

size_t strlen(const char *s) {
    size_t n = 0;

    while (s[n] != '\0') {
        n++;
    }

    return n;
}

char *strcpy(char *dest, const char *src) {
    size_t i;

    for (i = 0;; i++) {
        dest[i] = src[i];

        if (src[i] == '\0')
            break;
    }

    return dest + i;
}

char *strncpy(char *dest, const char *src, size_t len) {
    size_t i;

    for (i = 0; i < len; i++) {
        dest[i] = src[i];

        if (src[i] == '\0')
            break;
    }

    return dest + i;
}

int strcmp(const char *str1, const char *str2) {
    while (*str1 && *str1 == *str2) {
        str1++;
        str2++;
    }
    return (unsigned char)*str1 - (unsigned char)*str2;
}
