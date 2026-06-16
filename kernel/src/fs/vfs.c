#include <fs/vfs.h>
#include <base/spinlock.h>
#include <panic/panic.h>
#include <lib/kstring.h>
#include <lib/kmemory.h>
#include <lib/kmath.h>
#include <lib/errno.h>
#include <log/klog.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <proc/sched.h>


DECLARE_SPINLOCK(vfs_lock);

static vfs_super_block_t *vfs_root;
static linked_list_t vfs_fs_list;
static vfs_handle_t vfs_next_handle_count = VFS_MIN_HANDLE;

extern vfs_fs_t ramfs;
extern vfs_fs_t ttyfs;


///

vfs_fs_t *vfs_get_fs(char *name);


///

void vfs_init() {
    dlist_init(&vfs_fs_list);

    // vfs_register_fs(&ramfs);
    // vfs_register_fs(&ttyfs);

    // vfs_root = vfs_new_inode("root", VFS_NODE_ROOT, null, null);

    // // vfs_resolve_path("/", R_CREATE, VFS_NODE_DIRECTOR);
    // vfs_mount("/", "ramfs");

    // vfs_resolve_path("/dev", R_CREATE, VFS_NODE_DIRECTOR);

    // vfs_mount("/dev/ttyfs", "ttyfs");

    klogi("VFS Initialized.\n");
}

void vfs_register_fs(vfs_fs_t *fs) {
    spin_lock(&vfs_lock);
    dlist_add_prev(&vfs_fs_list, &fs->list_entry);
    spin_unlock(&vfs_lock);
}

vfs_fs_t *vfs_get_fs(char *name) {
    dlist_foreach(&vfs_fs_list, entry) {
        vfs_fs_t *fs = dlist_container_of(entry, vfs_fs_t, list_entry);

        if (strcmp(name, fs->fs_name) == 0) {
            return fs;
        }
    }

    return null;
}

/**
 * iterate find path next name.
 * this function always clear next
 * and change path content.
 * 
 * return: found next
 */
bool vfs_next_path(char *path, char *next, size_t nextlen, bool skiproot) {
    size_t pathlen = strlen(path);
    size_t path_i = 0;
    size_t next_i = 0;
    bool found_slash = false;

    if (pathlen == 0) {
        return false;
    }

    if (skiproot) {
        if (path[0] == '/') {
            path_i = 1;
        }
    }

    memset(next, 0, nextlen);

    for (; path_i < pathlen; path_i++, next_i++) {
        if (path[path_i] == '/') {
            found_slash = true;
            break;
        }

        next[next_i] = path[path_i];
    }

    /*change path is found slash*/
    if (found_slash) {
        memcpy(path, path + path_i + 1, pathlen - path_i - 1);
        memset(path + (pathlen - path_i - 1), 0, pathlen);
    } else {
        memset(path, 0, path_i + 1);
    }

    return strlen(next) > 0;
}


