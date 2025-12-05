#ifndef __fs_h__
#define __fs_h__

#include <efi.h>

void fs_init();

EFI_STATUS fs_open_file(IN CHAR16 *FileName, OUT EFI_FILE **OutFile);
EFI_STATUS fs_close_file(IN EFI_FILE *OutFile);

#endif //__fs_h__