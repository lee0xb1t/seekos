#include <fs/fat32.h>
#include <fs/vfs.h>
#include <base/spinlock.h>
#include <device/storage/ata.h>
#include <panic/panic.h>
#include <log/klog.h>
#include <mm/kmalloc.h>
#include <lib/kmemory.h>
#include <lib/kstring.h>


DECLARE_SPINLOCK(fat_lock);

static volatile boot_record_t mbr;
static volatile active_partition = -1;

vfs_fs_t fat32_fs = {
    .fs_name = "fat32",
    .mksb = fat32_mksb,
};

vfs_dentry_ops_t fat32_dentry_ops = {
    .compare = null,
    .iput = null,
};

vfs_inode_ops_t fat32_inode_ops = {
    .lookup = fat32_lookup,
};

vfs_file_ops_t fat32_file_ops = {
    .open = fat32_open,
    .close = fat32_close,
    .read = fat32_read,
    .write = fat32_write,
    .lseek = fat32_lseek,
    .iterate = fat32_iterate,
};


u32 _clus_to_lba(boot_record_t *, u32 data_start_lba, u32 clus);
bool _lookup(fat_dir_entry_t *, char* expect_name, fat_dir_entry_t **out_dir_entry, fat_lfn_entry_t **out_lfn_entry);
void _upper_to_lower(char *);
void _lower_to_upper(char *);
void _remove_space(char *);
void _get_sub_direntry(fat32_sb_data_t *sdata, u32 start_clus, fat_dir_entry_t **out_dir_entry);
u32 _size_align_to_clus(fat32_sb_data_t *sdata, u32 size);
u32 _get_used_clus_count(fat32_sb_data_t *sdata, u32 start_clus);
u32 _size_to_clus(fat32_sb_data_t *sdata, u32 size);
u32 _find_last_clus(fat32_sb_data_t *sdata, fat32_inode_data_t *idata);
u32 _alloc_fat(fat32_sb_data_t *sdata, u32 prev_clus);
u32 _expand(fat32_sb_data_t *sdata, fat32_inode_data_t *idata, u32 expand_clus);
u32 _get_next_fat_entry(fat32_sb_data_t *sdata, u32 clus);


void fat32_init() {
    ata_pio_read28(null, 1, 0, &mbr);
    if (mbr.valid_sign != 0xaa55) {
        panic("[FAT32] not found boot sector");
    }

    for (int i = 0; i < 4; i++) {
        klogi("partition%d: status = %p, type = %p, lba start = %p, count = %d\n", 
            i,
            mbr.partitions[i].status,
            mbr.partitions[i].type,
            mbr.partitions[i].lba_start,
            mbr.partitions[i].sector_count
        );

        if (mbr.partitions[i].status == 0x80 && 
            mbr.partitions[i].type == 0x0c) {
            
            active_partition = i;
        }
    }

    if (active_partition == -1) {
        panic("[FAT32] not found active partition");
    }

    vfs_register_fs(&fat32_fs);

    klogi("FAT32 Initialized.\n");
}


