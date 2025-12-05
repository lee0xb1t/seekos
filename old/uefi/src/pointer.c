#include <pointer.h>
#include <xlib.h>

extern EFI_SYSTEM_TABLE *gST;

EFI_SIMPLE_POINTER_PROTOCOL *spp;

EFI_GUID spp_guid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;

void pointer_init() {
    gST->BootServices->LocateProtocol(&spp_guid, NULL, (void**)&spp);
}

void pstat() {
    UINTN waitidx = 0;
    EFI_SIMPLE_POINTER_STATE state;

    spp->Reset(spp, TRUE);
    
    while (1) {
        printv("--------------------------------------\r\n");
        gST->BootServices->WaitForEvent(1, &spp->WaitForInput, &waitidx);
        printv(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
        if (EFI_ERROR(spp->GetState(spp, &state))) {
            print_int32(state.RelativeMovementX);
            print_string("  ");
            print_int32(state.RelativeMovementY);
            print_string("  ");
            print_int32(state.RelativeMovementZ);
            print_string("  ");
            print_int32(state.LeftButton);
            print_string("  ");
            print_int32(state.RightButton);
            print_string("\r\n");
        }
    }
}
