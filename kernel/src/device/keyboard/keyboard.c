#include <device/keyboard/keyboard.h>
#include <device/keyboard/keycode.h>
#include <log/klog.h>
#include <ia32/cpuinstr.h>
#include <system/isr.h>
#include <system/apic.h>
#include <system/madt.h>
#include <mm/mm.h>
#include <proc/kevent.h>


static bool is_keypad = false;
static bool capslock_state = false;
static bool scrolllock_state = false;
static bool numlock_state = false;

static bool keystate_ctrl = false;
static bool keystate_shift = false;
static bool keystate_alt = false;


void _process_scan_code(u8 scancode);
void _set_key_state(u8 keycode, bool ispressed);


void keyboard_callback(trapframe_t *trapframe) {
    u8 status = port_inb(PS2_PORT_STATUS);

    if (!(status & PS2_STATUS_OUTPUT_BUFFER_FULL)) {
        // No data
        goto _end;
    }

    u8 scancode = port_inb(PS2_PORT_DATA);
    _process_scan_code(scancode);

_end:
    apic_send_eoi();
}

void keyboard_init() {

    /* Disable PS/2 devices */
    port_outb(PS2_PORT_CMD, PS2_CMD_DISABLE_FIRST_PORT);
    port_outb(PS2_PORT_CMD, PS2_CMD_DISABLE_SECOND_PORT);

    /* Flush the output buffer */
    while (port_inb(PS2_PORT_STATUS) & PS2_STATUS_OUTPUT_BUFFER_FULL) {
        port_inb(PS2_PORT_DATA);
    }

    /* First register irq and then enable */

    isr_register_irq_h(KEYBOARD_IRQ, keyboard_callback);

    /* Enable PS/2 devices */
    port_outb(PS2_PORT_CMD, PS2_CMD_ENABLE_FIRST_PORT);
    port_outb(PS2_PORT_CMD, PS2_CMD_ENABLE_SECOND_PORT);


    klogi("keyboard initialized.\n");
}

void _process_scan_code(u8 scancode) {
    u16 keycode;

    if (scancode == KEYBOARD_SCANCODE_KEYPAD) {
        is_keypad = true;
    }

    bool is_pressed = (scancode & KEYBOARD_SCANCODE_RESELASE) == 0;
    u8 base_scancode = (scancode & (~KEYBOARD_SCANCODE_RESELASE));

    if (is_keypad) {
        // TODO
        is_keypad = false;
    } else {
        keycode = keycode_get_by_scancode(base_scancode);
    }

    if (keycode == KEY_RESERVED) {
        klogw("[KEYBOARD] Unknown scancode(0x%x)\n", scancode);
        return;
    }

    if (!is_pressed) {
        bool update_leds = false;

        switch(keycode) {
            case KEY_CAPSLOCK:
                capslock_state = !capslock_state;
                update_leds = true;
                klogd("[KEYBOARD]  capslock %s\n", capslock_state ? "on" : "off");
                break;
            case KEY_SCROLLLOCK:
                scrolllock_state = !scrolllock_state;
                update_leds = true;
                klogd("[KEYBOARD]  scroll lock %s\n", scrolllock_state ? "on" : "off");
                break;
            case KEY_NUMLOCK:
                numlock_state = !numlock_state;
                update_leds = true;
                klogd("[KEYBOARD]  numlock %s\n", numlock_state ? "on" : "off");
                break;
            default:
                break;
        }

        if (update_leds) {
            // TODO
        }
    }

    /* key state */
    _set_key_state(keycode, is_pressed);

    // send event
    keyboard_data_t data = {
        .key_state_ctrl = keystate_ctrl,
        .key_state_shift = keystate_shift,
        .key_state_alt = keystate_alt,
        .key_code = keycode,
        .key_pressed = is_pressed,
        .key_pad = is_keypad
    };
    kevent_publish(EV_KEYBOARD, data.flags);
}

void _set_key_state(u8 keycode, bool ispressed) {
    switch (keycode) {
        case KEY_LEFTCTRL: {
            if (ispressed) {
                keystate_ctrl = true;
                return;
            } else {
                keystate_ctrl = false;
            }
            break;
        }

        case KEY_LEFTSHIFT:
        case KEY_RIGHTSHIFT:
        {
            if (ispressed) {
                keystate_shift = true;
                return;
            } else {
                keystate_shift = false;
            }
            break;
        }
        
        case KEY_LEFTALT: {
            if (ispressed) {
                keystate_alt = true;
                return;
            } else {
                keystate_alt = false;
            }
            break;
        }

        default:
            break;
    }
}