vfs_super_block_t *fat32_mksb(vfs_fs_t *fs) {
    mbr_partition_t *partition = &mbr.partitions[active_partition];
    boot_record_t *pbr = kzalloc(sizeof(boot_record_t));
    ata_pio_read28(null, 1, partition->lba_start, pbr);

    u32 fat1_start_lba = partition->lba_start + pbr->bpb_rsvd_sec_cnt;
    u32 fat2_start_lba = fat1_start_lba + pbr->bpb_fatsz32;
    u32 data_start_lba = fat1_start_lba + pbr->bpb_fatsz32 * pbr->bpb_num_fats;

    u32 *fat1_table = (u32 *)kzalloc(pbr->bpb_fatsz32 * pbr->bpb_bytes_per_sec);
    ata_pio_read28(null, pbr->bpb_fatsz32, fat1_start_lba, fat1_table);

    u32 *fat2_table = (u32 *)kzalloc(pbr->bpb_fatsz32 * pbr->bpb_bytes_per_sec);
    ata_pio_read28(null, pbr->bpb_fatsz32, fat2_start_lba, fat2_table);

    fat32_sb_data_t *sdata = (fat32_sb_data_t *)kzalloc(sizeof(fat32_sb_data_t));
    sdata->parti_start_lba = partition->lba_start;
    sdata->sector_count = partition->sector_count;
    sdata->fat1_start_lba = fat1_start_lba;
    sdata->fat2_start_lba = fat2_start_lba;
    sdata->data_start_lba = data_start_lba;
    sdata->fat1_data = fat1_table;
    //sdata->fat1_sec = pbr->bpb_fatsz32 * pbr->bpb_bytes_per_sec;
    sdata->fat2_data = fat2_table;
    //sdata->fat2_len = pbr->bpb_fatsz32 * pbr->bpb_bytes_per_sec;
    sdata->fat_sec = pbr->bpb_fatsz32;
    sdata->pbr = pbr;
    

    // create super block
    vfs_super_block_t *sb = kzalloc(sizeof(vfs_super_block_t));
    sb->s_fs = fs;
    sb->s_ops = null;
    //
    sb->s_pdata = (void *)sdata;


    // create dentry
    vfs_dentry_t *dentry = kzalloc(sizeof(vfs_dentry_t));
    dentry->d_ops = &fat32_dentry_ops;
    dentry->d_parent = dentry;
    dentry->d_name[0] = '/';
    dlist_init(&dentry->d_subdirs);
    //
    sb->s_root = dentry;


    // create inode
    vfs_inode_t *inode = kzalloc(sizeof(vfs_inode_t));
    inode->i_fsize = 0;
    inode->i_mountpoint = null;
    inode->i_ops = &fat32_inode_ops;
    inode->i_sb = sb;
    inode->i_type = VFS_NODE_DIRECTOR;
    
    inode->i_fops = &fat32_file_ops;
    //
    dentry->d_inode = inode;


    // inode data
    fat32_inode_data_t *i_data = (fat32_inode_data_t *)kzalloc(sizeof(fat32_inode_data_t));
    i_data->is_root = true;
    i_data->start_clus = pbr->bpb_root_clus;
    i_data->start_lba = _clus_to_lba(pbr, data_start_lba, pbr->bpb_root_clus);
    i_data->next_clus = fat1_table[pbr->bpb_root_clus];
    inode->i_pdata = i_data;

    return sb;
}

vfs_dentry_t *fat32_lookup(vfs_inode_t *this, vfs_dentry_t *expect_dentry) {
    bool is_dir = false;
    u32 read_sector = 0;
    u32 next_clus = 0;

    spin_lock(&fat_lock);

    fat32_sb_data_t *s_data = (fat32_sb_data_t *)this->i_sb->s_pdata;
    fat32_inode_data_t *inode_data = (fat32_inode_data_t *)this->i_pdata;

    if (inode_data->is_root) {
        is_dir = true;
    } else {
        if (inode_data->dir_entry.attr & ATTR_DIRECTORY) {
            is_dir = true;
        }
    }
    
    fat_dir_entry_t *dir_entry = null;
    _get_sub_direntry(s_data, inode_data->start_clus, &dir_entry);

    fat_dir_entry_t *temp_dir_entry = null;
    fat_lfn_entry_t *temp_lfn_entry = null;

    bool is_found = _lookup(dir_entry, expect_dentry->d_name, &temp_dir_entry, &temp_lfn_entry);

    if (!is_found) {
        kfree(dir_entry);
        spin_unlock(&fat_lock);
        return null;
    }

    vfs_inode_t *inode = (vfs_inode_t *)kzalloc(sizeof(vfs_inode_t));
    inode->i_sb = this->i_sb;
    inode->i_fsize = temp_dir_entry->file_size;
    if (temp_dir_entry->attr & ATTR_DIRECTORY) {
        inode->i_type = VFS_NODE_DIRECTOR;
    } else if (temp_dir_entry->attr & ATTR_ARCHIVE) {
        inode->i_type = VFS_NODE_FILE;
    }
    inode->i_ops = &fat32_inode_ops;
    inode->i_fops = &fat32_file_ops;

    u32 entry_start_clus = ((u32)temp_dir_entry->cluster_high << 16) | temp_dir_entry->cluster_low;
    fat32_inode_data_t *i_data = (fat32_inode_data_t *)kzalloc(sizeof(fat32_inode_data_t));
    i_data->is_root = false;
    i_data->start_clus = entry_start_clus;
    i_data->start_lba = _clus_to_lba(s_data->pbr, s_data->data_start_lba, entry_start_clus);
    i_data->next_clus = s_data->fat1_data[entry_start_clus];
    i_data->dir_entry = *temp_dir_entry;
    if (temp_lfn_entry) {
        i_data->lfn_entry = *temp_lfn_entry;
    }
    i_data->file_size = i_data->dir_entry.file_size;
    inode->i_pdata = i_data;
    inode->i_fsize = i_data->dir_entry.file_size;

    kfree(dir_entry);

    expect_dentry->d_inode = inode;
    expect_dentry->d_ops = &fat32_dentry_ops;

    spin_unlock(&fat_lock);
    return expect_dentry;
}

