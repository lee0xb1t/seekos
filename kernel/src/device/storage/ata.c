#include <device/storage/ata.h>
#include <ia32/cpuinstr.h>
#include <panic/panic.h>
#include <base/spinlock.h>
#include <log/klog.h>


static volatile char current_drive_char = 'a';
static volatile ata_device_t *current_drive = null;

static volatile ata_device_t ata_primary_master = {
    .io = ATA_PRIMARY_IO_BASE, .ctrl = ATA_PRIMARY_CTRL_BASE,
    .slave = ATA_MASTER_DRIVE,
};

static volatile ata_device_t ata_primary_slave = {
    .io = ATA_PRIMARY_IO_BASE, .ctrl = ATA_PRIMARY_CTRL_BASE,
    .slave = ATA_SLAVE_DRIVE,
};

static volatile ata_device_t ata_secondary_master = {
    .io = ATA_SECONDARY_IO_BASE, .ctrl = ATA_SECONDARY_IO_BASE,
    .slave = ATA_MASTER_DRIVE,
};

static volatile ata_device_t ata_secondary_slave = {
    .io = ATA_SECONDARY_IO_BASE, .ctrl = ATA_SECONDARY_IO_BASE,
    .slave = ATA_SLAVE_DRIVE,
};

DECLARE_SPINLOCK(ata_lock);


void ata_init() {
    if (ata_detect(&ata_primary_master)) {
        current_drive = 'a';
        current_drive = &ata_primary_master;
    } else if (ata_detect(&ata_primary_slave)) {
        current_drive = 'b';
        current_drive = &ata_primary_slave;
    } else if (ata_detect(&ata_secondary_master)) {
        current_drive = 'c';
        current_drive = &ata_secondary_master;
    } else if (ata_detect(&ata_secondary_slave)) {
        current_drive = 'd';
        current_drive = &ata_secondary_slave;
    } else {
        panic("Not found ata device");
    }

    klogi("ATA Initialized.\n");
}

bool ata_detect(ata_device_t *dev) {
    // ata_reset_drive(dev);
    ata_delay_400ns(dev);

    port_inb(dev->io + ATA_REG_STATUS);
    port_outb(dev->io + ATA_REG_ALTSTATUS, 0);
    ata_delay_400ns(dev);

    /* select */
    port_inb(dev->io + ATA_REG_STATUS);
    port_outb(dev->io + ATA_REG_HDDEVSEL, 0xa0 | (dev->slave << 4));
    ata_delay_400ns(dev);

    /* init data */
    port_outb(dev->io + ATA_REG_SECCOUNT0, 0);
    port_outb(dev->io + ATA_REG_LBA0, 0);
    port_outb(dev->io + ATA_REG_LBA1, 0);
    port_outb(dev->io + ATA_REG_LBA2, 0);

    /* IDENTIFY */
    port_outb(dev->io + ATA_REG_COMMAND, 0xec);

    ata_delay_400ns(dev);

    u8 status = port_inb(dev->io + ATA_REG_STATUS);
    if (status == 0) {
        /* primary master not exists */
        return false;
    }

    ata_delay_400ns(dev);

    int timer = I32_MAX;
    while (--timer) {
        status = port_inb(dev->io + ATA_REG_STATUS);
        if (status & ATA_STATUS_ERR) {
            return false;
        }
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRQ)) {
            goto _s1;
        }
    }
    return false;

_s1:
    ata_delay_400ns(dev);

    if (port_inb(dev->io + ATA_REG_LBA1) || port_inb(dev->io + ATA_REG_LBA2)) {
        /* Not ata */
        return false;
    }

    ata_delay_400ns(dev);

    timer = I32_MAX;
    while (--timer) {
        status = port_inb(dev->io + ATA_REG_STATUS);
        if (status & ATA_STATUS_ERR) {
            return false;
        }
        if (status & ATA_STATUS_DRQ) {
            goto _s2;
        }
    }
    return false;

_s2:
    ata_delay_400ns(dev);

    /*read 256 bytes*/
    for (int i = 0; i < 256; i++) {
        u8 *buff = &dev->identify;
        buff[i] = port_inb(dev->io + ATA_REG_DATA);
    }

    return true;
}

void ata_delay_400ns(ata_device_t *dev) {
    for(int i = 0; i < 4; i++)
      port_inb(dev->io + ATA_REG_ALTSTATUS);
}

void ata_reset_drive(ata_device_t *dev) {
    port_outb(dev->ctrl, 0x04);
    port_outb(dev->ctrl, 0x00);
}

void ata_poll(ata_device_t *dev, bool datardy) {
    u8 status = port_inb(dev->io + ATA_REG_STATUS);

    while (status & ATA_STATUS_BSY) {
        status = port_inb(dev->io + ATA_REG_STATUS);
    }

    if (datardy) {
        int ignore = 5;
        while (status = port_inb(dev->io + ATA_REG_STATUS)) {
            if (!(--ignore)) {
                if (status & ATA_STATUS_ERR || status & ATA_STATUS_DF) {
                    panic("[ATA] poll error, status: %p\n", status);
                }
            }

            if (status & ATA_STATUS_DRQ) {
                break;
            }
        }
    }
}

void ata_pio_read28(ata_device_t *dev, u8 sector_count, u32 lba, u8 *buff) {
    if (dev == null) {
        dev = current_drive;
    }

    ata_delay_400ns(dev);

    port_outb(dev->io + ATA_REG_HDDEVSEL, 0xE0 | (dev->slave << 4) | ((lba >> 24) & 0x0F));
    ata_delay_400ns(dev);

    port_outb(dev->io + ATA_REG_FEATURES, 0x00);

    port_outb(dev->io + ATA_REG_SECCOUNT0, sector_count);
    port_outb(dev->io + ATA_REG_LBA0, (u8)lba);
    port_outb(dev->io + ATA_REG_LBA1, (u8)(lba >> 8));
    port_outb(dev->io + ATA_REG_LBA2, (u8)(lba >> 16));
    port_outb(dev->io + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    ata_delay_400ns(dev);

    for (int i = 0; i < sector_count; i++) {
        ata_poll(dev, true);

        port_insw(dev->io + ATA_REG_DATA, buff, 256);
        buff += 512;

        ata_delay_400ns(dev);
    }

    ata_poll(dev, false);
}

void ata_pio_write28(ata_device_t *dev, u8 sector_count, u32 lba, u8 *buff) {
    if (!dev) {
        dev = current_drive;
    }

    ata_delay_400ns(dev);

    port_outb(dev->io + ATA_REG_HDDEVSEL, 0xE0 | (dev->slave << 4) | ((lba >> 24) & 0x0F));
    ata_delay_400ns(dev);

    port_outb(dev->io + ATA_REG_FEATURES, 0x00);

    port_outb(dev->io + ATA_REG_SECCOUNT0, sector_count);
    port_outb(dev->io + ATA_REG_LBA0, (u8)lba);
    port_outb(dev->io + ATA_REG_LBA1, (u8)(lba >> 8));
    port_outb(dev->io + ATA_REG_LBA2, (u8)(lba >> 16));
    port_outb(dev->io + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    ata_delay_400ns(dev);

    for (int i = 0; i < sector_count; i++) {
        ata_poll(dev, true);

        u16 *obuff = (u16 *)buff;
        for (int j = 0; j < 256; j++) {
            port_outw(dev->io + ATA_REG_DATA, obuff[j]);
        }
        buff += 512;

        ata_delay_400ns(dev);
    }

    ata_poll(dev, false);
    port_outb(dev->io + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    ata_poll(dev, false);
}