vfs_dentry_t *vfs_resolve_path(const char *path, u8 mode) {
    size_t whole_path_len = 0;
    char whole_path[VFS_MAX_PATH_LENGTH] = {0};

    vfs_dentry_t *current = null;

    char tmppath[VFS_MAX_PATH_LENGTH] = { 0 };
    char nextpath[VFS_MAX_PATH_LENGTH] = { 0 };

    if (!vfs_root) {
        goto _end;
    }

    vfs_dentry_t *parent = vfs_root->s_root;

    if (path[0] == '/' && strlen(path) == 1) {
        return parent;
    }

    if (path[0] != '/') {
        char *cwd = sched_get_task()->cwd;
        memcpy(whole_path, cwd, strlen(cwd));
        if (strcmp(cwd, "/") != 0) {
            memcpy(whole_path + strlen(whole_path), "/", 1);
        }
        memcpy(whole_path + strlen(whole_path), path, strlen(path));
    } else {
        memcpy(whole_path, path, strlen(path));
    }

    whole_path_len = strlen(whole_path);

    memcpy(tmppath, whole_path, whole_path_len);

    for(;;) {
        bool hasnext = vfs_next_path(tmppath, nextpath, VFS_MAX_PATH_LENGTH, true);

        if (!hasnext) {
            break;
        }

        if (strcmp(nextpath, ".") == 0) {
            continue;
        } else if (strcmp(nextpath, "..") == 0) {
            if (current) {
                current = current->d_parent;
                parent = current;
            }
            continue;
        }

        current = null;

        dlist_foreach(&parent->d_subdirs, entry) {
            current = dlist_container_of(entry, vfs_dentry_t, d_list_entry);
            if (strcmp(current->d_name, nextpath) == 0) {
                break;
            }
            current = null;
        }

        if (current == null) {
            current = (vfs_dentry_t *)kzalloc(sizeof(vfs_dentry_t));
            memcpy(current->d_name, nextpath, strlen(nextpath));

            if (!parent->d_inode || !parent->d_inode->i_ops ||
                parent->d_inode->i_ops->lookup(parent->d_inode, current) == null) {
                kloge("[VFS] not found file or dir (%s), component=%s\n", whole_path, nextpath);
                kfree(current);
                current = null;
                goto _end;
            }

            dlist_init(&current->d_list_entry);
            dlist_init(&current->d_subdirs);
            current->d_parent = parent;
            dlist_add_prev(&parent->d_subdirs, &current->d_list_entry);
        }

        if (current->d_inode->i_mountpoint) {
            current = current->d_inode->i_mountpoint->s_root;
        }

        parent = current;
    }

_end:
    return current;
}


i32 vfs_mount_fs(const char *path, const char *fs_name) {
    size_t pathlen = strlen(path);

    spin_lock(&vfs_lock);

    vfs_fs_t *fs = vfs_get_fs(fs_name);

    if (fs == null) {
        panic("[VFS] not found filesystem");
    }

    if (path[0] == '/' && pathlen == 1) {
        vfs_root = fs->mksb(fs);
        spin_unlock(&vfs_lock);
        return 0;
    }

    vfs_dentry_t *dentry = vfs_resolve_path(path, R_NO_CREATE);
    
    if (dentry == null) {
        spin_unlock(&vfs_lock);
        return -1;
    }

    vfs_super_block_t *sb = fs->mksb(fs);
    dentry->d_inode->i_mountpoint = sb;
    sb->s_root->d_parent = dentry;

    spin_unlock(&vfs_lock);
    
    return 0;
}


vfs_handle_t vfs_open(char *path, vfs_openmode_t openmode) {
    
    spin_lock(&vfs_lock);

    vfs_dentry_t *dentry = vfs_resolve_path(path, R_NO_CREATE);
    if (dentry == null) {
        spin_unlock(&vfs_lock);
        return -ENOENT;
    }

    if (!dentry->d_inode) {
        spin_unlock(&vfs_lock);
        panic("[VFS] dentry not found inode");
    }

    if (dentry->d_inode->i_mountpoint) {
        dentry = dentry->d_inode->i_mountpoint->s_root;
    }

    vfs_handle_t fh = ++vfs_next_handle_count;
    
    vfs_file_t *fd = kzalloc(sizeof(vfs_file_t));
    fd->f_handle = fh;
    fd->f_openmode = openmode;
    fd->f_dentry = dentry;
    fd->count = 1;

    fd->f_inode = dentry->d_inode;
    fd->f_ops = dentry->d_inode->i_fops;

    fd->f_ops->open(dentry->d_inode, fd);

    task_t *t = sched_get_task();
    dlist_add_prev(&t->open_files, &fd->open_list_entry);

    spin_unlock(&vfs_lock);

    return fh;
}

i32 vfs_close(vfs_handle_t fh) {
    i32 r = -1;
    spin_lock(&vfs_lock);

    vfs_file_t *fd = null;

    task_t *t = sched_get_task();
    dlist_foreach(&t->open_files, entry) {
        fd = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (fd->f_handle == fh) {
            break;
        }
        fd = null;
    }

    if (!fd) {
        spin_unlock(&vfs_lock);
        return -1;
    }

    if (--fd->count > 0) {
        spin_unlock(&vfs_lock);
        return 0;
    }

    if (fd->f_ops) {
        spin_unlock(&vfs_lock);
        r = fd->f_ops->close(fd->f_inode, fd);
        spin_lock(&vfs_lock);
    }

    dlist_remove_entry(&fd->open_list_entry);
    kfree(fd);

    spin_unlock(&vfs_lock);
    return r;
}

