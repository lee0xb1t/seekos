#include <fs/vfs.h>
#include <base/spinlock.h>
#include <panic/panic.h>
#include <lib/kstring.h>
#include <lib/kmemory.h>
#include <lib/kmath.h>
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
    dlist_add_prev(&vfs_fs_list, &fs->list_entry);
}

vfs_fs_t *vfs_get_fs(char *name) {
    dlist_foreach(&vfs_fs_list, entry) {
        vfs_fs_t *fs = dlist_container_of(entry, vfs_fs_t, list_entry);

        if (strcmp(name, fs->fs_name)) {
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
    size_t nextpathlen = 0;

    vfs_dentry_t *parent = vfs_root->s_root;

    if (!vfs_root) {
        goto _end;
    }

    if (path[0] == '/' && strlen(path) == 1) {
        return parent;
    }

    if (path[0] != '/') {
        char *cwd = sched_get_task()->cwd;

        if (strcmp(cwd, "/")) {
            whole_path[0] = '/';
            whole_path[1] = '\0';
        }

        memcpy(whole_path, cwd, strlen(cwd));
        memcpy(whole_path + strlen(whole_path), "/", 1);
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

        if (strcmp(nextpath, ".")) {
            continue;
        } else if (strcmp(nextpath, "..")) {
            current = current->d_parent;
            parent = current;
            continue;
        }

        current = null;

        dlist_foreach(&parent->d_subdirs, entry) {
            current = dlist_container_of(entry, vfs_dentry_t, d_list_entry);
            if (strcmp(current->d_name, nextpath)) {
                break;
            }
            current = null;
        }

        if (current == null) {
            current = (vfs_dentry_t *)kzalloc(sizeof(vfs_dentry_t));
            memcpy(current->d_name, nextpath, strlen(nextpath));

            if (parent->d_inode->i_ops->lookup(parent->d_inode, current) == null) {
                // not found file or dir
                kloge("[VFS] not found file or dir (%s)\n", whole_path);
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
    u64 flags;
    size_t pathlen = strlen(path);

    spin_lock_irq(&vfs_lock, flags);

    vfs_fs_t *fs = vfs_get_fs(fs_name);

    if (fs == null) {
        panic("[VFS] not found filesystem");
    }

    if (path[0] == '/' && pathlen == 1) {
        vfs_root = fs->mksb(fs);
        spin_unlock_irq(&vfs_lock, flags);
        return 0;
    }

    vfs_dentry_t *dentry = vfs_resolve_path(path, R_NO_CREATE);
    
    if (dentry == null) {
        return -1;
    }

    dentry->d_inode->i_mountpoint = fs->mksb(fs);

    spin_unlock_irq(&vfs_lock, flags);
    
    return 0;
}


vfs_handle_t vfs_open(char *path, vfs_openmode_t openmode) {
    u64 flags;
    
    spin_lock_irq(&vfs_lock, flags);

    vfs_dentry_t *dentry = vfs_resolve_path(path, R_NO_CREATE);
    if (dentry == null) {
        spin_unlock_irq(&vfs_lock, flags);
        return VFS_INVALID_HANDLE;
    }

    if (!dentry->d_inode) {
        spin_unlock_irq(&vfs_lock, flags);
        panic("[VFS] dentry not found inode");
    }

    if (dentry->d_inode->i_mountpoint) {
        dentry = dentry->d_inode->i_mountpoint->s_root;
    }

    // create description and handle
    vfs_handle_t fh = ++vfs_next_handle_count;
    
    vfs_file_t *fd = kzalloc(sizeof(vfs_file_t));
    fd->f_handle = fh;
    fd->f_openmode = openmode;
    fd->f_dentry = dentry;

    fd->f_inode = dentry->d_inode;
    fd->f_ops = dentry->d_inode->i_fops;

    fd->f_ops->open(dentry->d_inode, fd);

    task_t *t = sched_get_task();
    dlist_add_prev(&t->open_files, &fd->open_list_entry);

    spin_unlock_irq(&vfs_lock, flags);

    return fh;
}

i32 vfs_close(vfs_handle_t fh) {
    u64 flags;
    i32 r = -1;
    spin_lock_irq(&vfs_lock, flags);

    vfs_file_t *fd = null;

    task_t *t = sched_get_task();
    dlist_foreach(&t->open_files, entry) {
        fd = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (fd->f_handle == fh) {
            break;
        }
        fd = null;
    }

    fd->count--;
    if (fd->count >= 0) {
        spin_unlock_irq(&vfs_lock, flags);
        return 0;
    }

    if (fd && fd->f_ops) {
        spin_unlock_irq(&vfs_lock, flags);
        r = fd->f_ops->close(fd->f_inode, fd);
        spin_lock_irq(&vfs_lock, flags);
    }

    dlist_remove_entry(&fd->open_list_entry);
    kfree(fd);

    spin_unlock_irq(&vfs_lock, flags);
    return r;
}

i32 vfs_read(vfs_handle_t fh, i32 len, char* buff) {
    u64 flags;
    i32 r = 0;

    spin_lock_irq(&vfs_lock, flags);

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
        spin_unlock_irq(&vfs_lock, flags);
        r = fd->f_ops->read(fd->f_inode, fd, len, buff);
        spin_lock_irq(&vfs_lock, flags);
    }

    spin_unlock_irq(&vfs_lock, flags);
    return r;
}

i32 vfs_write(vfs_handle_t fh, i32 len, const char* buff) {
    u64 flags;
    i32 r = 0;

    spin_lock_irq(&vfs_lock, flags);

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
        spin_unlock_irq(&vfs_lock, flags);
        r = fd->f_ops->write(fd->f_inode, fd, len, buff);
        spin_lock_irq(&vfs_lock, flags);
    }

    spin_unlock_irq(&vfs_lock, flags);
    return r;
}

i32 vfs_lseek(vfs_handle_t fh, i32 offset , i32 wence) {
    u64 flags;
    i32 r = 0;

    spin_lock_irq(&vfs_lock, flags);

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
        //r = fd->f_ops->lseek(fd->f_inode, fd, offset, wence);
        spin_unlock_irq(&vfs_lock, flags);
        r = fd->f_ops->lseek(fd->f_inode, fd, offset, wence);
        spin_lock_irq(&vfs_lock, flags);
    }

    spin_unlock_irq(&vfs_lock, flags);
    return r;
}

i32 vfs_get_full_path(vfs_handle_t fh, i32 len, char *buff) {
    u64 flags;
    i32 r = 0;

    spin_lock_irq(&vfs_lock, flags);

    vfs_file_t *fd = null;

    task_t *t = sched_get_task();
    dlist_foreach(&t->open_files, entry) {
        fd = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (fd->f_handle == fh) {
            break;
        }
        fd = null;
    }

    // if (fd && fd->f_ops) {
    //     r = fd->f_ops->lseek(fd->f_inode, fd, offset, wence);
    // }

    char full_path[VFS_MAX_FS_NAME] = {0};
    char temp_path[VFS_MAX_FS_NAME] = {0};
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
        dentry = dentry->d_parent;

        memcpy(temp_path+1, full_path, strlen(full_path));
        memcpy(temp_path, "/", 1);
        memcpy(full_path, temp_path, strlen(temp_path));
    }

    memcpy(buff, full_path, min(strlen(full_path), len));

    spin_unlock_irq(&vfs_lock, flags);
    return r;
}

i32 vfs_iterate(vfs_handle_t fh, i32 *filecnt, vfs_dirent_t **dirent) {
    u64 flags;
    i32 r = -1;

    spin_lock_irq(&vfs_lock, flags);

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
        //r = fd->f_ops->iterate(fd->f_inode, fd, null, filecnt, dirent);
        spin_unlock_irq(&vfs_lock, flags);
        r = fd->f_ops->iterate(fd->f_inode, fd, null, filecnt, dirent);
        spin_lock_irq(&vfs_lock, flags);
    }

    spin_unlock_irq(&vfs_lock, flags);
    return r;
}

