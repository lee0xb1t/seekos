#ifndef FAT32_H
#define FAT32_H
#include <lib/ktypes.h>
#include <fs/vfs.h>


typedef struct _mbr_partition_t {
    u8 status;                  // 0x80=活动, 0x00=非活动
    u8 chs_start[3];            // 起始CHS地址
    u8 type;                    // 分区类型 (0x0B/0x0C=FAT32)
    u8 chs_end[3];              // 结束CHS地址
    u32 lba_start;              // 分区起始LBA
    u32 sector_count;           // 分区扇区数
} __attribute__((packed)) mbr_partition_t;

typedef struct _boot_record_t {
    u8 bs_jmpboot[3];
    char bs_oemname[8];
    u16 bpb_bytes_per_sec;
    u8 bpb_sec_per_clus;
    u16 bpb_rsvd_sec_cnt;
    u8 bpb_num_fats;
    u16 bpb_root_ent_cnt;
    u16 bpb_total_sec16;
    u8 bpb_media;
    u16 bpb_fatsz16;
    u16 bpb_sec_per_trk;
    u16 bpb_num_heads;
    u32 bpb_hidd_sec;
    u32 bpb_total_sec32;
    u32 bpb_fatsz32;
    u16 bpb_ext_flags;
    u16 bpb_fsver;
    u32 bpb_root_clus;
    u16 bpb_fsinfo;
    u16 bpb_bk_boot_sec;
    u8 bpb_reserved[12];
    u8 bs_drv_num;
    u8 bs_reserved1;
    u8 bs_boot_sig;
    u32 bs_vol_id;
    u8 bs_vol_lab[11];
    u64 bs_file_sys_type;
    u8 boot_code[350];
    u32 unique_disk_id;
    u16 reserved2;
    mbr_partition_t partitions[4];
    u16 valid_sign;
} __attribute__((packed)) boot_record_t;


// FAT 特殊簇值
#define CLUSTER_FREE    0x00000000
#define CLUSTER_BAD     0x0FFFFFF7
#define CLUSTER_EOF_MIN 0x0FFFFFF8
#define CLUSTER_EOF_MAX 0x0FFFFFFF

typedef u32 fat_entry_t;

// 文件属性定义
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_LONG_NAME  0x0F

typedef struct _fat_dir_entry_t {
    u8 name[11];           // 8.3格式文件名（无点分隔）
    u8 attr;               // 文件属性
    u8 nt_reserved;        // NT保留字节
    u8 crt_time_tenth;     // 创建时间的10毫秒单位
    u16 crt_time;          // 创建时间
    u16 crt_date;          // 创建日期
    u16 lst_access_date;   // 最后访问日期
    u16 cluster_high;      // 起始簇号的高16位（FAT32）
    u16 wrt_time;          // 最后修改时间
    u16 wrt_date;          // 最后修改日期
    u16 cluster_low;       // 起始簇号的低16位
    u32 file_size;         // 文件大小（字节）
} __attribute__((packed)) fat_dir_entry_t;

typedef struct _fat_lfn_entry_t {
    u8 sequence;           // 序列号和标志位
    u8 name1[10];          // 文件名第1部分 (UTF-16LE)
    u8 attr;               // 属性 (总是 0x0F)
    u8 type;               // 类型 (总是 0x00)
    u8 checksum;           // 短文件名校验和
    u8 name2[12];          // 文件名第2部分 (UTF-16LE)
    u16 cluster_low;       // 总是 0x0000
    u8 name3[4];           // 文件名第3部分 (UTF-16LE)
} __attribute__((packed)) fat_lfn_entry_t;

typedef struct fat32_sb_data_t {
    u32 parti_start_lba;
    u32 sector_count;

    u32 fat1_start_lba;
    u32 fat2_start_lba;
    u32 data_start_lba;

    fat_entry_t *fat1_data;
    fat_entry_t *fat2_data;
    u32 fat_sec;
    
    boot_record_t *pbr;
} fat32_sb_data_t;

typedef struct _fat32_inode_data_t {
    u32 start_clus;
    u32 start_lba;
    u32 file_size;
    u32 next_clus;

    bool is_root;
    fat_lfn_entry_t lfn_entry;
    fat_dir_entry_t dir_entry;
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