i32 vfs_read(vfs_handle_t fh, i32 len, char* buff) {
    i32 r = 0;

    spin_lock(&vfs_lock);

    vfs_file_t *fd = null;

    task_t *t = sched_get_task();
    dlist_foreach(&t->open_files, entry) {
        fd = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (fd->f_handle == fh) {
            break;
        }
        fd = null;
    }

    if (fd && fd->f_ops) {
        //r = fd->f_ops->read(fd->f_inode, fd, len, buff);
        spin_unlock(&vfs_lock);
        r = fd->f_ops->read(fd->f_inode, fd, len, buff);
        spin_lock(&vfs_lock);
    }

    spin_unlock(&vfs_lock);
    return r;
}

i32 vfs_write(vfs_handle_t fh, i32 len, const char* buff) {
    i32 r = 0;

    spin_lock(&vfs_lock);

    vfs_file_t *fd = null;

    task_t *t = sched_get_task();
    dlist_foreach(&t->open_files, entry) {
        fd = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (fd->f_handle == fh) {
            break;
        }
        fd = null;
    }

    if (fd && fd->f_ops) {
        // r = fd->f_ops->write(fd->f_inode, fd, len, buff);
        spin_unlock(&vfs_lock);
        r = fd->f_ops->write(fd->f_inode, fd, len, buff);
        spin_lock(&vfs_lock);
    }

    spin_unlock(&vfs_lock);
    return r;
}

i32 vfs_lseek(vfs_handle_t fh, i32 offset , i32 wence) {
    i32 r = 0;

    spin_lock(&vfs_lock);

    vfs_file_t *fd = null;

    task_t *t = sched_get_task();
    dlist_foreach(&t->open_files, entry) {
        fd = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (fd->f_handle == fh) {
            break;
        }
        fd = null;
    }

    if (fd && fd->f_ops && fd->f_ops->lseek) {
        spin_unlock(&vfs_lock);
        r = fd->f_ops->lseek(fd->f_inode, fd, offset, wence);
        spin_lock(&vfs_lock);
    }

    spin_unlock(&vfs_lock);
    return r;
}

i32 vfs_get_full_path(vfs_handle_t fh, i32 len, char *buff) {
    i32 r = 0;

    spin_lock(&vfs_lock);

    vfs_file_t *fd = null;

    task_t *t = sched_get_task();
    dlist_foreach(&t->open_files, entry) {
        fd = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (fd->f_handle == fh) {
            break;
        }
        fd = null;
    }

    if (!fd) {
        spin_unlock(&vfs_lock);
        return -1;
    }

    char full_path[VFS_MAX_PATH_LENGTH] = {0};
    char temp_path[VFS_MAX_PATH_LENGTH] = {0};
    vfs_dentry_t *dentry = fd->f_dentry;

    if (dentry == vfs_root->s_root) {
        memcpy(temp_path+1, full_path, strlen(full_path));
        memcpy(temp_path, "/", 1);
        memcpy(full_path, temp_path, strlen(temp_path));
    }
    
    while (dentry != vfs_root->s_root) {
        memcpy(temp_path+strlen(dentry->d_name), full_path, strlen(full_path));
        memcpy(temp_path, dentry->d_name, strlen(dentry->d_name));
        memcpy(full_path, temp_path, strlen(temp_path));
        vfs_dentry_t *parent = dentry->d_parent;
        if (parent == dentry || parent == null)
            break;
        dentry = parent;

        memcpy(temp_path+1, full_path, strlen(full_path));
        memcpy(temp_path, "/", 1);
        memcpy(full_path, temp_path, strlen(temp_path));
    }

    size_t full_len = strlen(full_path);
    if (full_len > 1 && full_path[full_len - 1] == '/') {
        full_path[full_len - 1] = '\0';
    }

    i32 copy_len = min(strlen(full_path), len - 1);
    memcpy(buff, full_path, copy_len);
    buff[copy_len] = '\0';

    spin_unlock(&vfs_lock);
    return r;
}

