#ifndef FAT32_H
#define FAT32_H
#include <lib/ktypes.h>
#include <fs/vfs.h>

//------------------------------------------------------------------------------
enum
{
  FAT_ERR_NONE     =  0,
  FAT_ERR_NOFAT    = -1,
  FAT_ERR_BROKEN   = -2,
  FAT_ERR_IO     = -3,
  FAT_ERR_PARAM    = -4,
  FAT_ERR_PATH     = -5,
  FAT_ERR_EOF      = -6,
  FAT_ERR_DENIED   = -7,
  FAT_ERR_FULL     = -8,
};

enum
{
  FAT_ATTR_NONE     = 0x00,
  FAT_ATTR_RO       = 0x01,
  FAT_ATTR_HIDDEN   = 0x02,
  FAT_ATTR_SYS      = 0x04,
  FAT_ATTR_LABEL    = 0x08,
  FAT_ATTR_DIR      = 0x10,
  FAT_ATTR_ARCHIVE  = 0x20,
  FAT_ATTR_LFN      = 0x0f,
};

enum
{
  FAT_WRITE      = 0x01, // Open file for writing
  FAT_READ       = 0x02, // Open file for reading
  FAT_APPEND     = 0x04, // Set file offset to the end of the file
  FAT_TRUNC      = 0x08, // Truncate the file after opening
  FAT_CREATE     = 0x10, // Create the file if it do not exist

  FAT_ACCESSED   = 0x20, // do not use (internal)
  FAT_MODIFIED   = 0x40, // do not use (internal)
  FAT_FILE_DIRTY = 0x80, // do not use (internal)
};

enum
{
  FAT_SEEK_START,
  FAT_SEEK_CURR,
  FAT_SEEK_END,
};

//------------------------------------------------------------------------------
typedef struct
{
  bool (*read)(u8* buf, u32 sect);
  bool (*write)(const u8* buf, u32 sect);
} DiskOps;

typedef struct
{
  u8 hour;
  u8 min;
  u8 sec;
  u8 day;
  u8 month;
  u16 year;
} Timestamp;

typedef struct Fat
{
  struct Fat* next;
  DiskOps ops;
  u32 clust_msk;
  u32 clust_cnt;
  u32 info_sect;
  u32 fat_sect[2];
  u32 data_sect;  
  u32 root_clust;
  u32 last_used;
  u32 free_cnt;
  u32 sect;
  u8 buf[512];
  u8 flags;
  u8 clust_shift;
  u8 name_len;
  char name[32];
} Fat;

typedef struct
{
  Timestamp created;
  Timestamp modified;
  u32 size;
  u8 attr;
  char name[255];
  u8 name_len;
} DirInfo;

typedef struct
{
  Fat* fat;
  u32 sclust;
  u32 clust;
  u32 sect;
  u16 idx;
} Dir;

typedef struct
{
  Fat* fat;
  u32 dir_sect;
  u32 sclust;
  u32 clust;
  u32 sect;
  u32 size;
  u32 offset;
  u16 dir_idx;
  u8 attr;
  u8 flags;
  u8 buf[512];
} File;

//------------------------------------------------------------------------------
const char* fat_get_error(int err);

int fat_probe(DiskOps* ops, int partition);
int fat_mount(DiskOps* ops, int partition, Fat* fat, const char* path);
int fat_umount(Fat* fat);
int fat_sync(Fat* fat);

int fat_stat(const char* path, DirInfo* info);
int fat_unlink(const char* path);

int fat_file_open(File* file, const char* path, u8 flags);
int fat_file_close(File* file);
int fat_file_read(File* file, void* buf, int len, int* bytes);
int fat_file_write(File* file, const void* buf, int len, int* bytes);
int fat_file_seek(File* file, int offset, int seek);
int fat_file_sync(File* file);

int fat_dir_create(Dir* dir, const char* path);
int fat_dir_open(Dir* dir, const char* path);
int fat_dir_read(Dir* dir, DirInfo* info);
int fat_dir_rewind(Dir* dir);
int fat_dir_next(Dir* dir);

void fat_get_timestamp(Timestamp* ts);


//
// VFS
//

typedef struct {
  Fat* fat;
  u32 start_clus;
  u32 file_size;
  u8 attr;
  bool is_root;
  u32 dir_sect;
  u16 dir_idx;
} fat32_inode_data_t;

void fat32_init();

vfs_super_block_t *fat32_mksb(vfs_fs_t *fs);
vfs_dentry_t *fat32_lookup(vfs_inode_t *, vfs_dentry_t *);

i32 fat32_open(vfs_inode_t *, vfs_file_t *);
i32 fat32_close(vfs_inode_t *, vfs_file_t *);
i32 fat32_read(vfs_inode_t *, vfs_file_t *, i32 len, char *buffer);
i32 fat32_write(vfs_inode_t *, vfs_file_t *, i32 len, const char *buffer);
i32 fat32_lseek(vfs_inode_t *, vfs_file_t *, i32 offset , i32 wence);

i32 fat32_iterate(vfs_inode_t *, vfs_file_t *, char *path, i32 *filecnt, vfs_dirent_t **dirent);


#endif