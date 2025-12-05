#ifndef __VFS_H
#define __VFS_H
#include <lib/ktypes.h>
#include <base/linkedlist.h>
#include <lib/ktypes.h>


#define VFS_MAX_PATH_LENGTH         256
#define VFS_MAX_FS_NAME             16

#define VFS_INVALID_HANDLE          -1
#define VFS_MIN_HANDLE              100

#define R_CREATE                    0b001
#define R_NO_CREATE                 0b010

#define SEEK_SET                    0
#define SEEK_CUR                    1
#define SEEK_END                    2

#define VFS_STDIN                   0
#define VFS_STDOUT                  1
#define VFS_STDERR                  2


typedef struct _task_t task_t;

typedef enum {
    VFS_NODE_INVALID,
    VFS_NODE_FILE,
    VFS_NODE_DIRECTOR,
    VFS_NODE_SYMLINK,
    VFS_NODE_BLOCK,
    VFS_NODE_CHARACTER,
    VFS_NODE_MOUNTPOINT,
    VFS_NODE_ROOT,
} vfs_inode_type_t;


typedef struct _vfs_fs_t vfs_fs_t;
typedef struct _vfs_fs_ops_t vfs_fs_ops_t;

typedef struct _vfs_super_block_t vfs_super_block_t;
typedef struct _vfs_super_block_ops_t vfs_super_block_ops_t;

typedef struct _vfs_dentry_t vfs_dentry_t;
typedef struct _vfs_dentry_ops_t vfs_dentry_ops_t;

typedef struct _vfs_inode_t vfs_inode_t;
typedef struct _vfs_inode_ops_t vfs_inode_ops_t;

typedef struct _vfs_file_t vfs_file_t;
typedef struct _vfs_file_ops_t vfs_file_ops_t;


typedef struct _vfs_super_block_ops_t {
} vfs_super_block_ops_t;

typedef struct _vfs_super_block_t {
    vfs_dentry_t *s_root;
    vfs_super_block_ops_t *s_ops;
    vfs_fs_t *s_fs;
    void *s_pdata;     /*private data*/
} vfs_super_block_t;


typedef struct _vfs_inode_ops_t {
    vfs_dentry_t *(*lookup)(vfs_inode_t *, vfs_dentry_t *);
} vfs_inode_ops_t;

typedef struct _vfs_inode_t {
    size_t i_fsize;
    // u32 i_fattr;

    vfs_inode_ops_t *i_ops;
    vfs_inode_type_t i_type;
    vfs_super_block_t *i_sb;

    vfs_file_ops_t *i_fops;
    vfs_super_block_t *i_mountpoint;

    void *i_pdata;      /*private data*/
} vfs_inode_t;


typedef struct _vfs_dentry_ops_t {
    bool (*compare)(vfs_dentry_t *, char *dest_name);
    i64 (*iput)(vfs_dentry_t *, vfs_inode_t *dest_dentry);
} vfs_dentry_ops_t;

typedef struct _vfs_dentry_t {
    linked_list_t d_list_entry;     /*sibling vfs_dentry_t*/
    
    char d_name[VFS_MAX_PATH_LENGTH];

    vfs_inode_t *d_inode;
    vfs_dentry_t *d_parent;
    vfs_dentry_ops_t *d_ops;

    linked_list_t d_subdirs;        /*child vfs_dentry_t*/
} vfs_dentry_t;


typedef struct _vfs_dirent_t {
    char name[VFS_MAX_PATH_LENGTH];
    vfs_inode_type_t type;
    u32 sz;
} vfs_dirent_t;



typedef i32 vfs_handle_t;

typedef enum _vfs_openmode_t {
    VFS_MODE_READ = 0,
    VFS_MODE_WRITE = 1,
    VFS_MODE_READWRITE = 2,
} vfs_openmode_t;

typedef struct _vfs_file_ops_t {
    i32 (*open)(vfs_inode_t *, vfs_file_t *);
    i32 (*close)(vfs_inode_t *, vfs_file_t *);
    i32 (*read)(vfs_inode_t *, vfs_file_t *, i32 len, char *buffer);
    i32 (*write)(vfs_inode_t *, vfs_file_t *, i32 len, const char *buffer);
    i32 (*lseek)(vfs_inode_t *, vfs_file_t *, i32 offset , i32 wence);

    i32 (*iterate)(vfs_inode_t *, vfs_file_t *, char *path, i32 *filecnt, vfs_dirent_t **dirent);
} vfs_file_ops_t;

typedef struct _vfs_file_t {
    vfs_handle_t f_handle;
    vfs_openmode_t f_openmode;
    vfs_inode_t *f_inode;
    vfs_dentry_t *f_dentry;
    vfs_file_ops_t *f_ops;
    void *f_pdata;      /*private data*/
    size_t f_pos;

    linked_list_t open_list_entry;

    bool is_console;
    int count;
} vfs_file_t;


typedef struct _vfs_fs_t {
    linked_list_t list_entry;
    char fs_name[VFS_MAX_FS_NAME];
    vfs_super_block_t *(*mksb)(vfs_fs_t *);
} vfs_fs_t;



void vfs_init();

void vfs_register_fs(vfs_fs_t *);
vfs_fs_t *vfs_get_fs(char *name);

i32 vfs_mount_fs(const char *path, const char *fs_name);

bool vfs_next_path(char *path, char *next, size_t nextlen, bool skiproot);
vfs_dentry_t *vfs_resolve_path(const char *path, u8 mode);


vfs_handle_t vfs_open(char *path, vfs_openmode_t openmode);
i32 vfs_close(vfs_handle_t);
i32 vfs_read(vfs_handle_t, i32 len, char* buff);
i32 vfs_write(vfs_handle_t, i32 len, const char* buff);
i32 vfs_lseek(vfs_handle_t, i32 offset , i32 wence);
i32 vfs_get_full_path(vfs_handle_t, i32 len, char *buff);

i32 vfs_iterate(vfs_handle_t, i32 *filecnt, vfs_dirent_t **dirent);

vfs_handle_t vfs_open_console(char *path, vfs_handle_t fh);
i32 vfs_close_console(vfs_handle_t fh);

void vfs_init();

void vfs_copy(linked_list_t *p);


#endif