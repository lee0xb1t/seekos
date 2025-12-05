#include "loader.h"

void halt(void) {
    __asm__ __volatile__ ("hlt;");
    for(;;){}
}