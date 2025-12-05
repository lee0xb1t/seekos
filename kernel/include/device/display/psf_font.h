#ifndef __psf_hone_h__
#define __psf_hone_h__

#include <lib/ktypes.h>

#define PSF1_FONT_WIDTH 8

#define PSF1_MAGIC0     0x36
#define PSF1_MAGIC1     0x04

#define PSF1_MODE512    0x01
#define PSF1_MODEHASTAB 0x02
#define PSF1_MODEHASSEQ 0x04
#define PSF1_MAXMODE    0x05

#define PSF1_SEPARATOR  0xFFFF
#define PSF1_STARTSEQ   0xFFFE


typedef struct __attribute__((packed)) {
    u8 magic[2];
    u8 mode;
    u8 size;
} psf1_header_t;

typedef struct __attribute__((packed)) {
    psf1_header_t header;
    u8 data[];
} psf1_font_t;

#endif //__psf_hone_h__
