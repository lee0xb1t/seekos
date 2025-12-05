#include <xlib.h>
#include <stdarg.h>

extern EFI_SYSTEM_TABLE *gST;

/* Ascii Table */
char print_at[2][16] = {
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' },
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' },
};


void clear_screen() {
    gST->ConOut->ClearScreen(gST->ConOut);
}


void console_putc(char ch) {
    char tempc[1] = { ch };
    CHAR16 tempw[2] = { 0 };
    c2w(tempw, tempc, 1);
    gST->ConOut->OutputString(gST->ConOut, tempw);
}

void print_char(char ch) {
    if (ch == '\n') {
        console_putc('\r');
    }
    console_putc(ch);
}

void print_string(const char *str) {
    while(*str)
        print_char(*str++);
}

void print_int32(int32_t number) {
    if (number == 0) {
        print_char('0');
        return;
    }

    int base = 10;

    int32_t temp;
    bool negative = false;
    
    if (number < 0) {
        negative = true;
        temp = (~number) + 1;
    } else {
        temp = number;
    }

    char buffer[32] = { 0 };

    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = print_at[0][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    if (negative) {
        print_char('-');
    }

    print_string(buffer);
}

void print_uint32(uint32_t number) {
    if (number == 0) {
        print_char('0');
        return;
    }

    int base = 10;
    uint32_t temp = number;
    char buffer[32] = { 0 };
    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = print_at[0][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    print_string(buffer);
}

void print_int64(int64_t number) {
    if (number == 0) {
        print_char('0');
        return;
    }

    int base = 10;
    int64_t temp;
    bool negative = false;
    
    if (number < 0) {
        negative = true;
        temp = (~number) + 1;
    } else {
        temp = number;
    }

    char buffer[32] = { 0 };

    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = print_at[0][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    if (negative) {
        print_char('-');
    }

    print_string(buffer);
}

void print_uint64(uint64_t number) {
    if (number == 0) {
        print_char('0');
        return;
    }

    int base = 10;
    uint64_t temp = number;
    char buffer[32] = { 0 };
    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = print_at[0][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    print_string(buffer);
}

void print_hex(uint64_t number, int wordcase) {
    if (number == 0) {
        print_char('0');
        return;
    }

    int base = 16;
    uint64_t temp = number;
    char buffer[32] = { 0 };
    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = print_at[wordcase][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    print_string(buffer);
}

void print_binary(uint64_t number) {
    if (number == 0) {
        print_char('0');
        return;
    }

    int base = 2;
    uint64_t temp = number;
    char buffer[65] = { 0 };
    size_t count = 0;

    while (temp > 0) {
        int number_bit = temp % base;
        buffer[count++] = print_at[0][number_bit];
        temp /= base;
    }

    size_t halfcount = count & ~((size_t)1);
    halfcount /= 2;

    for (size_t i = 0; i < halfcount; i++) {
        int ch = buffer[i];
        buffer[i] = buffer[count - 1 - i];
        buffer[count - 1 - i] = ch;
    }

    print_string(buffer);
}

void print_char16(CHAR16* str) {
    gST->ConOut->OutputString(gST->ConOut, str);
}

void print_vprintf_core(const char *format, va_list args) {
    for (int i = 0; format[i] != '\0'; i++) {
        char ch = format[i];
        switch (ch)
        {
        case '%': {
            char next = format[++i];
            switch (next)
            {
            case '%':
                print_char('%');
                break;
            case 'd':
                print_int32(va_arg(args, i32));
                break;
            case 'q':
                print_int64(va_arg(args, i64));
                break;
            case 'x':
                print_hex(va_arg(args, u64), 0);
                break;
            case 'X':
                print_hex(va_arg(args, u64), 1);
                break;
            case 'p':
                print_string("0x");
                print_hex(va_arg(args, u64), 0);
                break;
            case 'b':
                print_binary(va_arg(args, u64));
                break;
            case 's':
                print_string(va_arg(args, char*));
                break;
            case 'w':
                switch (format[i+1])
                {
                case 's':
                    print_char16(va_arg(args, CHAR16*));
                    ++i;
                    break;
                
                default:
                    print_string(" [undefined keyword] ");
                    break;
                }

                break;

            case 'c':
                print_char(va_arg(args, int));
                break;
            case 'u': {
                switch (format[i+1])
                {
                case 'd':
                    print_uint32(va_arg(args, u32));
                    ++i;
                    break;
                case 'q':
                    print_uint64(va_arg(args, u64));
                    ++i;
                    break;
                
                default:
                    print_uint32(va_arg(args, u32));
                    break;
                }

                break;
            }   // case 'u':
            
            default:
                print_string(" [undefined keyword] ");
                break;
            }
            
            break;
        }   // case '%':
        
        default:
            print_char(ch);
            break;
        }

    }   // for (int i = 0; format[i] != '\0'; i++)

}

void printv(const char *format, ...) {
    va_list args;
    va_start(args, format);
    print_vprintf_core(format, args);
    va_end(args);
}


int gets(char* buffer, int length) {
    int count = 0;

    while(1) {
        EFI_INPUT_KEY key;
        UINTN waitidx = 0;
        gST->BootServices->WaitForEvent(1, &gST->ConIn->WaitForKey, &waitidx);
        gST->ConIn->ReadKeyStroke(gST->ConIn, &key);

        if (count > length - 1) {
            break;
        }

        if (key.UnicodeChar != L'\r') {
            buffer[count++] = key.UnicodeChar & 0xff;
            print_char(key.UnicodeChar & 0xff);
            continue;
        }

        if (key.UnicodeChar == L'\r') {
            print_string("\r\n");
            break;
        }
    }

    return count;
}

// void memzero(CHAR16 *buffer, int length) {
//     while (length)
//         buffer[length--] = 0;
// }

void memzero(void *buffer, int length) {
    gST->BootServices->SetMem((void*)buffer, length, 0);
}

int strlen(const char *buffer) {
    int i = 0;

    if (buffer[0] == 0) {
        return 0;
    }

    while (buffer[i++]);

    return i - 1;
}

int memcmp(const void *mem1, const void *mem2, int len) {
    char *mem1_temp = (char *)mem1;
    char *mem2_temp = (char *)mem2;

    for (int i = 0; i < len; i++) {
        if (mem1_temp[i] != mem2_temp[i]) {
            return mem1_temp[i] - mem2_temp[i];
        }
    }

    return 0;
}

int strcmp(const char *buffer1, const char *buffer2) {
    int len1 = strlen(buffer1);
    int len2 = strlen(buffer2);

    if (len1 != len2) {
        return -1;
    }

    for (int i = 0; i < len1; i++) {
        if (buffer1[i] != buffer2[i]) {
            return -1;
        }
    }

    return 0;
}

void memcpy(void *dest, void *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        *((u8 *)dest + i) = *((u8 *)src + i);
    }
}

EFI_STATUS alloc_pool(
    IN EFI_MEMORY_TYPE            PoolType,
    IN UINTN                      Size,
    OUT VOID                      **Buffer
) {
    return gST->BootServices->AllocatePool(PoolType, Size, Buffer);
}

EFI_STATUS free_pool(
    IN VOID           *Buffer
) {
    return gST->BootServices->FreePool(Buffer);
}

EFI_STATUS alloc_pages(
    IN EFI_ALLOCATE_TYPE            Type,
    IN EFI_MEMORY_TYPE              MemoryType,
    IN UINTN                        Pages,
    IN OUT EFI_PHYSICAL_ADDRESS     *Memory
) {
    return gST->BootServices->AllocatePages(Type, MemoryType, Pages, Memory);
}

EFI_STATUS free_pages(
    IN EFI_PHYSICAL_ADDRESS    Memory,
    IN UINTN                   Pages
) {
    return gST->BootServices->FreePages(Memory, Pages);
}

void c2w(CHAR16* wbuffer, char* buffer, int size) {
    for (int i = 0; i < size; i++) {
        wbuffer[i] = ((CHAR16)buffer[i]);
    }
}

void w2c(char* buffer, CHAR16* wbuffer, int size) {
    for (int i = 0; i < size; i++) {
        buffer[i] = ((CHAR16)wbuffer[i]) >> 8;
    }
}
