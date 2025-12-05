#ifndef __KMATH_H
#define __KMATH_H
#include <lib/kutils.h>
#include <lib/ktypes.h>


#define min(a, b) ( (a > b) ? b : a )
#define max(a, b) ( (a < b) ? b : a )

static FORCEINLINE int ffs(int i) {
    for (u32 p = 0; p < sizeof(int) * 8; p++) {
        if (( (i >> p) & 1  ) == 1) {
            return p;
        }
    }
    return sizeof(int) * 8;
}

// static FORCEINLINE int fls(int i) {
//     for (int p = 0; p < sizeof(int) * 8; p++) {
//         if (( (i >> p) & 1  ) == 1) {
//             return p;
//         }
//     }
//     return sizeof(int) * 8;
// }

#endif