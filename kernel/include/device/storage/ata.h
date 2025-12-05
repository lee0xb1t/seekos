#ifndef ATA_H
#define ATA_H
#include <lib/ktypes.h>


#define ATA_PRIMARY_IO_BASE             0x1f0
#define ATA_PRIMARY_CTRL_BASE           0x3f6

#define ATA_SECONDARY_IO_BASE           0x170
#define ATA_SECONDARY_CTRL_BASE         0x376

#define ATA_REG_DATA                    0x00
#define ATA_REG_ERROR                   0x01
#define ATA_REG_FEATURES                0x01
#define ATA_REG_SECCOUNT0               0x02
#define ATA_REG_LBA0                    0x03
#define ATA_REG_LBA1                    0x04
#define ATA_REG_LBA2                    0x05
#define ATA_REG_HDDEVSEL                0x06
#define ATA_REG_COMMAND                 0x07
#define ATA_REG_STATUS                  0x07
#define ATA_REG_SECCOUNT1               0x08
#define ATA_REG_LBA3                    0x09
#define ATA_REG_LBA4                    0x0A
#define ATA_REG_LBA5                    0x0B
#define ATA_REG_CONTROL                 0x0C
#define ATA_REG_ALTSTATUS               0x0C
#define ATA_REG_DEVADDRESS              0x0D

#define ATA_STATUS_ERR                  0x01
#define ATA_STATUS_IDX                  0x02
#define ATA_STATUS_CORR                 0x04
#define ATA_STATUS_DRQ                  0x08
#define ATA_STATUS_SRV                  0x10
#define ATA_STATUS_DF                   0x20
#define ATA_STATUS_RDY                  0x40
#define ATA_STATUS_BSY                  0x80

#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_READ_DMA        0xC8
#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_WRITE_DMA       0xCA
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_CACHE_FLUSH     0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_PACKET          0xA0
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY        0xEC


#define ATA_MASTER_DRIVE                0x00
#define ATA_SLAVE_DRIVE                 0x01


typedef struct {
    u16 flags;
    u16 unused1[9];
    char serial[20];
    u16 unused2[3];
    char firmware[8];
    char model[40];
    u16 sectors_per_int;
    u16 unused3;
    u16 capabilities[2];
    u16 unused4[2];
    u16 valid_ext_data;
    u16 unused5[5];
    u16 size_of_rw_mult;
    u32 sectors_28;
    u16 unused6[38];
    u64 sectors_48;
    u16 unused7[152];
} __attribute__ ((packed)) ata_identify_t;

typedef struct _ata_device_t {
    u16 io;
    u16 ctrl;
    u8 slave;
    ata_identify_t identify;
} ata_device_t;


void ata_init();
bool ata_detect(ata_device_t *);
void ata_delay_400ns(ata_device_t *);
void ata_reset_drive(ata_device_t *);
void ata_poll(ata_device_t *, bool datardy);

void ata_pio_read28(ata_device_t *, u8 sector_count, u32 lba, u8 *buff);
void ata_pio_write28(ata_device_t *, u8 sector_count, u32 lba, u8 *buff);

#endif