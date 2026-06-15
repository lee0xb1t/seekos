#include <fs/fat32.h>
#include <fs/vfs.h>
#include <base/spinlock.h>
#include <device/storage/ata.h>
#include <panic/panic.h>
#include <log/klog.h>
#include <mm/kmalloc.h>
#include <lib/kmemory.h>
#include <lib/kstring.h>

//------------------------------------------------------------------------------
#define LIMIT(a, b) ((a) < (b) ? (a) : (b))

#define FSINFO_HEAD_SIG    0x41615252
#define FSINFO_STRUCT_SIG  0x61417272
#define FSINFO_TAIL_SIG    0xaa550000

#define MBR_PART_OFF  446

#define EXT_FLAG_MIRROR  (1 << 7)
#define EXT_FLAG_ACT     0x000f
#define EXT_FLAG_SECOND  0x0001

#define LFN_HEAD_MSK  0x40
#define LFN_SEQ_MSK   0x1f

#define SFN_FREE  0xe5
#define SFN_LAST  0x00
#define SFN_PAD   0x20

//------------------------------------------------------------------------------
enum
{
  FAT_BUF_DIRTY  = 0x01,
  FAT_INFO_DIRTY = 0x02,
};

enum
{
  CLUST_FREE = 0x01,
  CLUST_USED = 0x02,
  CLUST_LAST = 0x04,
  CLUST_BAD  = 0x08,
};

//------------------------------------------------------------------------------
typedef struct __attribute__((packed))
{
  u8 boot[446];
  struct
  {
    u8 status;
    u8 reserved_0[3];
    u8 type;
    u8 reserved_1[3];
    u32 lba;
    u32 size;
  } part[4];
  u16 sig;
} Mbr;

typedef struct __attribute__((packed))
{
  u8 jump[3];
  char name[8];
  u16 bytes_per_sect;
  u8 sect_per_clust;
  u16 res_sect_cnt;
  u8 fat_cnt;
  u16 root_ent_cnt;
  u16 sect_cnt_16;
  u8 media;
  u16 sect_per_fat_16;
  u16 sect_per_track;
  u16 head_cnt;
  u32 hidden_sect_cnt;
  u32 sect_cnt_32;
  u32 sect_per_fat_32;
  u16 ext_flags;
  u8 minor;
  u8 major;
  u32 root_cluster;
  u16 info_sect;
  u16 copy_bpb_sector;
  u8 reserved_0[12];
  u8 drive_num;
  u8 reserved_1;
  u8 boot_sig;
  u32 volume_id;
  char volume_label[11];
  char fs_type[8];
  u8 reserved_2[420];
  u8 sign[2];
} Bpb;

typedef struct __attribute__((packed))
{
  u32 head_sig;
  u8 reserved_0[480];
  u32 struct_sig;
  u32 free_cnt;
  u32 next_free;
  u8 reserved_1[12];
  u32 tail_sig;
} FsInfo;

typedef struct __attribute__((packed))
{
  u8 name[11];
  u8 attr;
  u8 reserved;
  u8 tenth;
  u16 cre_time;
  u16 cre_date;
  u16 acc_date;
  u16 clust_hi;
  u16 mod_time;
  u16 mod_date;
  u16 clust_lo;
  u32 size;
} Sfn;

typedef union __attribute__((packed))
{
  u8 raw[32];
  struct
  {
    u8 seq;
    u8 name0[10];
    u8 attr;
    u8 type;
    u8 crc;
    u8 name1[12];
    u16 clust;
    u8 name2[4];
  };
} Lfn;

typedef struct
{
  u32 sect;
  u16 idx;
} Loc;

//------------------------------------------------------------------------------
static Fat* g_fat_list;

static u8 g_lfn_indices[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};

static u8 g_buf[512];
static u16 g_len;
static u8 g_crc;


DECLARE_SPINLOCK(fat_lock);

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


//------------------------------------------------------------------------------
static char to_upper(char c)
{
  return (c >= 'a' && c <= 'z') ? c & ~0x20 : c;
}

//------------------------------------------------------------------------------
static bool name_eq(const void* a, const void* b, int len)
{
  const u8* p = a;
  const char* q = b;
  for (int i = 0; i < len; i++)
    if (to_upper(p[i]) != to_upper(q[i]))
      return false;
  return true;
}

//------------------------------------------------------------------------------
static int subpath_len(const char* path)
{
  int i;
  for (i = 0; path[i] && path[i] != '/'; i++);
  return i;
}

//------------------------------------------------------------------------------
static int last_subpath_len(const char* path)
{
  int len = subpath_len(path);
  if (len == 0)
    return 0;

  path += len;

  // Verify last subpath
  while (*path == '/')
    path++;

  return *path ? 0 : len;
}

//------------------------------------------------------------------------------
static u8 get_crc(const u8* name)
{
  u8 sum = 0;
  for (int i = 0; i < 11; i++)
    sum = ((sum & 1) << 7) + (sum >> 1) + name[i];
  return sum;
}

//------------------------------------------------------------------------------
static void decode_timestamp(u16 date, u16 time, Timestamp* ts)
{
  ts->day = date & 0x1f;
  ts->month = (date >> 5) & 0xf;
  ts->year = ((date >> 9) & 0x3f) + 1980;
  ts->hour = (time >> 11) & 0x1f;
  ts->min = (time >> 5) & 0x3f;
  ts->sec = 2 * (time & 0x1f);
}

//------------------------------------------------------------------------------
static void encode_timestamp(u16* date, u16* time)
{
  Timestamp ts;
  fat_get_timestamp(&ts);
  
  *date = ((ts.year - 1980) & 0x3f) << 9 | (ts.month & 0xf) << 5 | (ts.day & 0x1f);
  *time = ((ts.sec / 2) & 0x1f) | (ts.min & 0x3f) << 5 | (ts.hour & 0x1f) << 11;
}

//------------------------------------------------------------------------------
static Fat* find_fat_volume(const char* name, int len)
{
  for (Fat* it = g_fat_list; it; it = it->next)
  {
    if (len == it->name_len && name_eq(name, it->name, len))
      return it;
  }

  return null;
}

//------------------------------------------------------------------------------
static u32 sect_to_clust(Fat* fat, u32 sect)
{
  return ((sect - fat->data_sect) >> fat->clust_shift) + 2;
}

//------------------------------------------------------------------------------
static u32 clust_to_sect(Fat* fat, u32 clust)
{
  return ((clust - 2) << fat->clust_shift) + fat->data_sect;
}

