#include <fs.h>
#include <xlib.h>

extern EFI_SYSTEM_TABLE *gST;
extern EFI_HANDLE gImageHandle;

EFI_GUID SimpleFileSystemGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;

EFI_GUID elip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_LOADED_IMAGE_PROTOCOL* elip_proto;

void fs_init() {
    EFI_STATUS status;

    status = gST->BootServices->HandleProtocol(gImageHandle, &elip_guid, (void**)&elip_proto);

    if (EFI_ERROR(status)) {
        printv("Failed to get loaded image!\r\n");
    }

    status = gST->BootServices->HandleProtocol(elip_proto->DeviceHandle, &SimpleFileSystemGuid, (void**)&SimpleFileSystem);

    if (EFI_ERROR(status)) {
        printv("Failed to locate simple filesystem!\r\n");
    }
    
}

EFI_STATUS fs_open_file(IN CHAR16 *FileName, OUT EFI_FILE **OutFile) {
    EFI_STATUS status;
    EFI_FILE *FileProtocol;
    EFI_FILE *LoadedFile;

    status = SimpleFileSystem->OpenVolume(SimpleFileSystem, &FileProtocol);
    if (EFI_ERROR(status)) {
        printv("Failed to OpenVolume!\r\n");
        return status;
    }

    status = FileProtocol->Open(FileProtocol, &LoadedFile, FileName, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if (EFI_ERROR(status)) {
        printv("Failed to open file!\r\n");
        return status;
    }

    *OutFile = LoadedFile;

    return EFI_SUCCESS;
}


EFI_STATUS fs_close_file(IN EFI_FILE *OutFile) {
    EFI_STATUS status;
    EFI_FILE *VolumeFile;

    status = SimpleFileSystem->OpenVolume(SimpleFileSystem, &VolumeFile);
    if (EFI_ERROR(status)) {
        return status;
    }

    VolumeFile->Close(OutFile);

     if (EFI_ERROR(status)) {
        return status;
    }

    return status;
}
