#include <fs/ttyfs.h>
#include <base/spinlock.h>
#include <proc/kevent.h>
#include <panic/panic.h>
#include <log/klog.h>
#include <lib/kcolor.h>
#include <lib/kmath.h>
#include <lib/kmemory.h>
#include <device/keyboard/keycode.h>
#include <device/keyboard/keyboard.h>
#include <device/display/term.h>
#include <mm/kmalloc.h>


DECLARE_SPINLOCK(tty_lock);
static ttyfs_fdata_t tty_buf;


vfs_fs_t ttyfs = {
    .fs_name = TTY_FS_NAME,
    .mksb = ttyfs_mksb,
};

vfs_dentry_ops_t ttyfs_dentry_ops = {
    .compare = null,
    .iput = null,
};

vfs_inode_ops_t ttyfs_inode_ops = {
    .lookup = ttyfs_lookup,
};

vfs_file_ops_t ttyfs_file_ops = {
    .open    = ttyfs_open,
    .close   = ttyfs_close,
    .read    = ttyfs_read,
    .write   = ttyfs_write,
};


void ttyfs_init(void) {
    vfs_register_fs(&ttyfs);
    klogi("TTYFS Initialized.\n");
}

vfs_super_block_t *ttyfs_mksb(vfs_fs_t *fs) {
    vfs_super_block_t *sb = kzalloc(sizeof(vfs_super_block_t));
    sb->s_fs = fs;
    sb->s_ops = null;
    sb->s_pdata = null;

    vfs_dentry_t *dentry = kzalloc(sizeof(vfs_dentry_t));
    dentry->d_ops = &ttyfs_dentry_ops;
    dentry->d_parent = dentry;
    dentry->d_name[0] = '\0';
    dlist_init(&dentry->d_subdirs);
    sb->s_root = dentry;

    vfs_inode_t *inode = kzalloc(sizeof(vfs_inode_t));
    inode->i_fsize = 0;
    inode->i_mountpoint = null;
    inode->i_ops = &ttyfs_inode_ops;
    inode->i_sb = sb;
    inode->i_type = VFS_NODE_CHARACTER;
    inode->i_pdata = null;
    inode->i_fops = &ttyfs_file_ops;
    dentry->d_inode = inode;

    return sb;
}


i32 ttyfs_open(vfs_inode_t *this, vfs_file_t *filep) {
    ((void)this); ((void)filep);
    return 0;
}

i32 ttyfs_close(vfs_inode_t *this, vfs_file_t *filep) {
    ((void)this); ((void)filep);
    return 0;
}


static void _tty_echo_char(char ch) {
    switch (ch) {
    case '\b':
        if (tty_buf.isize > 0) {
            tty_buf.isize--;
            tty_buf.icursor = (tty_buf.icursor - 1 + TTY_BUFFER_SIZE) % TTY_BUFFER_SIZE;
            term_back();
        }
        break;
    case '\n':
        term_newline();
        break;
    case '\0':
        break;
    default:
        if (tty_buf.isize >= TTY_BUFFER_SIZE) {
            panic("[TTYFS] tty buffer overflow");
        }
        tty_buf.ibuff[tty_buf.icursor] = ch;
        tty_buf.icursor = (tty_buf.icursor + 1) % TTY_BUFFER_SIZE;
        tty_buf.isize++;
        term_putch(term_color(), ch);
        break;
    }
    term_refresh();
}


i32 ttyfs_read(vfs_inode_t *this, vfs_file_t *filep, i32 len, char *buffer) {
    ((void)this); ((void)filep);

    u64 flags;
    keyboard_data_t kb_data;

    spin_lock_irq(&tty_lock, flags);

    if (len > TTY_BUFFER_SIZE) {
        spin_unlock_irq(&tty_lock, flags);
        return -1;
    }

    while (true) {
        spin_unlock_irq(&tty_lock, flags);

        kb_data.flags = kevent_subscribe(EV_KEYBOARD);
        char ch = keyecode_to_ascii(kb_data.key_code, kb_data.key_state_shift);

        spin_lock_irq(&tty_lock, flags);

        if (!kb_data.key_pressed)
            continue;

        if (ch == '\0')
            continue;

        _tty_echo_char(ch);

        if (ch == '\n')
            goto _done;
    }

_done:
    i32 actual_len = (i32)min((u32)len, (u32)tty_buf.isize);

    u64 avail_to_end = TTY_BUFFER_SIZE - tty_buf.ibegin;
    if ((u64)actual_len <= avail_to_end) {
        memcpy(buffer, tty_buf.ibuff + tty_buf.ibegin, actual_len);
    } else {
        memcpy(buffer, tty_buf.ibuff + tty_buf.ibegin, avail_to_end);
        memcpy(buffer + avail_to_end, tty_buf.ibuff, (u64)actual_len - avail_to_end);
    }

    tty_buf.isize = 0;
    tty_buf.ibegin = tty_buf.icursor;

    spin_unlock_irq(&tty_lock, flags);
    return actual_len;
}


i32 ttyfs_write(vfs_inode_t *this, vfs_file_t *filep, i32 len, const char *buffer) {
    ((void)this);
    if (!buffer || len <= 0) return 0;

    u64 flags;
    spin_lock_irq(&tty_lock, flags);

    if (filep->is_console && filep->f_handle == VFS_STDERR) {
        term_color_push(COLOR_RED);
        term_write_n((const char *)buffer, (u32)len);
        term_color_pop();
    } else {
        term_write_n((const char *)buffer, (u32)len);
    }

    term_flush();
    spin_unlock_irq(&tty_lock, flags);
    return len;
}


vfs_dentry_t *ttyfs_lookup(vfs_inode_t *this, vfs_dentry_t *dest_dentry) {
    ((void)this); ((void)dest_dentry);
    return null;
}