u32 _clus_to_lba(boot_record_t *br, u32 data_start_lba, u32 clus) {
    return data_start_lba + (clus - 2) * br->bpb_sec_per_clus;
}

bool _lookup(fat_dir_entry_t *dir_entry, char* expect_name, fat_dir_entry_t **out_dir_entry, fat_lfn_entry_t **out_lfn_entry) {
    fat_dir_entry_t *temp_dir_entry = dir_entry;
    fat_lfn_entry_t *temp_lfn_entry = null;
    bool is_found = false;

    for (;;) {
        if (temp_dir_entry->name[0] == '\0' && temp_dir_entry->attr == 0) {
            break;
        }

        if (temp_dir_entry->attr & ATTR_LONG_NAME) {
            // skip lfn entry
            //panic("TODO: lfn entry");
        } else if ((temp_dir_entry->attr & ATTR_DIRECTORY) || (temp_dir_entry->attr & ATTR_ARCHIVE)) {

            char dir_name[VFS_MAX_PATH_LENGTH] = {0};
            memcpy(dir_name, temp_dir_entry->name, 8);
            _remove_space(dir_name);
            if ((temp_dir_entry->name+8)[0] != ' ') {
                (dir_name+strlen(dir_name))[0] = '.';
                memcpy(dir_name+strlen(dir_name), temp_dir_entry->name+8, 3);
            }
            
            _remove_space(dir_name);
            _upper_to_lower(dir_name);
            
            if (strcmp(dir_name, expect_name)) {
                is_found = true;
                break;
            }
        }

        temp_dir_entry++;
    }

    if (is_found) {
        *out_dir_entry = temp_dir_entry;
        *out_lfn_entry = temp_lfn_entry;
        return true;
    }

    return false;
}

void _upper_to_lower(char *name) {
    size_t len = strlen(name);
    for (int i = 0; i < len; i++) {
        if (name[i] >= 0x41 && name[i] <= 0x5a) {
            name[i] += 0x20;
        }
    }
}

void _lower_to_upper(char *name) {
    size_t len = strlen(name);
    for (int i = 0; i < len; i++) {
        if (name[i] >= 0x61 && name[i] <= 0x7a) {
            name[i] -= 0x20;
        }
    }
}

void _remove_space(char *name) {
    size_t len = strlen(name);
    for (int i = 0; i < len; i++) {
        if (name[i] == ' ') {
            name[i] = '\0';
        }
    }
}

void _get_sub_direntry(fat32_sb_data_t *sdata, u32 start_clus, fat_dir_entry_t **out_dir_entry) {
    u32 read_clus = 0;
    u32 temp_start_clus = start_clus;

    fat_entry_t *fat_table = sdata->fat1_data;

    while (true) {
        if (temp_start_clus > CLUSTER_FREE && temp_start_clus < CLUSTER_BAD) {
            read_clus++;
            temp_start_clus = fat_table[temp_start_clus];
        } else if (temp_start_clus >= CLUSTER_EOF_MIN) {
            break;
        } else {
            temp_start_clus++;
        }
    }
    
    u32 bytes_per_clus = sdata->pbr->bpb_sec_per_clus * sdata->pbr->bpb_bytes_per_sec;
    u8 *dir_entry = (u8 *)kzalloc(read_clus * bytes_per_clus);
    
    temp_start_clus = start_clus;
    for (int i = 0; i < read_clus; i++) {
        if (temp_start_clus > CLUSTER_FREE && temp_start_clus < CLUSTER_BAD) {
            u32 entry_lba = _clus_to_lba(sdata->pbr, sdata->data_start_lba, temp_start_clus);
            ata_pio_read28(null, sdata->pbr->bpb_sec_per_clus, entry_lba, &dir_entry[i*bytes_per_clus]);
            temp_start_clus = fat_table[temp_start_clus];
        } else if (temp_start_clus >= CLUSTER_EOF_MIN) {
            break;
        } else {
            temp_start_clus++;
            i--;
        }
    }

    *out_dir_entry = (fat_dir_entry_t *)dir_entry;
}