//------------------------------------------------------------------------------
static int sync_buf(Fat* fat)
{
  if (fat->flags & FAT_BUF_DIRTY)
  {
    if (!fat->ops.write(fat->buf, fat->sect))
      return FAT_ERR_IO;

    fat->flags &= ~FAT_BUF_DIRTY;
  }
  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
static int update_buf(Fat* fat, u32 sect)
{
  if (fat->sect != sect)
  {
    int err = sync_buf(fat);
    if (err)
      return err;
    
    if (!fat->ops.read(fat->buf, sect))
      return FAT_ERR_IO;

    fat->sect = sect;
  }

  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
static int sync_fs(Fat* fat)
{
  int err = sync_buf(fat);
  if (err)
    return err;

  if (fat->flags & FAT_INFO_DIRTY)
  {
    err = update_buf(fat, fat->info_sect);
    if (err)
      return err;

    FsInfo* info = (FsInfo*)fat->buf;
    fat->flags |= FAT_BUF_DIRTY;
    info->next_free = fat->last_used;
    info->free_cnt = fat->free_cnt;

    err = sync_buf(fat);
    if (err)
      return err;

    fat->flags &= ~FAT_INFO_DIRTY;
  }

  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
static int get_fat(Fat* fat, u32 clust, u32* out_val, u8* out_flags)
{
  u32* items = (u32*)fat->buf;
  u32 sect = fat->fat_sect[0] + clust / 128; // Active FAT
  u32 idx = clust % 128;

  int err = update_buf(fat, sect);
  if (err)
    return err;

  // Upper nibble is ignored
  u32 val = items[idx] & 0x0fffffff;
  u8 flags;

  if (val == 0)
    flags = CLUST_FREE;
  else if (val == 0x0ffffff7)
    flags = CLUST_BAD;
  else if (val >= 0x0ffffff8)
    flags = CLUST_USED | CLUST_LAST;
  else if (val >= 2 && val < fat->clust_cnt)
    flags = CLUST_USED;
  else
    return FAT_ERR_BROKEN;

  *out_val = val;
  *out_flags = flags;

  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
static int put_fat2(Fat* fat, u32 fat_sect, u32 clust, u32 val)
{
  u32* items = (u32*)fat->buf;
  u32 sect = fat_sect + clust / 128;
  u16 idx = clust % 128;

  int err = update_buf(fat, sect);
  if (err)
    return err;

  // Upper nibble must be preserved
  items[idx] = (items[idx] & 0xf0000000) | (val & 0x0fffffff);
  fat->flags |= FAT_BUF_DIRTY;

  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
static int put_fat(Fat* fat, u32 clust, u32 val)
{
  if (fat->fat_sect[1]) // Mirroring enabled
  {
    int err = put_fat2(fat, fat->fat_sect[1], clust, val);
    if (err)
      return err;
  }

  return put_fat2(fat, fat->fat_sect[0], clust, val);
}

//------------------------------------------------------------------------------
static int remove_chain(Fat* fat, u32 clust)
{
  u8 flags;
  u32 next;

  fat->flags |= FAT_INFO_DIRTY;

  for (;;)
  {
    int err = get_fat(fat, clust, &next, &flags);
    if (err)
      return err;

    if (flags & (CLUST_BAD | CLUST_FREE))
      return FAT_ERR_BROKEN;

    err = put_fat(fat, clust, 0);
    if (err)
      return err;

    fat->free_cnt++;
    clust = next;

    if (flags & CLUST_LAST)
      break;
  }

  return sync_fs(fat);
}

//------------------------------------------------------------------------------
static int stretch_chain(Fat* fat, u32 clust, u32* out_clust)
{
  int err;
  u8 flags;
  u32 next;
  u32 prev = clust;
  bool scan = true;

  fat->flags |= FAT_INFO_DIRTY;

  if (prev)
  {
    // Stretching. Check next cluster.
    if (++clust >= fat->clust_cnt)
      clust = 2;

    err = get_fat(fat, clust, &next, &flags);
    if (err)
      return err;

    if (flags & CLUST_FREE)
      scan = false;
  }

  if (scan)
  {
    clust = fat->last_used;

    for (;;)
    {
      if (++clust >= fat->clust_cnt)
        clust = 2;

      if (clust == fat->last_used)
        return FAT_ERR_FULL;

      err = get_fat(fat, clust, &next, &flags);
      if (err)
        return flags;

      if (flags & CLUST_FREE)
        break;
    }
  }

  err = put_fat(fat, clust, 0x0fffffff); // EOC
  if (err)
    return err;

  if (prev)
  {
    // Stretching. Add link.
    err = put_fat(fat, prev, clust);
    if (err)
      return err;
  }

  fat->last_used = clust;
  fat->free_cnt--;

  *out_clust = clust;
  return sync_fs(fat);
}

//------------------------------------------------------------------------------
static int create_chain(Fat* fat, u32* out_clust)
{
  return stretch_chain(fat, 0, out_clust);
}

//------------------------------------------------------------------------------
static int clust_clear(Fat* fat, u32 clust)
{
  int err = sync_buf(fat);
  if (err)
    return err;
  
  u32 sect = clust_to_sect(fat, clust);
  memset(fat->buf, 0, 512);

  for (int i = 0; i < (1 << fat->clust_shift); i++)
  {
    fat->flags |= FAT_BUF_DIRTY;
    fat->sect = sect++;

    err = sync_buf(fat);
    if (err)
      return err;
  }

  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
static void dir_at_clust(Dir* dir, u32 clust)
{
  dir->clust = clust;
  dir->sect = clust_to_sect(dir->fat, clust);
  dir->idx = 0;
}

//------------------------------------------------------------------------------
static void dir_enter(Dir* dir, u32 clust)
{
  // Cluster is zero for .. entries pointing to root
  if (clust == 0)
    clust = dir->fat->root_clust;

  dir->sclust = clust;
  dir_at_clust(dir, clust);
}

//------------------------------------------------------------------------------
static int dir_next(Dir* dir)
{
  dir->idx += sizeof(Sfn);

  if (dir->idx < 512)
    return FAT_ERR_NONE;

  dir->idx = 0;
  dir->sect++;

  if (dir->sect & dir->fat->clust_msk) // Still in same cluster
    return FAT_ERR_NONE;

  u8 flags;
  u32 next;

  int err = get_fat(dir->fat, dir->clust, &next, &flags);
  if (err)
    return err;

  if (flags & (CLUST_BAD | CLUST_FREE))
    return FAT_ERR_BROKEN;
  
  if (flags & CLUST_LAST) // EOC
    return FAT_ERR_EOF;

  dir_at_clust(dir, next);
  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
static int dir_advance(Dir* dir, int cnt)
{
  for (int i = 0; i < cnt; i++)
  {
    int err = dir_next(dir);
    if (err)
      return err;
  }
  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
static int dir_next_stretch(Dir* dir)
{
  int err = dir_next(dir);
  if (err != FAT_ERR_EOF)
    return err;

  u32 next;
  err = stretch_chain(dir->fat, dir->clust, &next);
  if (err)
    return err;

  dir_at_clust(dir, next);
  return clust_clear(dir->fat, dir->clust);
}

//------------------------------------------------------------------------------
static void* dir_ptr(Dir* dir)
{
  return dir->fat->buf + dir->idx;
}

//------------------------------------------------------------------------------
static bool sfn_is_last(Sfn* sfn)
{
  return sfn->name[0] == SFN_LAST;
}

//------------------------------------------------------------------------------
static bool sfn_is_free(Sfn* sfn)
{
  return sfn->name[0] == SFN_LAST || sfn->name[0] == SFN_FREE;
}

//------------------------------------------------------------------------------
static bool sfn_is_lfn(Sfn* sfn)
{
  return sfn->attr == FAT_ATTR_LFN;
}

//------------------------------------------------------------------------------
static u32 sfn_cluster(Sfn* sfn)
{
  return sfn->clust_hi << 16 | sfn->clust_lo;
}

//------------------------------------------------------------------------------
// Only certain characters are allowed in an SFN file name. Invalid characters 
// are converted to underscore. It does not follow Windows' algorithm, using
// ~N for duplicate names, since it relies on LFN names only.

static char sfn_char(char c)
{
  const char* str = "!#$%&'()-@^_`{}~ "; // Allowed special characters

  c = to_upper(c);
  if (c >= 'A' && c <= 'Z')
    return c;
  if (c >= '0' && c <= '9')
    return c;
  
  for (int i = 0; str[i]; i++)
  {
    if (c == str[i])
      return c;
  }

  return '_';
}

//------------------------------------------------------------------------------
static void put_sfn_name(u8* sfn_name, const char* name, int len)
{
  int i, j;

  for (i = 0; i < LIMIT(len, 8) && name[i] != '.'; i++)
    sfn_name[i] = sfn_char(name[i]);

  for (j = i; j < 8; j++)
    sfn_name[j] = SFN_PAD;

  while (i < len && name[i++] != '.');

  for (j = 0; j < 3 && i < len; j++, i++)
    sfn_name[8 + j] = sfn_char(name[i]);

  for (; j < 3; j++)
    sfn_name[8 + j] = SFN_PAD;
}

//------------------------------------------------------------------------------
static void parse_sfn_name(u8* sfn_name)
{
  u8* ptr = g_buf;

  for (int i = 0; i < 8 && sfn_name[i] != SFN_PAD; i++)
    *ptr++ = (sfn_name[i] >= 'A' && sfn_name[i] <= 'Z') ? sfn_name[i] | 0x20 : sfn_name[i];

  if (sfn_name[8] != SFN_PAD)
    *ptr++ = '.';

  for (int i = 8; i < 11 && sfn_name[i] != SFN_PAD; i++)
    *ptr++ = (sfn_name[i] >= 'A' && sfn_name[i] <= 'Z') ? sfn_name[i] | 0x20 : sfn_name[i];

  g_len = ptr - g_buf;
}

//------------------------------------------------------------------------------
static void put_lfn_name_frag(Lfn* lfn, const char* name, int len)
{
  int i;
  for (i = 0; i < len; i++)
  {
    lfn->raw[g_lfn_indices[i] + 0] = name[i];
    lfn->raw[g_lfn_indices[i] + 1] = 0x00;
  }
  
  if (i < 13)
  {
    lfn->raw[g_lfn_indices[i] + 0] = 0x00;
    lfn->raw[g_lfn_indices[i] + 1] = 0x00;

    while (++i < 13)
    {
      lfn->raw[g_lfn_indices[i] + 0] = 0xff;
      lfn->raw[g_lfn_indices[i] + 1] = 0xff;
    }
  }
}

//------------------------------------------------------------------------------
static int parse_lfn_name(Dir* dir)
{
  int err = update_buf(dir->fat, dir->sect);
  if (err) return err;
  
  Lfn* lfn = dir_ptr(dir);
  g_crc = lfn->crc;
  g_len = 0;

  int cnt = lfn->seq & LFN_SEQ_MSK;
  if (cnt > 20)
    return FAT_ERR_BROKEN;

  while (cnt--)
  {
    if (lfn->attr != FAT_ATTR_LFN || lfn->crc != g_crc)
      return FAT_ERR_BROKEN;

    for (int i = 0; i < 13; i++)
    {
      char c = lfn->raw[g_lfn_indices[i]];

      if (c == 0xff)
        return FAT_ERR_BROKEN; // 0x00 must be first

      if (c == 0x00)
        break;

      g_buf[13 * cnt + i] = c;
      g_len++;
    }

    err = dir_next(dir);
    if (err)
      return err;

    err = update_buf(dir->fat, dir->sect);
    if (err)
      return err;

    lfn = dir_ptr(dir);
  }

  return g_len <= 255 ? FAT_ERR_NONE : FAT_ERR_BROKEN;
}

//------------------------------------------------------------------------------
static int dir_search(Dir* dir, const char* name, int len, Loc* loc)
{
  // Should it support matching SFN?
  u8 sfn_name[11];
  put_sfn_name(sfn_name, name, len);

  dir_at_clust(dir, dir->sclust);

  for (int err = 0;; err = dir_next(dir))
  {
    if (err)
      return err;

    err = update_buf(dir->fat, dir->sect);
    if (err)
      return err;

    Sfn* sfn = dir_ptr(dir);

    if (sfn_is_last(sfn))
      return FAT_ERR_EOF;
    
    if (sfn_is_free(sfn))
      continue;

    if (loc)
    {
      // Update the start location (SFN or first LFN). Used when removing entries.
      loc->sect = dir->sect;
      loc->idx = dir->idx;
    }

    if (sfn_is_lfn(sfn))
    {
      err = parse_lfn_name(dir);
      if (err)
        return err;

      sfn = dir_ptr(dir);

      if (sfn_is_free(sfn) || sfn_is_lfn(sfn) || g_crc != get_crc(sfn->name))
        return FAT_ERR_BROKEN;

      if (g_len == len && name_eq(g_buf, name, len))
        return FAT_ERR_NONE;
    }
    else
    {
      if (!memcmp(sfn_name, sfn->name, sizeof(sfn_name)))
        return FAT_ERR_NONE;
    }
  }
}

//------------------------------------------------------------------------------
static bool dir_at_root(Dir* dir)
{
  return dir->clust == dir->fat->root_clust &&
    dir->sect == clust_to_sect(dir->fat, dir->fat->root_clust) && 
    dir->idx == 0;
}

//------------------------------------------------------------------------------
static int follow_path(Dir* dir, const char** path, Loc* loc)
{
  int err, len;
  u32 dir_clust;
  bool dir_enterable;
  const char* str = *path;

  if (*str++ != '/')
    return FAT_ERR_PATH;
  len = subpath_len(str);
  if (len == 0)
    return FAT_ERR_PATH;

  dir->fat = find_fat_volume(str, len);
  if (!dir->fat)
    return FAT_ERR_PATH;

  // Enter root by default (no entry points to it)
  dir_enter(dir, dir->fat->root_clust);
  dir_clust = dir->clust;
  dir_enterable = true;

  str += len;
  *path = str;

  for (;;)
  {
    while (*str == '/')
      str++;
    *path = str;

    len = subpath_len(str);
    if (len == 0)
      return FAT_ERR_NONE; // Do not enter directory. Dir points to the SFN of path.

    if (!dir_enterable)
      return FAT_ERR_PATH;

    dir_enter(dir, dir_clust);

    err = dir_search(dir, str, len, loc);
    if (err)
      return err;

    str += len;
    *path = str;

    Sfn* sfn = dir_ptr(dir);
    dir_clust = sfn_cluster(sfn);
    dir_enterable = (sfn->attr & FAT_ATTR_DIR) != 0;
  }
}

//------------------------------------------------------------------------------
static int remove_entries(Dir* dir, Loc* loc)
{
  // Save dir location (last entry to delete)
  u32 sect = dir->sect;
  u16 idx = dir->idx;

  // Rewind dir to loc (first entry to delete)
  dir->clust = sect_to_clust(dir->fat, loc->sect);
  dir->sect = loc->sect;
  dir->idx = loc->idx;

  for (;;)
  {
    int err = update_buf(dir->fat, dir->sect);
    if (err)
      return err;

    Sfn* sfn = dir_ptr(dir);
    sfn->name[0] = SFN_FREE;
    dir->fat->flags |= FAT_BUF_DIRTY;

    if (dir->sect == sect && dir->idx == idx)
      return FAT_ERR_NONE;

    err = dir_next(dir);
    if (err)
      return err;
  }
}

//------------------------------------------------------------------------------
static int dir_add(Dir* dir, const char* name, int len, u8 attr, u32 clust)
{
  if (len <= 0 || len > 255)
    return FAT_ERR_PARAM;

  int err;
  bool eod = false;
  u16 idx = 0;
  u32 sect = 0;
  int lfns = (len + 12) / 13;

  dir_enter(dir, dir->sclust);

  // Try to find lfn_cnt + 1 consecutive free entries. Stretch cluster chain
  // if necessary. Store location of first entry in the sequence.
  for (int cnt = 0; cnt < lfns + 1;)
  {
    err = update_buf(dir->fat, dir->sect);
    if (err)
      return err;

    Sfn* sfn = dir_ptr(dir);
    if (eod || sfn_is_free(sfn))
    {
      if (cnt++ == 0)
      {
        sect = dir->sect;
        idx = dir->idx;
      }
    }
    else
      cnt = 0;

    if (sfn_is_last(sfn))
      eod = true;

    err = dir_next_stretch(dir);
    if (err)
      return err;
  }

  if (eod)
  {
    // We are currently at the entry after the SFN we will create.
    // Since it hit EOD the entry is free. Create new EOD.
    err = update_buf(dir->fat, dir->sect);
    if (err)
      return err;
    
    Sfn* sfn = dir_ptr(dir);
    sfn->name[0] = 0x00;
    dir->fat->flags |= FAT_BUF_DIRTY;
  }

  // Rewind to the first free entry
  dir->clust = sect_to_clust(dir->fat, sect);
  dir->sect = sect;
  dir->idx = idx;

  u8 sfn_name[11];
  put_sfn_name(sfn_name, name, len);

  u8 crc = get_crc(sfn_name);
  u8 mask = LFN_HEAD_MSK;

  // Create LFN entries
  for (int i = lfns; i > 0; i--, mask = 0)
  {
    err = update_buf(dir->fat, dir->sect);
    if (err)
      return err;
    
    Lfn* lfn = dir_ptr(dir);
    dir->fat->flags |= FAT_BUF_DIRTY;

    int pos = 13 * (i - 1);
    put_lfn_name_frag(lfn, name + pos, LIMIT(len - pos, 13));
    lfn->attr = FAT_ATTR_LFN;
    lfn->seq = mask | i;
    lfn->crc = crc;
    lfn->type = 0;
    lfn->clust = 0;
    
    err = dir_next(dir);
    if (err)
      return err;
  }

  u16 time, date;
  encode_timestamp(&date, &time);

  err = update_buf(dir->fat, dir->sect);
  if (err)
    return err;

  Sfn* sfn = dir_ptr(dir);
  dir->fat->flags |= FAT_BUF_DIRTY;

  memcpy(sfn->name, sfn_name, sizeof(sfn->name));
  sfn->clust_hi = clust >> 16;
  sfn->clust_lo = clust & 0xffff;
  sfn->attr = attr;
  sfn->reserved = 0;
  sfn->tenth = 0;
  sfn->cre_time = time;
  sfn->mod_time = time;
  sfn->cre_date = date;
  sfn->mod_date = date;
  sfn->acc_date = date;
  sfn->size = 0;

  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
static bool get_part_lba(u8* buf, int partition, u32* lba)
{
  Mbr* mbr = (Mbr*)buf;

  if (mbr->sig != 0xaa55)
    return false;

  if (mbr->part[partition].type != 0x0c) // Must be FAT32
    return false;
  
  *lba = mbr->part[partition].lba;
  return true;
}

//------------------------------------------------------------------------------
static bool check_fat(u8* buf)
{
  Bpb* bpb = (Bpb*)buf;
  
  if (bpb->jump[0] != 0xeb && bpb->jump[0] != 0xe9)
    return false;

  // Check if we need to be this strict.
  if (bpb->fat_cnt != 2)
    return false;
  
  if (bpb->root_ent_cnt || bpb->sect_cnt_16 || bpb->sect_per_fat_16)
    return false;
  
  if (bpb->info_sect != 1)
    return false;

  if (memcmp(bpb->fs_type, "FAT32   ", 8))
    return false;
    
  if (bpb->bytes_per_sect != 512)
    return false;
  
  // Only two FAT tables should exist
  if (!(bpb->ext_flags & EXT_FLAG_MIRROR) && (bpb->ext_flags & EXT_FLAG_ACT) > 1)
    return false;
  
  // FAT type is determined from the count of clusters
  u32 sect_cnt = bpb->sect_cnt_32 - (bpb->res_sect_cnt + bpb->fat_cnt * bpb->sect_per_fat_32);
  return sect_cnt/ bpb->sect_per_clust >= 65525;
}

//------------------------------------------------------------------------------
int probe(DiskOps* ops, int partition, u32* lba)
{
  *lba = 0;
  if (!ops->read(g_buf, *lba))
    return FAT_ERR_IO;

  if (check_fat(g_buf))
    return partition == 0 ? FAT_ERR_NONE : FAT_ERR_NOFAT;

  if (!get_part_lba(g_buf, partition, lba))
    return FAT_ERR_NOFAT;
  
  if (!ops->read(g_buf, *lba))
    return FAT_ERR_IO;
  
  return check_fat(g_buf) ? FAT_ERR_NONE : FAT_ERR_NOFAT;
}

//------------------------------------------------------------------------------
// Probes a partition on the drive for a FAT32 file system. 
//
// Partition 0 returns success when either:
//  - the entire drive is formatted FAT32
//  - the drive contain an MBR with partition 0 formatted FAT32
//
// Partition 1 to 3 return success when:
//  - the drive contain an MBR with partition 1 to 3 formatted FAT32

int fat_probe(DiskOps* ops, int partition)
{
  u32 lba;
  return probe(ops, partition, &lba);
}

//------------------------------------------------------------------------------
// Mounts a file system. The name specifies which path is used to access it.
// For example: mounting using 'mnt', and accessing using '/mnt/path/file.txt'.
// Partition 0 referes to either the entire disk (absense of MBR), or to the 
// specified MBR partition.

int fat_mount(DiskOps* ops, int partition, Fat* fat, const char* name)
{
  u32 lba;
  int err = probe(ops, partition, &lba);
  if (err)
    return err;

  // Global buffer contains BPB when probe succeeds
  Bpb* bpb = (Bpb*)g_buf;

  bool mirror    = (bpb->ext_flags & EXT_FLAG_MIRROR) != 0;
  bool use_first = (bpb->ext_flags & EXT_FLAG_SECOND) == 0;

  u32 fat_0 = lba + bpb->res_sect_cnt;
  u32 fat_1 = lba + bpb->res_sect_cnt + bpb->sect_per_fat_32;
  
  fat->clust_shift = __builtin_ctz(bpb->sect_per_clust);
  fat->clust_msk = bpb->sect_per_clust - 1;
  fat->clust_cnt = bpb->sect_per_fat_32 * 128;
  fat->root_clust = bpb->root_cluster;
  fat->fat_sect[0] = use_first ? fat_0 : fat_1;
  fat->fat_sect[1] = mirror ? (use_first ? fat_1 : fat_0) : 0;
  fat->info_sect = lba + bpb->info_sect;
  fat->data_sect = lba + bpb->res_sect_cnt + bpb->fat_cnt * bpb->sect_per_fat_32;

  // Load FsInfo
  if (!ops->read(g_buf, fat->info_sect))
    return FAT_ERR_IO;

  FsInfo* info = (FsInfo*)g_buf;
  if (info->tail_sig != FSINFO_TAIL_SIG || 
      info->head_sig != FSINFO_HEAD_SIG ||
      info->struct_sig != FSINFO_STRUCT_SIG ||
      info->next_free == 0xffffffff || 
      info->free_cnt == 0xffffffff)
    return FAT_ERR_NOFAT;
  
  fat->last_used = info->next_free;
  fat->free_cnt  = info->free_cnt;

  int name_len = strlen(name);
  if (name_len > sizeof(fat->name))
    return FAT_ERR_PARAM;
  memcpy(fat->name, name, name_len);
  fat->name_len = name_len;

  fat->ops = *ops;
  fat->sect = 0;   // Causes buffering on first call

  fat->next = g_fat_list;
  g_fat_list = fat;

  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
// Syncronizes unwritten changes and removes the fat from the global list. All
// file must be closed before calling this.

int fat_umount(Fat* fat)
{
  Fat** it = &g_fat_list;
  while (*it && *it != fat)
    it = &(*it)->next;

  if (*it == null)
    return FAT_ERR_PARAM;

  *it = fat->next;
  return sync_fs(fat);
}

//------------------------------------------------------------------------------
// Synchronizes unwritten changes. Does not synchronize open files.

int fat_sync(Fat* fat)
{
  return sync_fs(fat);
}

//------------------------------------------------------------------------------
// Get information about a file or directory.

int fat_stat(const char* path, DirInfo* info)
{
  Dir dir;
  Loc loc;
  int err = follow_path(&dir, &path, &loc);
  if (err)
    return err;
  
  int len = subpath_len(path);
  if (len)
    return FAT_ERR_PATH;
  
  dir.clust = sect_to_clust(dir.fat, loc.sect);
  dir.sect = loc.sect;
  dir.idx = loc.idx;

  return fat_dir_read(&dir, info);
}

//------------------------------------------------------------------------------
// Unlinks (deletes) an existing file or empty directory.

int fat_unlink(const char* path)
{
  Dir dir;
  Loc loc;
  int err = follow_path(&dir, &path, &loc);
  if (err)
    return err;

  if (dir_at_root(&dir))
    return FAT_ERR_DENIED;

  Sfn* sfn = dir_ptr(&dir);
  u32 clust = sfn_cluster(sfn);

  if (sfn->attr & (FAT_ATTR_RO | FAT_ATTR_SYS | FAT_ATTR_LABEL))
    return FAT_ERR_DENIED;

  if (sfn->attr & FAT_ATTR_DIR)
  {
    // Make sure the directory is empty
    Dir tmp = dir;
    dir_enter(&tmp, sfn_cluster(sfn));

    err = dir_advance(&tmp, 2); // . and ..
    if (err)
      return err;
    
    for (;;)
    {
      err = update_buf(tmp.fat, tmp.sect);
      if (err)
        return err;

      sfn = dir_ptr(&tmp);
      
      if (sfn_is_last(sfn))
        break;

      if (!sfn_is_free(sfn))
        return FAT_ERR_DENIED;

      err = dir_next(&tmp);
      if (err)
        return err;
    }
  }

  // Delete clusters
  err = remove_chain(dir.fat, clust);
  if (err)
    return err;

  err = remove_entries(&dir, &loc);
  if (err)
    return err;

  return sync_fs(dir.fat);
}

//------------------------------------------------------------------------------
// Opens a file. The file structure contain the size and offset that can be read
// by the user at any point. Any combination of the following flags can be used:
//
//  - FAT_WRITE:   open for writing
//  - FAT_READ:    open for reading
//  - FAT_APPEND:  place file cursor at the end of the file
//  - FAT_TRUNC:   truncate the file
//  - FAT_CREATE:  create file if not existing

int fat_file_open(File* file, const char* path, u8 flags)
{
  Dir dir;
  dir.fat = 0;

  int err = follow_path(&dir, &path, null);
  if (err && err != FAT_ERR_EOF)
    return err;
  
  if (err == FAT_ERR_EOF) // File does not exist
  {
    if (0 == (flags & FAT_CREATE))
      return FAT_ERR_DENIED;
    
    int len = last_subpath_len(path);
    if (len == 0)
      return FAT_ERR_PATH;

    // Create a new file
    u32 clust;
    err = create_chain(dir.fat, &clust);
    if (err)
      return err;
    
    err = dir_add(&dir, path, len, FAT_ATTR_ARCHIVE, clust);
    if (err)
      return err;
  }

  Sfn* sfn = dir_ptr(&dir);

  file->fat = dir.fat;
  file->dir_sect = dir.sect;
  file->dir_idx = dir.idx;
  file->sclust = sfn_cluster(sfn);
  file->clust = file->sclust;
  file->sect = 0xffffffff;
  file->offset = 0;
  file->attr = sfn->attr;
  file->size = sfn->size;
  file->flags = flags;

  if (file->size && flags & FAT_TRUNC)
  {
    file->size = 0;
    file->flags |= FAT_MODIFIED;
  }

  return fat_file_seek(file, 0, (flags & FAT_APPEND) ? FAT_SEEK_END : FAT_SEEK_START);
}

//------------------------------------------------------------------------------
// Closes a file. Updates the directory entry if modified. Writes back the write 
// buffer if dirty.

int fat_file_close(File* file)
{
  if (!file->fat)
    return FAT_ERR_PARAM;
  return fat_file_sync(file);
}

//------------------------------------------------------------------------------
int fat_file_read(File* file, void* buf, int len, int* bytes)
{
  *bytes = 0;
  u8* dst = buf;

  if (!file->fat)
    return FAT_ERR_PARAM;

  if (0 == (file->flags & FAT_READ))
    return FAT_ERR_DENIED;
  
  file->flags |= FAT_ACCESSED;

  while (len && file->offset < file->size)
  {
    int idx = file->offset % 512;
    int cnt = LIMIT(len, LIMIT(512 - idx, file->size - file->offset));
    memcpy(dst, file->buf + idx, cnt);

    *bytes += cnt;
    dst += cnt;
    len -= cnt;

    int err = fat_file_seek(file, cnt, FAT_SEEK_CURR);
    if (err)
      return err;
  }

  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
// Write a number of bytes to the file. It allocates more clusters if the write
// exceeds the allocated space. It return the error code and the number of bytes
// written.

int fat_file_write(File* file, const void* buf, int len, int* bytes)
{
  int err = FAT_ERR_NONE;
  const u8* src = buf;
  *bytes = 0;

  if (!file->fat)
    return FAT_ERR_PARAM;

  if (0 == (file->flags & FAT_WRITE))
    return FAT_ERR_DENIED;
  
  file->flags |= FAT_MODIFIED | FAT_ACCESSED;

  while (len)
  {
    int idx = file->offset % 512;
    int cnt = LIMIT(len, 512 - idx);
    memcpy(file->buf + idx, src, cnt);
    file->flags |= FAT_FILE_DIRTY;

    *bytes += cnt;
    src += cnt;
    len -= cnt;

    int err = fat_file_seek(file, cnt, FAT_SEEK_CURR);
    if (err)
      break;
  }

  if (file->offset > file->size)
    file->size = file->offset;

  return err;
}

//------------------------------------------------------------------------------
// Seek into the file. This is internally used to update the file buffer and extend
// the file when needed. For a user, this is used to ether:
//
//  - Update the offset of subsequent reads and writes
//  - Preallocate space in the file (just seek the number of bytes to allocate)
// 
// Seeking backwards take more time as the cluster chain (often) must be followed 
// from the beginning.

int fat_file_seek(File* file, int offset, int seek)
{
  u32 ssect = file->sect;
  i64 off64 = 0;

  if (!file->fat)
    return FAT_ERR_PARAM;

  switch (seek)
  {
    case FAT_SEEK_START:
      off64 = 0;
      break;
    case FAT_SEEK_CURR:
      off64 = file->offset;
      break;
    case FAT_SEEK_END:
      off64 = file->size;
      break;
  }

  off64 += offset;
  if (off64 < 0 || off64 > 0xffffffff)
    return FAT_ERR_EOF;

  u32 off = (u32)off64;
  u32 clust_size = 512 << file->fat->clust_shift;
  u32 dst_clust = off / clust_size;
  u32 src_clust = file->offset / clust_size;

  if (dst_clust < src_clust)
  {
    // Backtracking not possible. Start scan from the beginning.
    file->clust = file->sclust;
    file->sect = clust_to_sect(file->fat, file->sclust);
    file->offset = 0;
    src_clust = 0;
  }

  // Follow the cluster chain. Expand when EOF.
  for (int i = 0; i < dst_clust - src_clust; i++)
  {
    u32 next;
    u8 flags;
    int err = get_fat(file->fat, file->clust, &next, &flags);
    if (err)
      return err;

    if (flags & (CLUST_BAD | CLUST_FREE))
      return FAT_ERR_BROKEN;

    if (flags & CLUST_LAST)
    {
      err = stretch_chain(file->fat, file->clust, &next);
      if (err)
        return err;
    }
    
    file->clust = next;
  }

  file->sect = clust_to_sect(file->fat, file->clust) + ((off / 512) & file->fat->clust_msk);
  file->offset = off;

  // Update file buffer when moving to new sector
  if (file->sect != ssect)
  {
    if (file->flags & FAT_FILE_DIRTY)
    {
      if (!file->fat->ops.write(file->buf, ssect))
        return FAT_ERR_IO;
      file->flags &= ~FAT_FILE_DIRTY;
    }

    if (!file->fat->ops.read(file->buf, file->sect))
      return FAT_ERR_IO;
  }
  
  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
// Synchronizes a file. Writes back dirty file data. Updates directory timestamp
// when accessed. Update directory size and timestamp when modified.

int fat_file_sync(File* file)
{
  int err;
  if (!file->fat)
    return FAT_ERR_PARAM;
  
  if (!file->fat->ops.write(file->buf, file->sect))
    return FAT_ERR_IO;

  if (file->flags & (FAT_ACCESSED | FAT_MODIFIED))
  {
    err = update_buf(file->fat, file->dir_sect);
    if (err)
      return err;
    
    Sfn* sfn = (Sfn*)(file->fat->buf + file->dir_idx);
    file->fat->flags |= FAT_BUF_DIRTY;

    u16 date, time;
    encode_timestamp(&date, &time);
    
    if (file->flags & FAT_ACCESSED)
      sfn->acc_date = date;

    if (file->flags & FAT_MODIFIED)
    {
      sfn->attr |= FAT_ATTR_ARCHIVE;
      sfn->size = file->size;
      sfn->mod_date = date;
      sfn->mod_time = time;
    }
  }

  err = sync_fs(file->fat);
  if (err)
    return err;

  file->flags &= ~(FAT_ACCESSED | FAT_MODIFIED);
  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
// Creates and enter a directory. Don't know if there is any point in returning dir.

int fat_dir_create(Dir* dir, const char* path)
{
  int err = follow_path(dir, &path, null);
  if (err != FAT_ERR_EOF)
    return err;
  
  int len = last_subpath_len(path);
  if (len == 0)
    return FAT_ERR_PATH;

  // Create a new directory
  u32 clust;
  err = create_chain(dir->fat, &clust);
  if (err)
    return err;

  // Empty directory
  err = clust_clear(dir->fat, clust);
  if (err)
    return err;

  u16 date, time;
  encode_timestamp(&date, &time);

  err = update_buf(dir->fat, clust_to_sect(dir->fat, clust));
  if (err)
    return err;

  Sfn* sfn = (Sfn*)dir->fat->buf;
  dir->fat->flags |= FAT_BUF_DIRTY;

  memset(sfn[0].name, ' ', sizeof(sfn->name));
  sfn[0].name[0] = '.';
  sfn[0].attr = FAT_ATTR_DIR;
  sfn[0].clust_hi = clust >> 16;
  sfn[0].clust_lo = clust & 0xffff;
  sfn[0].cre_date = date;
  sfn[0].cre_time = time;
  sfn[0].mod_date = date;
  sfn[0].mod_time = time;
  sfn[0].acc_date = date;

  u32 parent = dir->sclust;
  if (parent == dir->fat->root_clust)
    parent = 0;
  
  memcpy(sfn + 1, sfn, sizeof(Sfn));
  sfn[1].name[1] = '.';
  sfn[1].clust_hi = parent >> 16;
  sfn[1].clust_lo = parent & 0xffff;

  err = dir_add(dir, path, len, FAT_ATTR_DIR, clust);
  if (err)
    return err;
  
  dir_enter(dir, clust);
  return sync_fs(dir->fat);
}

//------------------------------------------------------------------------------
int fat_dir_open(Dir* dir, const char* path)
{
  int err = follow_path(dir, &path, null);
  if (err)
    return err;
  
  if (dir_at_root(dir))
    return FAT_ERR_NONE;

  // Dir points to the directory SFN. Enter the directory.
  err = update_buf(dir->fat, dir->sect);
  if (err)
    return err;

  Sfn* sfn = dir_ptr(dir);
  if (0 == (sfn->attr & FAT_ATTR_DIR))
    return FAT_ERR_PATH;

  dir_enter(dir, sfn_cluster(sfn));
  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
// Read directory entry pointed to by dir. Use dir_next to advance directory pointer.

int fat_dir_read(Dir* dir, DirInfo* info)
{
  if (!dir->fat)
    return FAT_ERR_PARAM;

  for (int err = 0;; err = dir_next(dir)) // Hack to allow continue
  {
    if (err)
      return err;

    err = update_buf(dir->fat, dir->sect);
    if (err)
      return err;

    Sfn* sfn = dir_ptr(dir);

    if (sfn_is_last(sfn))
      return FAT_ERR_EOF;

    if (sfn_is_free(sfn))
      continue;

    if (sfn_is_lfn(sfn))
    {
      err = parse_lfn_name(dir);
      if (err)
        return err;

      // Following entry must be SFN
      err = update_buf(dir->fat, dir->sect);
      if (err)
        return err;
      
      sfn = dir_ptr(dir);
      if (sfn_is_free(sfn) || g_crc != get_crc(sfn->name))
        return FAT_ERR_BROKEN;
    }
    else
      parse_sfn_name(sfn->name);

    // Parsed filename (SFN or LFN) are in global buffer
    memcpy(info->name, g_buf, g_len);
    info->name_len = g_len;

    decode_timestamp(sfn->cre_date, sfn->cre_time, &info->created);
    decode_timestamp(sfn->mod_date, sfn->mod_time, &info->modified);

    info->size = sfn->size;
    info->attr = sfn->attr;

    return FAT_ERR_NONE;
  }
}

//------------------------------------------------------------------------------
// Advances the directory pointer. Returns EOF when the EOF marker is hit. The 
// user should not call this after that point. Call rewind to reset the directory
// pointer to the beginning.

int fat_dir_next(Dir* dir)
{
  if (!dir->fat)
    return FAT_ERR_PARAM;

  return dir_next(dir);
}

//------------------------------------------------------------------------------
// Sets dir to point to the first entry in the directory.
int fat_dir_rewind(Dir* dir)
{
  if (!dir->fat)
    return FAT_ERR_PARAM;
  dir_at_clust(dir, dir->sclust);
  return FAT_ERR_NONE;
}

//------------------------------------------------------------------------------
const char* fat_get_error(int err)
{
  static const char* strs[] =
  {
    "FAT_ERR_NONE",
    "FAT_ERR_NOFAT",
    "FAT_ERR_BROKEN",
    "FAT_ERR_IO",
    "FAT_ERR_PARAM",
    "FAT_ERR_PATH",
    "FAT_ERR_EOF",
    "FAT_ERR_DENIED",
    "FAT_ERR_FULL",
  };

  err = -err;
  if (err < 0 || err >= sizeof(strs) / sizeof(*strs))
    return "null";
  return strs[err];
}

//------------------------------------------------------------------------------
// Override this function if it is needed. It is declared weak.

__attribute__((weak)) void fat_get_timestamp(Timestamp* ts)
{
  ts->day   = 1;
  ts->month = 1;
  ts->year  = 1980;
  ts->hour  = 0;
  ts->min   = 0;
  ts->sec   = 0;
}

//
// VFS
//

static bool fat32_disk_read(u8* buf, u32 sect) {
  ata_pio_read28(null, 1, sect, buf);
  return true;
}

static bool fat32_disk_write(const u8* buf, u32 sect) {
  ata_pio_write28(null, 1, sect, (u8*)buf);
  return true;
}

static int fat32_active_part = -1;

void fat32_init() {
  u8 mbr_buf[512];
  ata_pio_read28(null, 1, 0, mbr_buf);

  Mbr* mbr = (Mbr*)mbr_buf;
  if (mbr->sig == 0xaa55) {
    for (int i = 0; i < 4; i++) {
      if (mbr->part[i].type == 0x0c || mbr->part[i].type == 0x0b) {
        fat32_active_part = i;
        break;
      }
    }
  }

  vfs_register_fs(&fat32_fs);
  klogi("FAT32 Initialized.\n");
}

vfs_super_block_t *fat32_mksb(vfs_fs_t *fs) {
  DiskOps ops;
  ops.read = fat32_disk_read;
  ops.write = fat32_disk_write;

  Fat* fat = kzalloc(sizeof(Fat));

  int part = fat32_active_part >= 0 ? fat32_active_part : 0;
  int err = fat_mount(&ops, part, fat, "root");
  if (err) {
    kloge("[FAT32] fat_mount failed: %s\n", fat_get_error(err));
    kfree(fat);
    return null;
  }

  klogi("[FAT32] mounted: root_clust=%d data_sect=%d clust_cnt=%d sect_per_clust=%d\n",
        fat->root_clust, fat->data_sect, fat->clust_cnt, 1 << fat->clust_shift);

  vfs_super_block_t *sb = kzalloc(sizeof(vfs_super_block_t));
  sb->s_fs = fs;
  sb->s_ops = null;
  sb->s_pdata = fat;

  vfs_dentry_t *dentry = kzalloc(sizeof(vfs_dentry_t));
  dentry->d_ops = &fat32_dentry_ops;
  dentry->d_parent = dentry;
  dentry->d_name[0] = '/';
  dlist_init(&dentry->d_subdirs);
  sb->s_root = dentry;

  vfs_inode_t *inode = kzalloc(sizeof(vfs_inode_t));
  inode->i_fsize = 0;
  inode->i_mountpoint = null;
  inode->i_ops = &fat32_inode_ops;
  inode->i_sb = sb;
  inode->i_type = VFS_NODE_DIRECTOR;
  inode->i_fops = &fat32_file_ops;
  dentry->d_inode = inode;

  fat32_inode_data_t *idata = kzalloc(sizeof(fat32_inode_data_t));
  idata->fat = fat;
  idata->start_clus = fat->root_clust;
  idata->is_root = true;
  idata->attr = FAT_ATTR_DIR;
  inode->i_pdata = idata;

  return sb;
}

vfs_dentry_t *fat32_lookup(vfs_inode_t *this, vfs_dentry_t *expect_dentry) {
  spin_lock(&fat_lock);

  fat32_inode_data_t *parent_data = this->i_pdata;
  if (!(parent_data->attr & FAT_ATTR_DIR)) {
    spin_unlock(&fat_lock);
    return null;
  }

  Dir dir;
  dir.fat = parent_data->fat;
  dir.sclust = parent_data->start_clus;

  Loc loc;
  char* name = expect_dentry->d_name;
  int name_len = strlen(name);

  int err = dir_search(&dir, name, name_len, &loc);
  if (err) {
    spin_unlock(&fat_lock);
    return null;
  }

  err = update_buf(dir.fat, dir.sect);
  if (err) {
    spin_unlock(&fat_lock);
    return null;
  }

  Sfn* sfn = dir_ptr(&dir);

  vfs_inode_t *inode = kzalloc(sizeof(vfs_inode_t));
  inode->i_sb = this->i_sb;
  inode->i_ops = &fat32_inode_ops;
  inode->i_fops = &fat32_file_ops;

  if (sfn->attr & FAT_ATTR_DIR) {
    inode->i_type = VFS_NODE_DIRECTOR;
    inode->i_fsize = 0;
  } else {
    inode->i_type = VFS_NODE_FILE;
    inode->i_fsize = sfn->size;
  }

  fat32_inode_data_t *idata = kzalloc(sizeof(fat32_inode_data_t));
  idata->fat = parent_data->fat;
  idata->start_clus = sfn_cluster(sfn);
  idata->file_size = sfn->size;
  idata->attr = sfn->attr;
  idata->is_root = false;
  idata->dir_sect = dir.sect;
  idata->dir_idx = dir.idx;
  inode->i_pdata = idata;

  expect_dentry->d_inode = inode;
  expect_dentry->d_ops = &fat32_dentry_ops;

  spin_unlock(&fat_lock);
  return expect_dentry;
}

i32 fat32_open(vfs_inode_t *this, vfs_file_t *filep) {
  fat32_inode_data_t *idata = this->i_pdata;

  File* f = kzalloc(sizeof(File));
  f->fat = idata->fat;
  f->dir_sect = idata->dir_sect;
  f->dir_idx = idata->dir_idx;
  f->sclust = idata->start_clus;
  f->clust = f->sclust;
  f->size = idata->file_size;
  f->attr = idata->attr;
  f->flags = FAT_READ;
  if (filep->f_openmode == VFS_MODE_WRITE || filep->f_openmode == VFS_MODE_READWRITE)
    f->flags |= FAT_WRITE;
  f->sect = 0xffffffff;
  f->offset = 0;

  spin_lock(&fat_lock);
  fat_file_seek(f, 0, FAT_SEEK_START);
  spin_unlock(&fat_lock);
  filep->f_pos = f->offset;

  filep->f_pdata = f;
  return 0;
}

i32 fat32_close(vfs_inode_t *this, vfs_file_t *filep) {
  File* f = filep->f_pdata;
  if (!f) return 0;

  spin_lock(&fat_lock);
  fat_file_sync(f);
  kfree(f);
  spin_unlock(&fat_lock);

  filep->f_pdata = null;
  return 0;
}

i32 fat32_read(vfs_inode_t *this, vfs_file_t *filep, i32 len, char *buffer) {
  File* f = filep->f_pdata;
  if (!f) return -1;
  if (len <= 0) return 0;

  spin_lock(&fat_lock);
  int bytes;
  int err = fat_file_read(f, buffer, len, &bytes);
  filep->f_pos = f->offset;
  spin_unlock(&fat_lock);

  return err ? -1 : 0;
}

i32 fat32_write(vfs_inode_t *this, vfs_file_t *filep, i32 len, const char *buffer) {
  File* f = filep->f_pdata;
  if (!f) return -1;
  if (len <= 0) return 0;

  spin_lock(&fat_lock);
  int bytes;
  int err = fat_file_write(f, buffer, len, &bytes);
  filep->f_pos = f->offset;
  this->i_fsize = f->size;
  spin_unlock(&fat_lock);

  return err ? -1 : 0;
}

i32 fat32_lseek(vfs_inode_t *this, vfs_file_t *filep, i32 offset, i32 wence) {
  File* f = filep->f_pdata;
  if (!f) return -1;

  int seek;
  switch (wence) {
    case SEEK_SET: seek = FAT_SEEK_START; break;
    case SEEK_CUR: seek = FAT_SEEK_CURR; break;
    case SEEK_END: seek = FAT_SEEK_END; break;
    default: return -1;
  }

  spin_lock(&fat_lock);
  int err = fat_file_seek(f, offset, seek);
  filep->f_pos = f->offset;
  spin_unlock(&fat_lock);

  return err ? -1 : (i32)filep->f_pos;
}

i32 fat32_iterate(vfs_inode_t *this, vfs_file_t *filep, char *path, i32 *filecnt, vfs_dirent_t **dirent) {
  fat32_inode_data_t *idata = this->i_pdata;
  if (!(idata->attr & FAT_ATTR_DIR)) return -1;

  spin_lock(&fat_lock);

  Dir dir;
  int err;
  if (path && path[0]) {
    err = fat_dir_open(&dir, path);
  } else if (idata->is_root) {
    err = fat_dir_open(&dir, "/root");
  } else {
    dir.fat = idata->fat;
    dir_enter(&dir, idata->start_clus);
    err = 0;
  }
  if (err) {
    spin_unlock(&fat_lock);
    return -1;
  }

  vfs_dirent_t *entries = null;
  int count = 0;

  for (;;) {
    DirInfo info;
    err = fat_dir_read(&dir, &info);
    if (err) break;

    fat_dir_next(&dir);

    if (info.name_len == 1 && info.name[0] == '.') continue;
    if (info.name_len == 2 && info.name[0] == '.' && info.name[1] == '.') continue;

    entries = krealloc(entries, count * sizeof(vfs_dirent_t), (count + 1) * sizeof(vfs_dirent_t));
    memcpy(entries[count].name, info.name, info.name_len);
    entries[count].name[info.name_len] = '\0';
    entries[count].sz = info.size;
    entries[count].type = (info.attr & FAT_ATTR_DIR) ? VFS_NODE_DIRECTOR : VFS_NODE_FILE;
    count++;
  }

  spin_unlock(&fat_lock);

  *dirent = entries;
  *filecnt = count;
  return count;
}