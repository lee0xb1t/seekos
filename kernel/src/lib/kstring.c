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

bool strcmp(char *dest, const char *src) {
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);

    if (dest_len != src_len) {
        return false;
    }

    for (int i = 0;; i++) {
        if (dest[i] != src[i]) {
            return false;
        }

        if (src[i] == '\0')
            break;
    }
    
    return true;
}
