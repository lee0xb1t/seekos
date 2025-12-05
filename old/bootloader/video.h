#ifndef __video_h__
#define __video_h__

#include <types.h>


typedef struct {
	char signature[4];	// must be "VESA" to indicate valid VBE support
	u16 version;			// VBE version; high byte is major version, low byte is minor version
	u32 oem;			// segment:offset pointer to OEM
	u32 capabilities;		// bitfield that describes card capabilities
	// u32 video_modes;		// segment:offset pointer to list of supported video modes
	u16 video_modes_offset; // segment:offset pointer to list of supported video modes
    u16 video_modes_segment;
	u16 video_memory;		// amount of video memory in 64KB blocks
	u16 software_rev;		// software revision
	u32 vendor;			// segment:offset to card vendor string
	u32 product_name;		// segment:offset to card model name
	u32 product_rev;		// segment:offset pointer to product revision
	char reserved[222];		// reserved for future expansion
	char oem_data[256];		// OEM BIOSes store their strings in this area
} __attribute__ ((packed)) vesa_vbe_info_t;

typedef struct {
	u16 attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	u8 window_a;			// deprecated
	u8 window_b;			// deprecated
	u16 granularity;		// deprecated; used while calculating bank numbers
	u16 window_size;
	u16 segment_a;
	u16 segment_b;
	u32 win_func_ptr;		// deprecated; used to switch banks from protected mode without returning to real mode
	u16 pitch;			// number of bytes per horizontal line
	u16 width;			// width in pixels
	u16 height;			// height in pixels
	u8 w_char;			// unused...
	u8 y_char;			// ...
	u8 planes;
	u8 bpp;			// bits per pixel in this mode
	u8 banks;			// deprecated; total number of banks in this mode
	u8 memory_model;
	u8 bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	u8 image_pages;
	u8 reserved0;
 
	u8 red_mask;
	u8 red_position;
	u8 green_mask;
	u8 green_position;
	u8 blue_mask;
	u8 blue_position;
	u8 reserved_mask;
	u8 reserved_position;
	u8 direct_color_attributes;
 
	u32 framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	u32 off_screen_mem_off;
	u16 off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
	u8 reserved1[206];
} __attribute__ ((packed)) vesa_vbe_mode_info_t;

typedef struct {
	union {
		u16 flag;

		struct {
			u16 mode : 14;
			u16 lfb : 1;
			u16 dm : 1;
		};
	};
} __attribute__ ((packed)) vesa_mode_t;



//void vga_fetch_font();


//
// FUNCTION: Get VESA BIOS information
// Function code: 0x4F00
// Description: Returns the VESA BIOS information, including manufacturer, supported modes, available video memory, etc... Input: AX = 0x4F00
// Input: ES:DI = Segment:Offset pointer to where to store VESA BIOS information structure.
// Output: AX = 0x004F on success, other values indicate that VESA BIOS is not supported.
//
bool vesa_get_vesa_bios_information(vesa_vbe_info_t* vbe_info);


//
// FUNCTION: Get VESA mode information
// Function code: 0x4F01
// Description: This function returns the mode information structure for a specified mode. The mode number should be gotten from the supported modes array.
// Input: AX = 0x4F01
// Input: CX = VESA mode number from the video modes array
// Input: ES:DI = Segment:Offset pointer of where to store the VESA Mode Information Structure shown below.
// Output: AX = 0x004F on success, other values indicate a BIOS error or a mode-not-supported error.
//
bool vesa_mode_information(u16 mode, vesa_vbe_mode_info_t* vbe_mode_info);


//
//FUNCTION: Set VBE mode
// Function code: 0x4F02
// Description: This function sets a VBE mode.
// Input: AX = 0x4F02
// Input: BX = Bits 0-13 mode number; bit 14 is the LFB bit: when set, it enables the linear framebuffer, when clear, software must use bank switching. Bit 15 is the DM bit: when set, the BIOS doesn't clear the screen. Bit 15 is usually ignored and should always be cleared.
// Output: AX = 0x004F on success, other values indicate errors; such as BIOS error, too little video memory, unsupported VBE mode, mode doesn't support linear frame buffer, or any other error.
//
bool vesa_set_vbe_mode(u16 mode);


//
// FUNCTION: Get current VBE mode
// Function code: 0x4F03
// Description: This function returns the current VBE mode.
// Input: AX = 0x4F03
// Output: AX = 0x004F on success, other values indicate errors; maybe the system is not in a VBE mode?
// Output: BX = Bits 0-13 mode number; bit 14 is the LFB bit: when set, the system is using the linear framebuffer, when clear, the system is using bank switching. Bit 15 is the DM bit: when clear, video memory was cleared when the VBE mode was set.
//
bool vesa_get_current_mode(vesa_mode_t* current_mode);


bool video_find_best_vesa_mdoe(u16 *mode);


void video_init();

vesa_vbe_mode_info_t* video_get_current_mode_info();


#endif //__video_h__