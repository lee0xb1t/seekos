#ifndef __TTYFS_H
#define __TTYFS_H
#include <fs/vfs.h>
#include <lib/ktypes.h>


#define TTY_FS_NAME             "ttyfs"
#define TTY_BUFFER_SIZE         4096

typedef struct _ttyfs_fdata_t {
    char ibuff[TTY_BUFFER_SIZE];
    u64 ibegin;
    u64 isize;
    u64 icursor;
} ttyfs_fdata_t;


void ttyfs_init();

vfs_super_block_t *ttyfs_mksb(vfs_fs_t *fs);

i32 ttyfs_open(vfs_inode_t *, vfs_file_t *);
i32 ttyfs_close(vfs_inode_t *, vfs_file_t *);

i32 ttyfs_read(vfs_inode_t *, vfs_file_t *, i32 len, char *buffer);
i32 ttyfs_write(vfs_inode_t *, vfs_file_t *, i32 len, const char *buffer);

vfs_dentry_t *ttyfs_lookup(vfs_inode_t *, vfs_dentry_t *);

ttyfs_fdata_t *ttyfs_new_data();
void ttyfs_free_data(ttyfs_fdata_t *);

#endif