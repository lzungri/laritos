#define DEBUG
#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fs/ext2.h>
#include <dstruct/list.h>
#include <fs/vfs/types.h>
#include <fs/vfs/core.h>
#include <module/core.h>
#include <mm/heap.h>
#include <utils/math.h>
#include <utils/utils.h>
#include <time/timeconv.h>
#include <time/core.h>
#include <dstruct/bitset.h>
#include <fs/stat.h>

// TODO: This is hardcoded until we append the data.img right after
// the kernel.img
static char *dataimg_base = (char *) 0xA0000;

static inline uint32_t get_num_bgs(ext2_sb_t *sb) {
    uint32_t n = sb->info.blocks_count / sb->info.blocks_per_group;
    if (sb->info.blocks_count % sb->info.blocks_per_group > 0) {
        n++;
    }
    return n;
}

static ext2_bg_desc_t *get_bg_desc(ext2_sb_t *sb, uint32_t index) {
    if (index >= get_num_bgs(sb)) {
        error("Invalid group descriptor index %lu", index);
        return NULL;
    }

    /**
     * The block group descriptor table starts on the first block following
     * the superblock. This would be the third block on a 1KiB block file
     * system, or the second block for 2KiB and larger block file systems.
     */
    uint32_t bgd_offset = sb->block_size;
    if (sb->block_size == 1024) {
        bgd_offset <<= 1;
    }
    return (ext2_bg_desc_t *) (dataimg_base + bgd_offset + sizeof(ext2_bg_desc_t) * index);
}

static bool is_valid_superblock(ext2_sb_t *sb) {
    if (sb->info.magic != EXT2_SB_MAGIC) {
        error("Invalid ext2 magic number");
        return false;
    }

    return true;
}

static inline void *get_phys_block_ptr(ext2_sb_t *sb, uint32_t phys_block_num) {
    return dataimg_base + phys_block_num * sb->block_size;
}

static ext2_inode_data_t *get_inode_from_fs(ext2_sb_t *sb, uint32_t inode) {
    if (inode >= sb->info.inodes_count) {
        error("Invalid inode %lu", inode);
        return NULL;
    }

    // Get block descriptor in which the inode is allocated
    uint32_t bg;
    bg = (inode - 1) / sb->info.inodes_per_group;
    ext2_bg_desc_t *bgd = get_bg_desc(sb, bg);
    if (bgd == NULL) {
        error("Couldn't get group descriptor");
        return NULL;
    }

    // Calculate the offset within inode table
    uint32_t offset;
    offset = ((inode - 1) % sb->info.inodes_per_group) * EXT2_INODE_SIZE;

    // Calculate the block number of the inode
    uint32_t block;
    block = bgd->inode_table + (offset >> sb->block_size_bits);

    // Calculate the offset within the block
    uint32_t block_offset = offset & (sb->block_size - 1);
    return (ext2_inode_data_t *) (((char *) get_phys_block_ptr(sb, block)) + block_offset);
}

static inline void *get_inode_phys_block(ext2_sb_t *sb, ext2_inode_data_t *inode, uint32_t logical_block) {
    if (logical_block < EXT2_NDIR_BLOCKS) {
        return get_phys_block_ptr(sb, inode->block[logical_block]);
    }
    return NULL;
}

static inline uint32_t get_inode_num_blocks(ext2_sb_t *sb, ext2_inode_data_t *inode) {
    // Read ext2_inode_data_t.blocks documentation for more info
    return inode->blocks / (2 << sb->info.log_block_size);
}

static fs_inode_t *alloc_inode_for(ext2_sb_t *sb, ext2_direntry_t *dentry) {
    fs_inode_t *inode = sb->parent.ops.alloc_inode(&sb->parent);
    if (inode == NULL) {
        error("Couldn't allocate inode");
        return NULL;
    }
    inode->number = dentry->inode;

    ext2_inode_data_t *inode_data = get_inode_from_fs(sb, inode->number);
    if (inode_data == NULL) {
        error("Failed to read inode #%lu", inode->number);
        return NULL;
    }

    if (S_ISDIR(inode_data->mode)) {
        inode->mode |= FS_ACCESS_MODE_DIR;
    }
    if (inode_data->mode & S_IRUSR) {
        inode->mode |= FS_ACCESS_MODE_READ;
    }
    if (inode_data->mode & S_IWUSR) {
        inode->mode |= FS_ACCESS_MODE_WRITE;
    }
    if (inode_data->mode & S_IXUSR) {
        inode->mode |= FS_ACCESS_MODE_EXEC;
    }

    verbose("New inode #%lu mode=0x%x", inode->number, inode->mode);

    return inode;
}

