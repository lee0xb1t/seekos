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
    .lseek = ramfs_lseek,
    .iterate = ramfs_iterate,
};


static u32 oct2bin(const char *str, int size) {
    u32 n = 0;
    while (size-- > 0) {
        if (*str < '0' || *str > '7')
            break;
        n = n * 8 + (*str - '0');
        str++;
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
    vfs_super_block_t *sb = kzalloc(sizeof(vfs_super_block_t));
    sb->s_fs = fs;
    sb->s_ops = null;
    sb->s_pdata = null;

    vfs_dentry_t *dentry = kzalloc(sizeof(vfs_dentry_t));
    dentry->d_ops = &ramfs_dentry_ops;
    dentry->d_parent = dentry;
    dentry->d_name[0] = '\0';
    dlist_init(&dentry->d_subdirs);

    sb->s_root = dentry;

    vfs_inode_t *inode = kzalloc(sizeof(vfs_inode_t));
    inode->i_fsize = 0;
    inode->i_mountpoint = null;
    inode->i_ops = &ramfs_inode_ops;
    inode->i_sb = sb;
    inode->i_type = VFS_NODE_DIRECTOR;

    inode->i_fops = &ramfs_file_ops;

    dentry->d_inode = inode;

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
    return dest_dentry;
}

static void _ustar_strip_prefix(const char *path, char *out, size_t outlen) {
    const char *src = path;
    if (src[0] == '.' && src[1] == '/') {
        src += 2;
    }
    size_t i = 0;
    while (*src && i < outlen - 1) {
        out[i++] = *src++;
    }
    out[i] = '\0';
}

/**
 * read a single USTAR block header at buff + offset.
 * writes stripped filename and size/type to out params.
 * returns true if the block is a valid ustar entry.
 *
 * pre:  offset + 512 <= buff_end  (caller must ensure this)
 */
static bool _ustar_read_block_header(
    const char *buff, size_t offset,
    char *stripped_out, size_t stripped_sz,
    int *size_out, u8 *type_out
) {
    if (buff[offset + 257] == 'u' && !memcmp(buff + offset + 257, "ustar", 5)) {
        char u_filename[101];
        memcpy(u_filename, buff + offset, 100);
        u_filename[100] = '\0';

        _ustar_strip_prefix(u_filename, stripped_out, stripped_sz);
        _ustar_remove_last_slash(stripped_out);

        *size_out = (int)oct2bin(buff + offset + 124, 12);
        *type_out = (u8)(buff[offset + 156] - '0');
        return true;
    }
    return false;
}

bool _ustar_lookup(vfs_inode_t *inode, char *name, ramfs_ustar_data_t **out_data) {
    int namelen = strlen(name);

    ramfs_ustar_data_t *parent_data = (ramfs_ustar_data_t *)inode->i_pdata;

    char search_path[VFS_MAX_PATH_LENGTH] = {0};
    if (parent_data->filename[0] == '\0') {
        memcpy(search_path, name, namelen);
    } else {
        int parent_len = strlen(parent_data->filename);
        memcpy(search_path, parent_data->filename, parent_len);
        memcpy(search_path + parent_len, "/", 1);
        memcpy(search_path + parent_len + 1, name, namelen);
    }

    spin_lock(&buffer_lock);

    char *buff = (char *)rd_buff;
    size_t buff_end = rd_size;
    if (!buff || buff_end < 512) {
        spin_unlock(&buffer_lock);
        return false;
    }

    char stripped[VFS_MAX_PATH_LENGTH];

    size_t i = 0;
    while (i + 512 <= buff_end) {
        int size = 0;
        u8 type = 0;

        if (!_ustar_read_block_header(buff, i, stripped, VFS_MAX_PATH_LENGTH, &size, &type)) {
            spin_unlock(&buffer_lock);
            return false;
        }

        if (stripped[0] == '\0') {
            i += 512 + align_to_512(size);
            continue;
        }

        if (type == U_DIRECTORY || type == U_NORMAL) {
            if (strcmp(stripped, search_path) == 0) {
                ramfs_ustar_data_t *current_data = (ramfs_ustar_data_t *)kzalloc(sizeof(ramfs_ustar_data_t));
                memcpy(current_data->filename, stripped, strlen(stripped));
                memcpy(current_data->name, name, namelen);
                current_data->data_offset = i + 512;
                current_data->head_offset = i;
                current_data->size = size;
                current_data->type_flag = type;

                *out_data = current_data;
                spin_unlock(&buffer_lock);
                return true;
            }
        }

        i += 512 + align_to_512(size);
    }

    spin_unlock(&buffer_lock);
    return false;
}

void _ustar_remove_last_slash(char *path) {
    size_t pathlen = strlen(path);
    if (pathlen > 0) {
        if (path[pathlen - 1] == '/') {
            path[pathlen - 1] = '\0';
        }
    }
}


i32 ramfs_open(vfs_inode_t *this, vfs_file_t *file) {
    ((void)this);
    ((void)file);
    return 0;
}

i32 ramfs_close(vfs_inode_t *, vfs_file_t *) {
    return 0;
}

i32 ramfs_read(vfs_inode_t *this, vfs_file_t *filep, i32 len, char *buffer) {
    if (len == 0) return 0;

    ramfs_ustar_data_t *udata = (ramfs_ustar_data_t *)this->i_pdata;
    if (filep->f_pos >= (size_t)udata->size) {
        return -1;
    }

    int size = len;
    if (filep->f_pos + len > (size_t)udata->size) {
        size = udata->size - filep->f_pos;
    }

    memcpy(buffer, &((char *)rd_buff)[udata->data_offset + filep->f_pos], size);
    filep->f_pos += size;

    return 0;
}

i32 ramfs_write(vfs_inode_t *this, vfs_file_t *filep, i32 len, const char *buffer) {
    ((void)this);
    ((void)filep);
    ((void)len);
    ((void)buffer);
    return -1;
}

i32 ramfs_lseek(vfs_inode_t *this, vfs_file_t *filep, i32 offset, i32 wence) {
    ramfs_ustar_data_t *udata = (ramfs_ustar_data_t *)this->i_pdata;

    switch (wence) {
    case SEEK_SET:
        if (offset < 0) return -1;
        filep->f_pos = (size_t)offset;
        break;
    case SEEK_CUR:
        if (offset < 0 && (size_t)(-offset) > filep->f_pos) return -1;
        filep->f_pos += offset;
        break;
    case SEEK_END:
        if (offset > 0) return -1;
        filep->f_pos = udata->size + offset;
        break;
    default:
        return -1;
    }

    if (filep->f_pos > (size_t)udata->size) {
        filep->f_pos = udata->size;
    }

    return (i32)filep->f_pos;
}

static bool _ustar_is_direct_child(const char *entry, const char *parent) {
    size_t parent_len = strlen(parent);

    if (parent_len == 0) {
        for (const char *p = entry; *p; p++) {
            if (*p == '/') return false;
        }
        return entry[0] != '\0';
    }

    if (memcmp((void *)entry, (void *)parent, parent_len) != 0) {
        return false;
    }
    if (entry[parent_len] != '/')
        return false;

    const char *remainder = entry + parent_len + 1;
    if (remainder[0] == '\0')
        return false;
    for (const char *p = remainder; *p; p++) {
        if (*p == '/') return false;
    }
    return true;
}

i32 ramfs_iterate(vfs_inode_t *this, vfs_file_t *filep, char *path, i32 *filecnt, vfs_dirent_t **dirent) {
    ((void)filep);
    ((void)path);
    ramfs_ustar_data_t *idata = (ramfs_ustar_data_t *)this->i_pdata;
    const char *parent_path = idata->filename;

    spin_lock(&buffer_lock);

    char *buff = (char *)rd_buff;
    size_t buff_end = rd_size;
    if (!buff || buff_end < 512) {
        spin_unlock(&buffer_lock);
        *dirent = null;
        *filecnt = 0;
        return 0;
    }

    vfs_dirent_t *entries = null;
    int count = 0;

    char stripped[VFS_MAX_PATH_LENGTH];
    size_t i = 0;

    while (i + 512 <= buff_end) {
        int size = 0;
        u8 type = 0;

        if (!_ustar_read_block_header(buff, i, stripped, VFS_MAX_PATH_LENGTH, &size, &type))
            break;

        if (stripped[0] == '\0') {
            i += 512 + align_to_512(size);
            continue;
        }

        if (type == U_DIRECTORY || type == U_NORMAL) {
            if (_ustar_is_direct_child(stripped, parent_path)) {
                const char *child_name;
                size_t parent_len = strlen(parent_path);
                if (parent_len == 0) {
                    child_name = stripped;
                } else {
                    child_name = stripped + parent_len + 1;
                }

                entries = krealloc(entries,
                    count * sizeof(vfs_dirent_t),
                    (count + 1) * sizeof(vfs_dirent_t));
                memcpy(entries[count].name, child_name, strlen(child_name));
                entries[count].name[strlen(child_name)] = '\0';
                entries[count].sz = (u32)size;
                entries[count].type = (type == U_DIRECTORY) ? VFS_NODE_DIRECTOR : VFS_NODE_FILE;
                count++;
            }
        }

        i += 512 + align_to_512(size);
    }

    spin_unlock(&buffer_lock);

    *dirent = entries;
    *filecnt = count;
    return count;
}