vfs_handle_t vfs_open_console(char *path, vfs_handle_t fh) {
    u64 flags;

    spin_lock_irq(&vfs_lock, flags);

    vfs_dentry_t *dentry = vfs_resolve_path(path, R_NO_CREATE);
    if (dentry == null) {
        spin_unlock_irq(&vfs_lock, flags);
        return VFS_INVALID_HANDLE;
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

    fd->f_inode = dentry->d_inode;
    fd->f_ops = dentry->d_inode->i_fops;

    fd->f_ops->open(dentry->d_inode, fd);

    task_t *t = sched_get_task();
    dlist_add_prev(&t->open_files, &fd->open_list_entry);

    spin_unlock_irq(&vfs_lock, flags);

    return fh;
}

vfs_handle_t vfs_close_console(vfs_handle_t fh) {
    u64 flags;
    i32 r = -1;

    spin_lock_irq(&vfs_lock, flags);

    vfs_file_t *fd = null;

    task_t *t = sched_get_task();
    dlist_foreach(&t->open_files, entry) {
        fd = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (fd->f_handle == fh && fd->is_console) {
            break;
        }
        fd = null;
    }

    if (fd && fd->f_ops) {
        r = fd->f_ops->close(fd->f_inode, fd);
    }

    dlist_remove_entry(&fd->open_list_entry);
    kfree(fd);

    spin_unlock_irq(&vfs_lock, flags);
    return r;
}

void vfs_copy(linked_list_t *p) {
    u64 flags;

    spin_lock_irq(&vfs_lock, flags);

    // TODO vfs_struct
    task_t *t = sched_get_task();

    dlist_foreach(p, entry) {
        vfs_file_t *filep = dlist_container_of(entry, vfs_file_t, open_list_entry);
        if (!filep->is_console) {
            filep->count++; //TODO
            vfs_file_t *nf = (vfs_file_t *)kzalloc(sizeof(vfs_file_t));
            memcpy(nf, filep, sizeof(vfs_file_t));
            dlist_add_prev(&t->open_files, &nf->open_list_entry);
        }
    }

    spin_unlock_irq(&vfs_lock, flags);
}
