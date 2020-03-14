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
#include <generated/autoconf.h>

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
    return (ext2_bg_desc_t *) (sb->mem_base + bgd_offset + sizeof(ext2_bg_desc_t) * index);
}

static bool is_valid_superblock(ext2_sb_t *sb) {
    if (sb->info.magic != EXT2_SB_MAGIC) {
        error("Invalid ext2 magic number");
        return false;
    }

    return true;
}

static inline void *get_phys_block_ptr(ext2_sb_t *sb, uint32_t phys_block_num) {
    return sb->mem_base + phys_block_num * sb->block_size;
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

/**
 * Ported from Linux source
 *
 * ext2_block_to_path - parse the block number into array of offsets
 * @inode: inode in question (we are only interested in its superblock)
 * @i_block: block number to be parsed
 * @offsets: array to store the offsets in
 *     @boundary: set this non-zero if the referred-to block is likely to be
 *            followed (on disk) by an indirect block.
 * To store the locations of file's data ext2 uses a data structure common
 * for UNIX filesystems - tree of pointers anchored in the inode, with
 * data blocks at leaves and indirect blocks in intermediate nodes.
 * This function translates the block number into path in that tree -
 * return value is the path length and @offsets[n] is the offset of
 * pointer to (n+1)th node in the nth one. If @block is out of range
 * (negative or too large) warning is printed and zero returned.
 *
 * Note: function doesn't find node addresses, so no IO is needed. All
 * we need to know is the capacity of indirect blocks (taken from the
 * inode->i_sb).
 */
static uint8_t ext2_log_block_to_path(ext2_sb_t *sb, uint32_t logical_block, int offsets[4], int *boundary) {
    int ptrs = sb->addr_per_block;
    int ptrs_bits = sb->addr_per_block_bits;
    const long direct_blocks = EXT2_NDIR_BLOCKS;
    const long indirect_blocks = ptrs;
    const long double_blocks = (1 << (ptrs_bits * 2));
    uint8_t n = 0;
    int final = 0;

    if (logical_block < direct_blocks) {
        offsets[n++] = logical_block;
        final = direct_blocks;
    } else if ( (logical_block -= direct_blocks) < indirect_blocks) {
        offsets[n++] = EXT2_IND_BLOCK;
        offsets[n++] = logical_block;
        final = ptrs;
    } else if ((logical_block -= indirect_blocks) < double_blocks) {
        offsets[n++] = EXT2_DIND_BLOCK;
        offsets[n++] = logical_block >> ptrs_bits;
        offsets[n++] = logical_block & (ptrs - 1);
        final = ptrs;
    } else if (((logical_block -= double_blocks) >> (ptrs_bits * 2)) < ptrs) {
        offsets[n++] = EXT2_TIND_BLOCK;
        offsets[n++] = logical_block >> (ptrs_bits * 2);
        offsets[n++] = (logical_block >> ptrs_bits) & (ptrs - 1);
        offsets[n++] = logical_block & (ptrs - 1);
        final = ptrs;
    } else {
        error("Logical block is too big");
    }
    if (boundary)
        *boundary = final - 1 - (logical_block & (ptrs - 1));

    return n;
}

static inline void *get_inode_phys_block(ext2_sb_t *sb, ext2_inode_data_t *inode, uint32_t logical_block) {
    int offsets[4];
    int boundary = 0;

    uint8_t paths = ext2_log_block_to_path(sb, logical_block, offsets, &boundary);
    if (paths == 0) {
        error("Couldn't get physical block for logical_block=%lu", logical_block);
        return NULL;
    }

    uint32_t phys_block = inode->block[offsets[0]];
    int i;
    // Start from path=1, we already grabbed the block for path=0
    for (i = 1; i < paths; i++) {
        uint32_t *block = get_phys_block_ptr(sb, phys_block);
        phys_block = *(block + offsets[i]);
    }

    return get_phys_block_ptr(sb, phys_block);
}

static inline uint32_t get_inode_num_blocks(ext2_sb_t *sb, ext2_inode_data_t *inode) {
    uint32_t n = inode->size >> sb->block_size_bits;
    if (inode->size & (sb->block_size - 1)) {
        n++;
    }
    return n;
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

static int ext2_def_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    ext2_sb_t *sb = (ext2_sb_t *) f->dentry->inode->sb;
    ext2_inode_data_t *pinode_data = get_inode_from_fs(sb, f->dentry->inode->number);
    if (pinode_data == NULL) {
        error("Failed to read inode #%lu", f->dentry->inode->number);
        return -1;
    }

    if (offset >= pinode_data->size) {
        return 0;
    }

    int nbytes = 0;

    uint32_t log_block = offset >> sb->block_size_bits;
    uint32_t block_offset = offset % sb->block_size;
    uint32_t bytes_to_eof = pinode_data->size - offset;

    for ( ;log_block < get_inode_num_blocks(sb, pinode_data); log_block++) {
        char *physb_ptr = get_inode_phys_block(sb, pinode_data, log_block);
        if (physb_ptr == NULL) {
            error("Couldn't get inode physical block for logical block #%lu", log_block);
            return -1;
        }

        int len = min(blen - nbytes, sb->block_size - block_offset);
        len = min(len, bytes_to_eof);
        memcpy((char *) buf + nbytes, physb_ptr + block_offset, len);
        nbytes += len;
        bytes_to_eof -= len;
        block_offset = 0;

        if (nbytes >= blen || bytes_to_eof == 0) {
            break;
        }
    }

    return nbytes;
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

            if (!is_dot_double_dot_dentry(dentry) && dentry->name_len > 0) {
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
    inode->fops.read = ext2_def_read;
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

static int populate_ext2_superblock(ext2_sb_t *sb, fs_mount_t *m) {
    memcpy(&sb->info, sb->mem_base + EXT2_SB_OFFSET, sizeof(ext2_sb_info_t));

    sb->block_size = (uint32_t) 1024 << sb->info.log_block_size;
    bitset_t bs = ~sb->block_size;
    sb->block_size_bits = BITSET_NBITS - bitset_ffz(bs) - 1;

    sb->addr_per_block = sb->block_size / sizeof(uint32_t);
    bs = ~sb->addr_per_block;
    sb->addr_per_block_bits = BITSET_NBITS - bitset_ffz(bs) - 1;

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

static int mount(fs_type_t *fstype, fs_mount_t *m, fs_param_t *params) {
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

    if (vfs_get_param_uint32(params, "mem-offset", (uint32_t *) &ext2sb->mem_base, 16) < 0) {
        error("No FS memory offset was given, required for in-memory FS");
        goto error_offset;
    }

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

    return 0;

error_root:
error_malformed:
error_populate:
error_offset:
    free(ext2sb);
error_sb:
    return -1;
}

FILESYSTEM_MODULE(ext2, mount);



#ifdef CONFIG_TEST_CORE_FS_EXT2_CORE
#include __FILE__
#endif
