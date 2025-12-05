    INTR_START  = 32
    INTR_COUNT  = 224

    #include "./idt_macro.S"

    .global exc_divide_error
    .global exc_debug
    .global exc_nmi
    .global exc_breakpoint
    .global exc_overflow
    .global exc_bounds
    .global exc_undefined_opcode
    .global exc_device_not_available
    .global exc_double_fault
    .global exc_coprocessor_segment_overrun
    .global exc_invalid_tss
    .global exc_segment_not_present
    .global exc_stack_segment_fault
    .global exc_general_protection
    .global exc_page_fault
    .global exc_x87_fpu_err
    .global exc_alignment_check
    .global exc_machine_check
    .global exc_simd_exception
    .global exc_virtualization_exception
    .global exc_control_protection_exception

    .global intr_stubs

    .extern exc_common_handler, intr_common_handler

    .text

exc_divide_error:         // Interrupt 0
    EXCEPTION_HANDLER 0, 0, exc_common_handler

exc_debug:                // Interrupt 1
    EXCEPTION_HANDLER 0, 1, exc_common_handler

exc_nmi:                  // Interrupt 2
    EXCEPTION_HANDLER 0, 2, exc_common_handler

exc_breakpoint:           // Interrupt 3
    EXCEPTION_HANDLER 0, 3, exc_common_handler

exc_overflow:             // Interrupt 4
    EXCEPTION_HANDLER 0, 4, exc_common_handler

exc_bounds:               // Interrupt 5
    EXCEPTION_HANDLER 0, 5, exc_common_handler

exc_undefined_opcode:     // Interrupt 6
    EXCEPTION_HANDLER 0, 6, exc_common_handler

exc_device_not_available: // Interrupt 7
    EXCEPTION_HANDLER 0, 7, exc_common_handler

exc_double_fault:         // Interrupt 8
    EXCEPTION_HANDLER 1, 8, exc_common_handler

exc_coprocessor_segment_overrun: // Interrupt 9
    EXCEPTION_HANDLER 0, 9, exc_common_handler

exc_invalid_tss:          // Interrupt 10
    EXCEPTION_HANDLER 1, 10, exc_common_handler

exc_segment_not_present:  // Interrupt 11
    EXCEPTION_HANDLER 1, 11, exc_common_handler

exc_stack_segment_fault:  // Interrupt 12
    EXCEPTION_HANDLER 1, 12, exc_common_handler

exc_general_protection:   // Interrupt 13
    EXCEPTION_HANDLER 1, 13, exc_common_handler

exc_page_fault:           // Interrupt 14
    EXCEPTION_HANDLER 1, 14, exc_common_handler

// Interrupt 15 is Intel reversed. Do not use.

exc_x87_fpu_err:          // Interrupt 16
    EXCEPTION_HANDLER 0, 16, exc_common_handler

exc_alignment_check:      // Interrupt 17
    EXCEPTION_HANDLER 1, 17, exc_common_handler

exc_machine_check:        // Interrupt 18
    EXCEPTION_HANDLER 0, 18, exc_common_handler

exc_simd_exception:       // Interrupt 19
    EXCEPTION_HANDLER 0, 19, exc_common_handler

exc_virtualization_exception: // Interrupt 20
    EXCEPTION_HANDLER 0, 20, exc_common_handler

exc_control_protection_exception: // Interrupt 21
    EXCEPTION_HANDLER 1, 21, exc_common_handler


// Interrupt 22-31 is Intel reversed. Do not use.


    .macro INTR_HANDLERx irqno
        intr_handler_\irqno:
        INTERRUPT_HANDLER \irqno, intr_common_handler
    .endm

    .macro INTR_STUBx irqno
        intr_stub_\irqno:
        jmp intr_handler_\irqno
    .endm

    .altmacro

    .set __irq_no,INTR_START
    .rept INTR_COUNT
        INTR_HANDLERx %__irq_no
        .set __irq_no,__irq_no+1
    .endr

intr_stubs:
    .set __irq_no,INTR_START
    .rept INTR_COUNT
        INTR_STUBx %__irq_no
        .set __irq_no,__irq_no+1
    .endr