i32 fat32_open(vfs_inode_t *this, vfs_file_t *filep) {
    // TODO
    // lock
}

i32 fat32_close(vfs_inode_t *this, vfs_file_t *filep) {
    // TODO
    // lock
}

i32 fat32_read(vfs_inode_t *this, vfs_file_t *filep, i32 len, char *buffer) {
    spin_lock(&fat_lock);

    fat32_sb_data_t *s_data = (fat32_sb_data_t *)this->i_sb->s_pdata;
    fat32_inode_data_t *inode_data = (fat32_inode_data_t *)this->i_pdata;

    fat_entry_t *fat_table = s_data->fat1_data;

    if (this->i_fsize <= filep->f_pos) {
        spin_unlock(&fat_lock);
        return -1;
    }

    u32 fpos = (u32)filep->f_pos;
    u32 bytes_per_clus = s_data->pbr->bpb_bytes_per_sec * s_data->pbr->bpb_sec_per_clus;

    u32 read_clus_start = fpos / bytes_per_clus;
    u32 read_clus_count = _size_align_to_clus(s_data, (fpos % bytes_per_clus) + len) / bytes_per_clus;

    u32 read_start_clus = inode_data->start_clus;
    for (int i = 0; i < read_clus_start; i++) {
        if (read_start_clus > CLUSTER_FREE && read_start_clus < CLUSTER_BAD) {
            read_start_clus = fat_table[read_start_clus];
        } else if (read_start_clus >= CLUSTER_EOF_MIN) {
            break;
        } else {
            read_start_clus++;
            i--;
        }
    }

    u8 *fdata = (u8 *)kzalloc(read_clus_count * bytes_per_clus);
    for (int i = 0; i < read_clus_count; i++) {
        if (read_start_clus > CLUSTER_FREE && read_start_clus < CLUSTER_BAD) {
            u32 lba = _clus_to_lba(s_data->pbr, s_data->data_start_lba, read_start_clus);
            ata_pio_read28(null, s_data->pbr->bpb_sec_per_clus, lba, &fdata[i*bytes_per_clus]);
            read_start_clus = fat_table[read_start_clus];
        } else if (read_start_clus >= CLUSTER_EOF_MIN) {
            break;
        } else {
            read_start_clus++;
            i--;
        }
    }
    
    memcpy(buffer, fdata + (filep->f_pos % bytes_per_clus), len);
    filep->f_pos += len;

    kfree(fdata);

    spin_unlock(&fat_lock);
    return 0;
}

