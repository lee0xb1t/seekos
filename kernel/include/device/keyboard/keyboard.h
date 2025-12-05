#ifndef __KEYBOARD_H
#define __KEYBOARD_H
#include <lib/ktypes.h>


#define KEYBOARD_IRQ        1


#define PS2_PORT_DATA       0x60
#define PS2_PORT_STATUS     0x64
#define PS2_PORT_CMD        0x64

#define PS2_CMD_DISABLE_FIRST_PORT   0xad
#define PS2_CMD_DISABLE_SECOND_PORT  0xa7
#define PS2_CMD_ENABLE_FIRST_PORT    0xae
#define PS2_CMD_ENABLE_SECOND_PORT   0xa8


#define PS2_STATUS_OUTPUT_BUFFER_FULL   0x01
#define PS2_STATUS_INPUT_BUFFER_FULL    0x02


#define KEYBOARD_SCANCODE_KEYPAD        0xe0

#define KEYBOARD_SCANCODE_RESELASE      0x80


typedef struct _keyboard_data_t {
    union {
        struct
        {
            u8 key_state_:5;
            u8 key_state_ctrl:1;
            u8 key_state_shift:1;
            u8 key_state_alt:1;
            
            u8 key_code;
            bool key_pressed;
            bool key_pad;
        };
        u64 flags;
    };
} keyboard_data_t;


void keyboard_init();

#endif