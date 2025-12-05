#include <exception/execdispatch.h>

#include <log/klog.h>
#include <panic/panic.h>

// void common_exception_dispatch(trapframe_t* trap_frame) {

//     // if (trap_frame->trapno == 3) {
//     //     return;
//     // }

//     klog_debug("TrapNo: %d\n", trap_frame->trapno);
//     panic("common_exception_dispatch");
// }
