#include <fs/ramfs.h>
#include <base/spinlock.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <lib/kstring.h>
#include <lib/kmath.h>
#include <lib/kmemory.h>
#include <log/klog.h>
#include <panic/panic.h>


#define align_to_512(_)     ( ( ((_)+511)&(~511) ) )

DECLARE_SPINLOCK(ramfs_lock);

static void *rd_buff;
static size_t rd_size;
DECLARE_SPINLOCK(buffer_lock);


vfs_fs_t ramfs = {
    .fs_name = "ramfs",
    .mksb = ramfs_mksb,
};

vfs_dentry_ops_t ramfs_dentry_ops = {
    .compare = null,
    .iput = null,
};

vfs_inode_ops_t ramfs_inode_ops = {
    .lookup = ramfs_lookup,
};

vfs_file_ops_t ramfs_file_ops = {
    .open = ramfs_open,
    .close = ramfs_close,
    .read = ramfs_read,
    .write = ramfs_write,
};


int oct2bin(unsigned char *str, int size) {
    int n = 0;
    unsigned char *c = str;
    while (--size > 0) {
    n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}


void ramfs_init(void *buff, size_t size) {
    rd_buff = buff;
    rd_size = size;
    vfs_register_fs(&ramfs);
    klogi("RAMFS Initialized.\n");
}

vfs_super_block_t *ramfs_mksb(vfs_fs_t *fs) {
    // create super block
    vfs_super_block_t *sb = kzalloc(sizeof(vfs_super_block_t));
    sb->s_fs = fs;
    sb->s_ops = null;
    sb->s_pdata = null;

    // create dentry
    vfs_dentry_t *dentry = kzalloc(sizeof(vfs_dentry_t));
    dentry->d_ops = &ramfs_dentry_ops;
    dentry->d_parent = dentry;
    dentry->d_name[0] = '/';
    dlist_init(&dentry->d_subdirs);
    //
    sb->s_root = dentry;

    vfs_inode_t *inode = kzalloc(sizeof(vfs_inode_t));
    inode->i_fsize = 0;
    inode->i_mountpoint = null;
    inode->i_ops = &ramfs_inode_ops;
    inode->i_sb = sb;
    inode->i_type = VFS_NODE_DIRECTOR;
    
    inode->i_fops = &ramfs_file_ops;
    //
    dentry->d_inode = inode;


    // inode data
    ramfs_ustar_data_t *i_data = (ramfs_ustar_data_t *)kzalloc(sizeof(ramfs_ustar_data_t));
    i_data->filename[0] = '\0';
    i_data->name[0] = '\0';
    i_data->type_flag = U_DIRECTORY;
    inode->i_pdata = i_data;

    return sb;
}

vfs_dentry_t *ramfs_lookup(vfs_inode_t *this, vfs_dentry_t *dest_dentry) {
    spin_lock(&ramfs_lock);

    vfs_inode_t *inode = null;
    ramfs_ustar_data_t *idata = null;

    if (_ustar_lookup(this, dest_dentry->d_name, &idata)) {

        inode = (vfs_inode_t *)kzalloc(sizeof(vfs_inode_t));
        inode->i_sb = this->i_sb;
        inode->i_fsize = idata->size;
        if (idata->type_flag == U_NORMAL) {
            inode->i_type = VFS_NODE_FILE;
        } else if (idata->type_flag == U_DIRECTORY) {
            inode->i_type = VFS_NODE_DIRECTOR;
        }
        inode->i_ops = &ramfs_inode_ops;
        inode->i_fops = &ramfs_file_ops;
        inode->i_pdata = idata;
        
        dest_dentry->d_ops = &ramfs_dentry_ops;
        dest_dentry->d_inode = inode;
    }

    spin_unlock(&ramfs_lock);
    return inode;
}

bool _ustar_lookup(vfs_inode_t *inode, char *name, ramfs_ustar_data_t **out_data) {
    int namelen = strlen(name);

    char current_path[VFS_MAX_PATH_LENGTH] = {0};

    spin_lock(&buffer_lock);

    ramfs_ustar_data_t *parent_data = (ramfs_ustar_data_t *)inode->i_pdata;
    // parent_data->filename
    strcpy(current_path, "./");
    memcpy(current_path, parent_data->filename, strlen(parent_data->filename));
    memcpy(current_path + strlen(current_path), name, namelen);
    // current_path[strlen(current_path)] = '/';


    char u_filename[101] = {0};

    char *buff = (char *)rd_buff;

    int i = 0;
    int size = 0;
    u8 type = 0;

    while(true) {
        if (!memcmp(buff+i+257, "ustar", 5)) {
            memcpy(u_filename, buff+i, 100);
            _ustar_remove_last_slash(u_filename);
            size = oct2bin(buff+i+124, 12);
            type = buff[i+156] - '0';

            switch (type)
            {
            case U_DIRECTORY:
            case U_NORMAL:
                if (strcmp(u_filename, current_path)) {
                    if (type == U_DIRECTORY) {
                        _ustar_add_last_slash(u_filename);
                    }
                    // found
                    ramfs_ustar_data_t *current_data = (ramfs_ustar_data_t *)kzalloc(sizeof(ramfs_ustar_data_t));
                    memcpy(current_data->filename, u_filename, strlen(u_filename));
                    memcpy(current_data->name, name, strlen(name));
                    current_data->data_offset = i+512;
                    current_data->head_offset = i;
                    current_data->size = size;
                    current_data->type_flag = type;

                    *out_data = current_data;
                    goto _endwhile;
                }
            default:
                break;
            }

            size = align_to_512(size);
            i += size + 512;
        } else {
            spin_unlock(&buffer_lock);
            return false;
        }
    }
_endwhile:

    spin_unlock(&buffer_lock);
    return true;
}

void _ustar_remove_last_slash(char *path) {
    size_t pathlen = strlen(path);
    if (pathlen > 0) {
        if (path[pathlen-1] == '/') {
            path[pathlen-1] = '\0';
        }
    }
}

void _ustar_add_last_slash(char *path) {
    size_t pathlen = strlen(path);
    if (pathlen > 0) {
        path[pathlen] = '/';
    }
}


i32 ramfs_open(vfs_inode_t *this, vfs_file_t *file) {

}

i32 ramfs_close(vfs_inode_t *, vfs_file_t *) {

}

i32 ramfs_read(vfs_inode_t *this, vfs_file_t *filep, i32 len, char *buffer) {
    ramfs_ustar_data_t *udata = (ramfs_ustar_data_t *)this->i_pdata;
    int size = len;

    if (filep->f_pos == udata->size) {
        return -1;
    } else if (filep->f_pos + len > udata->size) {
        size = min(len, udata->size - filep->f_pos);
    }

    memcpy(buffer, &((char *)rd_buff)[udata->data_offset+filep->f_pos], size);
    filep->f_pos += size;

    return 0;
}

i32 ramfs_write(vfs_inode_t *, vfs_file_t *, i32 len, const char *buffer) {
    panic("[RAMFS] ramfs connot write");
}


// vfs_inode_t *ramfs_open(vfs_inode_t *this, vfs_node_desc_t *desc, char *path) {
//     spin_lock_irq(&ramfs_lock, flags);

//     ramfs_data_t *data = kzalloc(sizeof(ramfs_data_t));
//     strcpy(data->path, path);
//     data->size += DEFAULT_RAMFS_DATA_SIZE;
//     data->data = kzalloc(data->size);

//     dlist_add_prev(&list_head, &data->list_entry);

//     desc->data = data;
//     spin_unlock_irq(&ramfs_lock, flags);
//     return this;
// }

// i64 ramfs_close(vfs_inode_t *, vfs_node_desc_t *) {
//     return -1;
// }

// i64 ramfs_read(vfs_inode_t *this, vfs_node_desc_t *fd, u64 len, char *buffer) {
//     ramfs_data_t *data = (ramfs_data_t *)fd->data;
//     if (data->size == data->seek) {
//         panic("TODO");
//     }
//     size_t actual_size = min(len, data->size - data->seek);
//     memcpy(buffer, data->data, actual_size);
//     data->seek += actual_size;
//     return actual_size;
// }

// i64 ramfs_write(vfs_inode_t *this, vfs_node_desc_t *fd, u64 len, const char *buffer) {
//     panic("TODO");
//     return -1;
// }

// vfs_inode_t *ramfs_mount(vfs_inode_t *this, const char *path, const char *fs_name) {
//     return this;
// }
