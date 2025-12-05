#include <fs/ttyfs.h>
#include <base/spinlock.h>
#include <proc/kevent.h>
#include <panic/panic.h>
#include <log/klog.h>
#include <lib/kmath.h>
#include <lib/kmemory.h>
#include <device/keyboard/keycode.h>
#include <device/keyboard/keyboard.h>
#include <device/display/term.h>
#include <mm/kmalloc.h>


DECLARE_SPINLOCK(tty_lock);
static volatile ttyfs_fdata_t tty_data = {0};


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
    .open = ttyfs_open,
    .close = ttyfs_close,
    .read = ttyfs_read,
    .write = ttyfs_write,
    
};


void ttyfs_init() {
    vfs_register_fs(&ttyfs);
    klogi("TTYFS Initialized.\n");
}

vfs_super_block_t *ttyfs_mksb(vfs_fs_t *fs) {
    // create super block
    vfs_super_block_t *sb = kzalloc(sizeof(vfs_super_block_t));
    sb->s_fs = fs;
    sb->s_ops = null;
    sb->s_pdata = null;

    // create dentry
    vfs_dentry_t *dentry = kzalloc(sizeof(vfs_dentry_t));
    dentry->d_ops = &ttyfs_dentry_ops;
    dentry->d_parent = dentry;
    dentry->d_name[0] = '/';
    dlist_init(&dentry->d_subdirs);
    //
    sb->s_root = dentry;

    vfs_inode_t *inode = kzalloc(sizeof(vfs_inode_t));
    inode->i_fsize = 0;
    inode->i_mountpoint = null;
    inode->i_ops = &ttyfs_inode_ops;
    inode->i_sb = sb;
    inode->i_type = VFS_NODE_DIRECTOR;
    inode->i_pdata = null;
    inode->i_fops = &ttyfs_file_ops;
    //
    dentry->d_inode = inode;

    return sb;
}

i32 ttyfs_open(vfs_inode_t *this, vfs_file_t *filep) {
    // ttyfs_fdata_t *fdata = ttyfs_new_data();
    // filep->f_pdata = fdata;
}

i32 ttyfs_close(vfs_inode_t *this, vfs_file_t *filep) {
    // ttyfs_free_data(filep->f_pdata);
}


i32 ttyfs_read(vfs_inode_t *this, vfs_file_t *filep, i32 len, char *buffer) {
    u64 flags;
    keyboard_data_t kb_data;

    spin_lock_irq(&tty_lock, flags);

    // ttyfs_fdata_t *fdata = (ttyfs_fdata_t *)filep->f_pdata;

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

        switch(ch) {
            case '\b':
            {
                if (tty_data.isize > 0) {
                    tty_data.isize--;
                    tty_data.icursor = (tty_data.icursor - 1) % TTY_BUFFER_SIZE;
                    term_back();
                }
                break;
            }
            case '\n':
            {
                term_newline();
                goto _newline;
                break;
            }
            case '\0':
            {
                break;
            }
            default: {
                tty_data.isize++;
                tty_data.ibuff[tty_data.icursor] = ch;
                tty_data.icursor = (tty_data.icursor + 1) % TTY_BUFFER_SIZE;
                term_putch(term_color(), ch);
                break;
            }
        }

        term_refresh();

        if (tty_data.isize > TTY_BUFFER_SIZE) {
            spin_unlock_irq(&tty_lock, flags);
            panic("[TTYFS] tty buffer is overflow");
        }
    }

_newline:
    /* Output to terminal */
    i32 actual_len = min(len, tty_data.isize);
    memcpy(buffer, tty_data.ibuff + tty_data.ibegin, actual_len);

    tty_data.isize = 0;
    tty_data.ibegin = tty_data.icursor;

    spin_unlock_irq(&tty_lock, flags);

    return 0;
}

i32 ttyfs_write(vfs_inode_t *this, vfs_file_t *filep, i32 len, const char *buffer) {
    u64 flags;
    i64 write_len = 0;

    spin_lock_irq(&tty_lock, flags);

    char temp_buff_min[3] = {0};
    char *temp_buff = len > 1 ? kzalloc(len + 2) : temp_buff_min;

    if (temp_buff) {
        memcpy(temp_buff, buffer, len);

        
        if (filep->is_console && filep->f_handle == VFS_STDERR) {
            kloge(temp_buff);
        } else {
            klogi(temp_buff);
        }

        if (len > 1)
            kfree(temp_buff);

        write_len = (i64)len;
    }

    spin_unlock_irq(&tty_lock, flags);

    return write_len;
}


vfs_dentry_t *ttyfs_lookup(vfs_inode_t *this, vfs_dentry_t *dest_dentry) {
    // linked_list_t *root_dentry = &this->i_sb->s_root->d_subdirs;
    // dlist_foreach(root_dentry, entry) {
    //     vfs_dentry_t* subdentry = dlist_container_of(entry, vfs_dentry_t, d_list_entry);
    //     //subdentry->d_ops->compare
    // }
    return null;
}


ttyfs_fdata_t *ttyfs_new_data() {
    ttyfs_fdata_t *data = kzalloc(sizeof(ttyfs_fdata_t));
    return data;
}

void ttyfs_free_data(ttyfs_fdata_t *d) {
    kfree(d);
}