i32 fat32_write(vfs_inode_t *this, vfs_file_t *filep, i32 len, const char *buffer) {
    spin_lock(&fat_lock);

    fat32_sb_data_t *s_data = (fat32_sb_data_t *)this->i_sb->s_pdata;
    fat32_inode_data_t *i_data = (fat32_inode_data_t *)this->i_pdata;

    fat_entry_t *fat_table = s_data->fat1_data;

    u32 fpos = (u32)filep->f_pos;
    u32 bytes_per_sec = s_data->pbr->bpb_bytes_per_sec;
    u32 bytes_per_clus = s_data->pbr->bpb_bytes_per_sec * s_data->pbr->bpb_sec_per_clus;

    u32 last_clus = _find_last_clus(s_data, i_data);

    u32 usedclus = _get_used_clus_count(s_data, i_data->start_clus);
    u32 usedsz = usedclus * bytes_per_clus;

    u32 remainsz = usedsz - fpos + 1;
    u32 needsz = 0;
    u32 need_clus = 0;
    
    if (remainsz < len) {
        // expand
        needsz = fpos + 1 + len - usedsz;
        needsz = _size_align_to_clus(s_data, needsz);
        need_clus = _size_to_clus(s_data, needsz);
        _expand(s_data, i_data, need_clus);
    }

    // write
    u32 last_lba = _clus_to_lba(s_data->pbr, s_data->data_start_lba, last_clus);

    u32 start_pos = fpos % bytes_per_clus;
    u8 *temp_buf = (u8 *)kzalloc((need_clus + 1) * bytes_per_clus + bytes_per_clus);
    ata_pio_read28(null, s_data->pbr->bpb_sec_per_clus, last_lba, temp_buf);
    memcpy(temp_buf+start_pos, buffer, remainsz);
    memcpy(temp_buf+start_pos+remainsz, (u8 *)buffer+remainsz, len - remainsz);

    u32 next_clus = last_clus;
    for (int i = 0; i < need_clus + 1; i++) {
        if (next_clus > CLUSTER_FREE && next_clus < CLUSTER_BAD) {
            u32 lba = _clus_to_lba(s_data->pbr, s_data->data_start_lba, next_clus);
            ata_pio_write28(null, s_data->pbr->bpb_sec_per_clus, lba, &temp_buf[i*bytes_per_clus]);
            next_clus = fat_table[next_clus];
        } else if (next_clus >= CLUSTER_EOF_MIN) {
            break;
        } else {
            next_clus++;
            i--;
        }
    }
    
    kfree(temp_buf);

    // TODO
    // modify fat entry size
    // modify time

    filep->f_pos += len;
    this->i_fsize += len;
    
    spin_unlock(&fat_lock);
}

i32 fat32_lseek(vfs_inode_t *this, vfs_file_t *filep, i32 offset , i32 wence) {
    spin_lock(&fat_lock);

    fat32_sb_data_t *s_data = (fat32_sb_data_t *)this->i_sb->s_pdata;
    fat32_inode_data_t *inode_data = (fat32_inode_data_t *)this->i_pdata;

    if (wence == SEEK_SET) {
        filep->f_pos = 0;
    } else if (wence == SEEK_END) {
        filep->f_pos = this->i_fsize;
    } else {
        // nothing
    }

    filep->f_pos += offset;

    spin_unlock(&fat_lock);
    return filep->f_pos;
}

i32 fat32_iterate(vfs_inode_t *this, vfs_file_t *filep, char *path, i32 *filecnt, vfs_dirent_t **out_dirent) {
    fat32_sb_data_t *s_data = (fat32_sb_data_t *)this->i_sb->s_pdata;
    fat32_inode_data_t *i_data = (fat32_inode_data_t *)this->i_pdata;

    spin_lock(&fat_lock);

    fat_dir_entry_t *dir_entry = null;
    _get_sub_direntry(s_data, i_data->start_clus, &dir_entry);

    fat_dir_entry_t *temp_dir_entry = dir_entry;
    fat_lfn_entry_t *temp_lfn_entry = null;

    int lfn_count = 0;
    int file_count = 0;
    vfs_dirent_t *dirent = null;

    for (;;) {
        if (temp_dir_entry->name[0] == '\0' && temp_dir_entry->attr == 0) {
            break;
        }

        if (temp_dir_entry->attr & ATTR_LONG_NAME) {
            lfn_count++;
            // skip lfn entry
            //panic("TODO: lfn entry");
        } else if ((temp_dir_entry->attr & ATTR_DIRECTORY) || (temp_dir_entry->attr & ATTR_ARCHIVE)) {
            if (lfn_count > 0) {
                lfn_count = 0;
                continue;
            }

            char dir_name[VFS_MAX_PATH_LENGTH] = {0};
            memcpy(dir_name, temp_dir_entry->name, 8);
            _remove_space(dir_name);
            if ((temp_dir_entry->name+8)[0] != ' ') {
                (dir_name+strlen(dir_name))[0] = '.';
                memcpy(dir_name+strlen(dir_name), temp_dir_entry->name+8, 3);
            }
            
            _remove_space(dir_name);
            _upper_to_lower(dir_name);
            
            if (dirent == null) {
                dirent = (vfs_dirent_t *)kzalloc(sizeof(vfs_dirent_t));
                file_count++;
            } else {
                dirent = (vfs_dirent_t *)krealloc(dirent, file_count * sizeof(vfs_dirent_t), (++file_count) * sizeof(vfs_dirent_t));
            }

            vfs_dirent_t *temp_dirent = &dirent[file_count - 1];
            memcpy(temp_dirent->name, dir_name, strlen(dir_name));
            temp_dirent->sz = temp_dir_entry->file_size;
            if (temp_dir_entry->attr & ATTR_ARCHIVE) {
                temp_dirent->type = VFS_NODE_FILE;
            } else {
                temp_dir_entry->attr = VFS_NODE_DIRECTOR;
            }
        }

        temp_dir_entry++;
    }

    spin_unlock(&fat_lock);
    
    *out_dirent = dirent;
    *filecnt = file_count;
    return file_count;
}

