#include <device/keyboard/keyboard.h>
#include <device/keyboard/keycode.h>
#include <log/klog.h>
#include <ia32/cpuinstr.h>
#include <system/isr.h>
#include <system/apic.h>
#include <system/madt.h>
#include <mm/mm.h>
#include <proc/kevent.h>


static bool is_extended = false;
static bool capslock_state = false;
static bool scrolllock_state = false;
static bool numlock_state = false;

static bool keystate_lctrl = false;
static bool keystate_rctrl = false;
static bool keystate_lshift = false;
static bool keystate_rshift = false;
static bool keystate_lalt = false;
static bool keystate_ralt = false;


void _process_scan_code(u8 scancode);
void _set_key_state(u8 keycode, bool ispressed);


void keyboard_callback(trapframe_t *trapframe) {
    u8 status = port_inb(PS2_PORT_STATUS);

    if (!(status & PS2_STATUS_OUTPUT_BUFFER_FULL)) {
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

    if (scancode == KEYBOARD_SCANCODE_KEYPAD) {
        is_extended = true;
        return;
    }

    bool is_pressed = (scancode & KEYBOARD_SCANCODE_RELEASE) == 0;
    u8 base_scancode = (scancode & (~KEYBOARD_SCANCODE_RELEASE));

    u16 keycode;
    if (is_extended) {
        keycode = keycode_get_keypad_by_scancode(base_scancode);
        is_extended = false;
    } else {
        keycode = keycode_get_by_scancode(base_scancode);
    }

    if (keycode == KEY_RESERVED) {
        klogw("[KEYBOARD] Unknown scancode(0x%x)\n", scancode);
        return;
    }

    if (is_pressed) {
        switch(keycode) {
            case KEY_CAPSLOCK:
                capslock_state = !capslock_state;
                klogd("[KEYBOARD]  capslock %s\n", capslock_state ? "on" : "off");
                break;
            case KEY_SCROLLLOCK:
                scrolllock_state = !scrolllock_state;
                klogd("[KEYBOARD]  scroll lock %s\n", scrolllock_state ? "on" : "off");
                break;
            case KEY_NUMLOCK:
                numlock_state = !numlock_state;
                klogd("[KEYBOARD]  numlock %s\n", numlock_state ? "on" : "off");
                break;
            default:
                break;
        }
    }

    /* key state */
    _set_key_state(keycode, is_pressed);

    bool ctrl = keystate_lctrl || keystate_rctrl;
    bool shift = keystate_lshift || keystate_rshift;
    bool alt = keystate_lalt || keystate_ralt;

    keyboard_data_t data = {
        .key_state_ctrl = ctrl,
        .key_state_shift = shift,
        .key_state_alt = alt,
        .key_code = keycode,
        .key_pressed = is_pressed,
        .key_pad = false,
    };
    kevent_publish(EV_KEYBOARD, data.flags);
}

void _set_key_state(u8 keycode, bool ispressed) {
    switch (keycode) {
        case KEY_LEFTCTRL:
            keystate_lctrl = ispressed;
            break;

        case KEY_RIGHTCTRL:
            keystate_rctrl = ispressed;
            break;

        case KEY_LEFTSHIFT:
            keystate_lshift = ispressed;
            break;

        case KEY_RIGHTSHIFT:
            keystate_rshift = ispressed;
            break;

        case KEY_LEFTALT:
            keystate_lalt = ispressed;
            break;

        case KEY_RIGHTALT:
            keystate_ralt = ispressed;
            break;

        default:
            break;
    }
}