vfs_inode_type_t vfs_get_inode_type(vfs_handle_t fh) {
    vfs_inode_type_t type = VFS_NODE_INVALID;

    spin_lock(&vfs_lock);

    vfs_file_t *fd = null;

    task_t *t = sched_get_task();
    dlist_foreach(&t->open_files, entry) {
        fd = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (fd->f_handle == fh) {
            break;
        }
        fd = null;
    }

    if (fd && fd->f_inode) {
        type = fd->f_inode->i_type;
    }

    spin_unlock(&vfs_lock);
    return type;
}

i32 vfs_iterate(vfs_handle_t fh, i32 *filecnt, vfs_dirent_t **dirent) {
    i32 r = -1;

    spin_lock(&vfs_lock);

    vfs_file_t *fd = null;

    task_t *t = sched_get_task();
    dlist_foreach(&t->open_files, entry) {
        fd = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (fd->f_handle == fh) {
            break;
        }
        fd = null;
    }

    if (fd && fd->f_ops && fd->f_ops->iterate
        && fd->f_inode && VFS_NODE_IS_DIR(fd->f_inode->i_type)) {
        spin_unlock(&vfs_lock);
        r = fd->f_ops->iterate(fd->f_inode, fd, null, filecnt, dirent);
        spin_lock(&vfs_lock);
    }

    spin_unlock(&vfs_lock);
    return r;
}

vfs_handle_t vfs_open_console(char *path, vfs_handle_t fh) {

    spin_lock(&vfs_lock);

    vfs_dentry_t *dentry = vfs_resolve_path(path, R_NO_CREATE);
    if (dentry == null) {
        spin_unlock(&vfs_lock);
        return -ENOENT;
    }

    if (!dentry->d_inode) {
        panic("[VFS] dentry not found inode");
    }

    if (dentry->d_inode->i_mountpoint) {
        dentry = dentry->d_inode->i_mountpoint->s_root;
    }

    // create description and handle
    
    vfs_file_t *fd = kzalloc(sizeof(vfs_file_t));
    fd->is_console = true;
    fd->f_handle = fh;
    fd->f_openmode = VFS_MODE_READWRITE;
    fd->f_dentry = dentry;
    fd->count = 1;

    fd->f_inode = dentry->d_inode;
    fd->f_ops = dentry->d_inode->i_fops;

    fd->f_ops->open(dentry->d_inode, fd);

    task_t *t = sched_get_task();
    dlist_add_prev(&t->open_files, &fd->open_list_entry);

    spin_unlock(&vfs_lock);

    return fh;
}

vfs_handle_t vfs_close_console(vfs_handle_t fh) {
    i32 r = -1;

    spin_lock(&vfs_lock);

    vfs_file_t *fd = null;

    task_t *t = sched_get_task();
    dlist_foreach(&t->open_files, entry) {
        fd = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (fd->f_handle == fh && fd->is_console) {
            break;
        }
        fd = null;
    }

    if (!fd) {
        spin_unlock(&vfs_lock);
        return -1;
    }

    if (fd->f_ops) {
        r = fd->f_ops->close(fd->f_inode, fd);
    }

    dlist_remove_entry(&fd->open_list_entry);
    kfree(fd);

    spin_unlock(&vfs_lock);
    return r;
}

void vfs_copy(task_t *dest, linked_list_t *src) {

    spin_lock(&vfs_lock);

    dlist_foreach(src, entry) {
        vfs_file_t *filep = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (!filep->is_console) {
            filep->count++;
            vfs_file_t *nf = (vfs_file_t *)kzalloc(sizeof(vfs_file_t));
            *nf = *filep;
            dlist_init(&nf->open_list_entry);
            nf->f_pdata = null;
            dlist_add_prev(&dest->open_files, &nf->open_list_entry);
        }
    }

    spin_unlock(&vfs_lock);
}