u32 _size_align_to_clus(fat32_sb_data_t *sdata, u32 size) {
    u32 bytes_per_clus = sdata->pbr->bpb_bytes_per_sec * sdata->pbr->bpb_sec_per_clus;
    if (size < bytes_per_clus) {
        size = bytes_per_clus;
        return size;
    }

    size = size - (size % bytes_per_clus) + bytes_per_clus;

    // size += bytes_per_clus;
    // size = size & (~bytes_per_clus);

    return size;
}

u32 _get_used_clus_count(fat32_sb_data_t *sdata, u32 start_clus) {
    fat_entry_t *fat1_table = sdata->fat1_data;
    int sz = 0;
    for (;;) {
        if (start_clus > CLUSTER_FREE && start_clus < CLUSTER_BAD) {
            start_clus = fat1_table[start_clus];
            sz++;
        } else if (start_clus >= CLUSTER_EOF_MIN) {
            break;
        } else {
            start_clus++;
        }
    }
    return sz;
}

u32 _size_to_clus(fat32_sb_data_t *sdata, u32 size) {
    return size / (sdata->pbr->bpb_bytes_per_sec * sdata->pbr->bpb_sec_per_clus);
}

u32 _find_last_clus(fat32_sb_data_t *sdata, fat32_inode_data_t *idata) {
    fat_entry_t *fat1_table = sdata->fat1_data;
    u32 last_clus = idata->start_clus;
    u32 prev_clus = 0;
    for (;;) {
        if (last_clus > CLUSTER_FREE && last_clus < CLUSTER_BAD) {
            prev_clus = last_clus;
            last_clus = fat1_table[last_clus];
        } else if (last_clus >= CLUSTER_EOF_MIN) {
            break;
        } else {
            last_clus++;
        }
    }
    return prev_clus;
}

u32 _alloc_fat(fat32_sb_data_t *sdata, u32 prev_clus) {
    fat_entry_t *fat1_table = sdata->fat1_data;
    fat_entry_t *fat2_table = sdata->fat2_data;
    u32 fat_sz = sdata->fat_sec * sdata->pbr->bpb_bytes_per_sec / sizeof(fat_entry_t);
    u32 idx = 0;
    for (; idx < fat_sz; idx++) {
        if (!fat1_table[idx]) {
            fat1_table[prev_clus] = idx;
            fat1_table[idx] = CLUSTER_EOF_MAX;
            fat2_table[prev_clus] = idx;
            fat2_table[idx] = CLUSTER_EOF_MAX;
            ata_pio_write28(null, sdata->fat_sec, sdata->fat1_start_lba, fat1_table);
            ata_pio_write28(null, sdata->fat_sec, sdata->fat2_start_lba, fat2_table);
            return idx;
        }
    }

    panic("[FAT32] not enough fat entry");
    return 0;
}

u32 _expand(fat32_sb_data_t *sdata, fat32_inode_data_t *idata, u32 expand_clus) {
    boot_record_t *pbr = sdata->pbr;
    u32 fat_sz = sdata->fat_sec * pbr->bpb_bytes_per_sec;

    u32 last_clus = _find_last_clus(sdata, idata);
    for (int i = 0; i < expand_clus; i++) {
        u32 newfat = _alloc_fat(sdata, last_clus);
        last_clus = newfat;
    }
    return last_clus;
}

u32 _get_next_fat_entry(fat32_sb_data_t *sdata, u32 clus) {
    fat_entry_t *fat1_table = sdata->fat1_data;
    for (;;) {
        if (clus > CLUSTER_FREE && clus < CLUSTER_BAD) {
            clus = fat1_table[clus];
            return clus;
        } else if (clus >= CLUSTER_EOF_MIN) {
            return CLUSTER_EOF_MAX;
        } else {
            // skip
        }
    }
}