static fs_inode_t *ext2_def_lookup(fs_inode_t *parent, char *name) {
    ext2_sb_t *sb = (ext2_sb_t *) parent->sb;
    ext2_inode_data_t *pinode_data = get_inode_from_fs(sb, parent->number);
    if (pinode_data == NULL) {
        error("Failed to read inode #%lu", parent->number);
        return NULL;
    }

    int i;
    for (i = 0; i < get_inode_num_blocks(sb, pinode_data); i++) {
        char *dptr = get_inode_phys_block(sb, pinode_data, i);
        if (dptr == NULL) {
            error("Couldn't get inode physical block");
            return NULL;
        }

        char *next_block = dptr + sb->block_size;
        while (dptr < next_block) {
            ext2_direntry_t *dentry = (ext2_direntry_t *) dptr;

            if (strncmp(dentry->name, name, dentry->name_len) == 0 &&
                    strlen(name) == dentry->name_len) {
                return alloc_inode_for(sb, dentry);
            }

            dptr += dentry->rec_len;
        }
    }

    return NULL;
}

static int ext2_def_open(fs_inode_t *inode, fs_file_t *f) {
    return 0;
}

static int ext2_def_close(fs_inode_t *inode, fs_file_t *f) {
    return 0;
}

static inline bool is_dot_double_dot_dentry(ext2_direntry_t *dentry) {
    return (dentry->name_len == 1 && dentry->name[0] == '.') ||
            (dentry->name_len == 2 && dentry->name[0] == '.' &&
                    dentry->name[1] == '.');
}

static int ext2_listdir(fs_file_t *f, uint32_t offset, fs_listdir_t *dirlist, uint32_t listlen) {
    ext2_sb_t *sb = (ext2_sb_t *) f->dentry->inode->sb;
    ext2_inode_data_t *inode = get_inode_from_fs(sb, f->dentry->inode->number);
    if (inode == NULL) {
        error("Failed to read inode %lu", f->dentry->inode->number);
        return -1;
    }

    uint32_t nentries = 0;
    uint32_t entrypos = 0;

    int i;
    for (i = 0; i < get_inode_num_blocks(sb, inode); i++) {
        char *dptr = get_inode_phys_block(sb, inode, i);
        if (dptr == NULL) {
            error("Couldn't get inode physical block");
            return -1;
        }

        char *next_block = dptr + sb->block_size;
        while (dptr < next_block) {
            ext2_direntry_t *dentry = (ext2_direntry_t *) dptr;

            if (!is_dot_double_dot_dentry(dentry)) {
                if (entrypos >= offset) {
                    fs_listdir_t *dir = &dirlist[nentries];
                    uint16_t namelen = min(dentry->name_len, sizeof(dir->name) - 1);
                    strncpy(dir->name, dentry->name, namelen);
                    // The length of a directory entry is always a multiple of 4 and,
                    // therefore, null characters (\0) are added for padding at the
                    // end of the filename, if necessary. The name_len field stores
                    // the actual filename length
                    dir->name[namelen] = '\0';
                    dir->isdir = dentry->file_type == EXT2_FT_DIR;

                    nentries++;
                    if (nentries >= listlen) {
                        return nentries;
                    }
                }
                entrypos++;
            }

            dptr += dentry->rec_len;
        }
    }

    return nentries;
}

static fs_inode_t *alloc_inode(fs_superblock_t *sb) {
    fs_inode_t *inode = calloc(1, sizeof(ext2_inode_t));
    if (inode == NULL) {
        error("No memory available for ext2_inode_t structure");
        return NULL;
    }

    inode->sb = sb;

    inode->ops.lookup = ext2_def_lookup;
    inode->ops.mkdir = NULL; // Not supported yet
    inode->ops.rmdir = NULL; // Not supported yet
    inode->ops.mkregfile = NULL; // Not supported yet
    inode->ops.rmregfile = NULL; // Not supported yet

    inode->fops.open = ext2_def_open;
    inode->fops.close = ext2_def_close;
    inode->fops.listdir = ext2_listdir;
    return (fs_inode_t *) inode;
}

