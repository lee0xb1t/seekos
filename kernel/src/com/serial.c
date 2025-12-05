#include <com/serial.h>
#include <ia32/cpuinstr.h>

bool serial_init() {
    port_outb(COM1_BASE + 1, 0x00);    // Disable all interrupts
    port_outb(COM1_BASE + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    port_outb(COM1_BASE + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    port_outb(COM1_BASE + 1, 0x00);    //                  (hi byte)
    port_outb(COM1_BASE + 3, 0x03);    // 8 bits, no parity, one stop bit
    port_outb(COM1_BASE + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    port_outb(COM1_BASE + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    port_outb(COM1_BASE + 4, 0x1E);    // Set in loopback mode, test the serial chip
    port_outb(COM1_BASE + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    port_inb(COM1_BASE + 0);

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    port_outb(COM1_BASE + 4, 0x0F);
    return true;
}


u8 serial_read() {
    for (int i = 0; i < 3; i++) {
        if ((port_inb(COM1_BASE + 5) & 1) != 0) break;
    }
    return port_inb(COM1_BASE);
}


void serial_write(u8 data) {
    for (int i = 0; i < 3; i++) {
        if ((port_inb(COM1_BASE + 5) & 0x20)) break;
    }
    port_outb(COM1_BASE, data);
}
