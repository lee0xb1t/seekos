#ifndef __RAMFS_H
#define __RAMFS_H
#include <fs/vfs.h>


#define DEFAULT_RAMFS_DATA_SIZE     4096

typedef struct _ramfs_data_t {
    linked_list_t list_entry;

    size_t size;
    size_t seek;
    void *data;
    char *path[VFS_MAX_PATH_LENGTH];
} ramfs_data_t;

typedef enum _ramfs_ustar_type_t {
    U_NORMAL = 0,
    U_HARD = 1,
    U_SYMBOLIC = 2,
    U_DIRECTORY = 5,
} ramfs_ustar_type_t;

typedef struct _ramfs_ustar_data_t {
    char filename[VFS_MAX_PATH_LENGTH];
    char name[VFS_MAX_PATH_LENGTH];
    int size;
    int data_offset;
    int head_offset;
    char mode[8];
    u8 type_flag;
} ramfs_ustar_data_t;


void ramfs_init(void *buff, size_t size);

vfs_super_block_t *ramfs_mksb(vfs_fs_t *fs);

vfs_dentry_t *ramfs_lookup(vfs_inode_t *, vfs_dentry_t *);

i32 ramfs_open(vfs_inode_t *, vfs_file_t *);
i32 ramfs_close(vfs_inode_t *, vfs_file_t *);
i32 ramfs_read(vfs_inode_t *, vfs_file_t *, i32 len, char *buffer);
i32 ramfs_write(vfs_inode_t *, vfs_file_t *, i32 len, const char *buffer);

// vfs_inode_t *ramfs_mount(vfs_inode_t *, const char *path, const char *fs_name);

bool _ustar_lookup(vfs_inode_t *inode, char *name, ramfs_ustar_data_t **out_data);
void _ustar_remove_last_slash(char *path);
void _ustar_add_last_slash(char *path);


#endif