static void free_inode(fs_inode_t *inode) {
    verbose("Freeing inode #%lu", inode != NULL ? inode->number : 0);
    free(inode);
}

static int unmount(fs_mount_t *fsm) {
    free(fsm->sb);
    return 0;
}

static int populate_inode(ext2_sb_t *sb, ext2_inode_t *inode) {
    ext2_inode_data_t *disk_inode = get_inode_from_fs(sb, inode->parent.number);
    if (disk_inode == NULL) {
        error("Couldn't get inode #%lu", inode->parent.number);
        return -1;
    }


    return 0;
}

static int populate_ext2_superblock(ext2_sb_t *sb, fs_mount_t *m) {
    memcpy(&sb->info, dataimg_base + EXT2_SB_OFFSET, sizeof(ext2_sb_info_t));

    sb->block_size = (uint32_t) 1024 << sb->info.log_block_size;
    bitset_t bs = ~sb->block_size;
    sb->block_size_bits = BITSET_NBITS - bitset_ffz(bs) - 1;

    debug("Ext2 superblock info:");
    debug("  magic: 0x%X", sb->info.magic);
    debug("  inodes_count: %lu", sb->info.inodes_count);
    debug("  blocks_count: %lu", sb->info.blocks_count);
    debug("  block_size: %lu", sb->block_size);
    debug("  free_blocks_count: %lu", sb->info.free_blocks_count);
    debug("  free_inodes_count: %lu", sb->info.free_inodes_count);
    debug("  blocks_per_group: %lu", sb->info.blocks_per_group);
    debug("  frags_per_group: %lu", sb->info.frags_per_group);
    debug("  inodes_per_group: %lu", sb->info.inodes_per_group);
    debug("  mnt_count: %u", sb->info.mnt_count);
    calendar_t cal;
    epoch_to_localtime_calendar(sb->info.wtime, &cal);
    debug("  wtime: %02d/%02d/%04ld %02d:%02d:%02d",
            cal.mon + 1, cal.mday, cal.year + 1900,
            cal.hour, cal.min, cal.sec);

    verbose("Block groups:");
    int i;
    for (i = 0; i < get_num_bgs(sb); i++) {
        ext2_bg_desc_t *bgd = get_bg_desc(sb, i);
        if (bgd == NULL) {
            error("Couldn't get block group descriptor #%d", i);
            continue;
        }

        verbose("  #%d:", i);
        verbose("    block_bitmap: %lu", bgd->block_bitmap);
        verbose("    inode_bitmap: %lu", bgd->inode_bitmap);
        verbose("    inode_table: %lu", bgd->inode_table);
        verbose("    free_blocks_count: %u", bgd->free_blocks_count);
        verbose("    free_inodes_count: %u", bgd->free_inodes_count);
    }

    return 0;
}

static int mount(fs_type_t *fstype, fs_mount_t *m) {
    ext2_sb_t *ext2sb = calloc(1, sizeof(ext2_sb_t));
    if (ext2sb == NULL) {
        error("No memory available for ext2_sb_t structure");
        goto error_sb;
    }

    m->sb = (fs_superblock_t *) ext2sb;
    m->sb->fstype = fstype;
    m->sb->ops.alloc_inode = alloc_inode;
    m->sb->ops.free_inode = free_inode;

    m->ops.unmount = unmount;

    if (populate_ext2_superblock(ext2sb, m) < 0) {
        error("Couldn't read superblock");
        goto error_populate;
    }

    if (!is_valid_superblock(ext2sb)) {
        error("Malformed superblock");
        goto error_malformed;
    }

    m->sb->root = alloc_inode(m->sb);
    if (m->sb->root == NULL) {
        error("Couldn't allocate root inode");
        goto error_root;
    }
    m->sb->root->number = EXT2_ROOT_INO;

    if (populate_inode(ext2sb, (ext2_inode_t *) m->sb->root) < 0) {
        error("Couldn't populate root inode");
        goto error_root_pop;
    }

    return 0;

error_root_pop:
    free_inode(m->sb->root);
error_root:
error_malformed:
error_populate:
    free(ext2sb);
error_sb:
    return -1;
}

FILESYSTEM_MODULE(ext2, mount);



#ifdef CONFIG_TEST_CORE_FS_EXT2_CORE
#include __FILE__
#endif
