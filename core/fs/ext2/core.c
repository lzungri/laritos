/**
 * DISCLAIMER: This is a very basic, slow, and unreliable implementation of the ext2
 * filesystem based on my poor knowledge of ext2. Use at your own risk...
 *
 * TODO:
 *  - Support indirect blocks for write operations
 *  - Support big/little endian
 *  - Thread safety
 *  - Implement rmdir
 *  - Fix lots of bugs...
 *  - Improve performance (e.g. do not flush metadata right after each modification, use dirty flag)
 */

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
#include <generated/autoconf.h>


static inline int dev_read(ext2_sb_t *sb, void *buf, size_t n, uint32_t offset) {
    return sb->parent.dev->ops.read(sb->parent.dev, buf, n, offset);
}

static inline int dev_write(ext2_sb_t *sb, void *buf, size_t n, uint32_t offset) {
    return sb->parent.dev->ops.write(sb->parent.dev, buf, n, offset);
}

static inline uint32_t inode_nblocks(ext2_sb_t *sb, ext2_inode_data_t *inode) {
    // inode->blocks is a 32-bit value representing the total number of
    // 512-bytes blocks reserved to contain the data of this inode
    return inode->blocks >> (sb->block_size_bits - 9);
}

static ext2_bg_desc_t *get_bg_desc(ext2_sb_t *sb, uint32_t index) {
    if (index >= sb->num_bg_descs) {
        error("Invalid group descriptor index %lu", index);
        return NULL;
    }

    return &sb->bg_descs[index];
}

static inline ext2_bg_desc_t *get_bgd_for_inode(ext2_sb_t *sb, uint32_t inodenum) {
    return get_bg_desc(sb, (inodenum - 1) / sb->info.inodes_per_group);
}

static inline ext2_bg_desc_t *get_bgd_for_block(ext2_sb_t *sb, uint32_t blocknum) {
    return get_bg_desc(sb, (blocknum - 1) / sb->info.blocks_per_group);
}

static bool is_valid_superblock(ext2_sb_t *sb) {
    if (sb->info.magic != EXT2_SB_MAGIC) {
        error("Invalid ext2 magic number");
        return false;
    }

    return true;
}

static inline uint32_t get_phys_block_offset(ext2_sb_t *sb, uint32_t phys_block_num) {
    return phys_block_num * sb->block_size;
}

static int read_inode_from_dev(ext2_sb_t *sb, uint32_t inode, ext2_inode_data_t *data) {
    if (inode >= sb->info.inodes_count) {
        error("Invalid inode %lu", inode);
        return -1;
    }

    // Get block descriptor in which the inode is allocated
    ext2_bg_desc_t *bgd = get_bgd_for_inode(sb, inode);
    if (bgd == NULL) {
        error("Couldn't get group descriptor");
        return -1;
    }

    // Calculate the offset within inode table
    uint32_t offset;
    offset = ((inode - 1) % sb->info.inodes_per_group) * EXT2_INODE_SIZE;

    // Calculate the block number of the inode
    uint32_t block;
    block = bgd->inode_table + (offset >> sb->block_size_bits);

    // Calculate the offset within the block
    uint32_t block_offset = offset & (sb->block_size - 1);

    if (dev_read(sb, data, sizeof(ext2_inode_data_t),
            get_phys_block_offset(sb, block) + block_offset) < 0) {
        error("Couldn't read inode data from device '%s'", sb->parent.dev->parent.id);
        return -1;
    }

    return 0;
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

static int get_inode_phys_block_offset(ext2_sb_t *sb, ext2_inode_data_t *inode, uint32_t logical_block, uint32_t *block) {
    int offsets[4];
    int boundary = 0;

    uint8_t paths = ext2_log_block_to_path(sb, logical_block, offsets, &boundary);
    if (paths == 0) {
        error("Couldn't get physical block for logical_block=%lu", logical_block);
        return -1;
    }

    uint32_t phys_block = inode->block[offsets[0]];
    int i;
    // Start from path=1, we already grabbed the block for path=0
    for (i = 1; i < paths; i++) {
        uint32_t block_offset = get_phys_block_offset(sb, phys_block);
        if (dev_read(sb, &phys_block, sizeof(uint32_t),
                block_offset + offsets[i] * sizeof(uint32_t)) < 0) {
            error("Couldn't read phys block offset from device '%s'", sb->parent.dev->parent.id);
            return -1;
        }
    }

    *block = get_phys_block_offset(sb, phys_block);
    return 0;
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
        error("Couldn't allocate inode for '%s'", dentry->name);
        return NULL;
    }
    inode->number = dentry->inode;

    ext2_inode_data_t inode_data;
    if (read_inode_from_dev(sb, inode->number, &inode_data) < 0) {
        error("Failed to read inode #%lu", inode->number);
        goto error_read;
    }

    if (S_ISDIR(inode_data.mode)) {
        inode->mode |= FS_ACCESS_MODE_DIR;
    }
    if (inode_data.mode & S_IRUSR) {
        inode->mode |= FS_ACCESS_MODE_READ;
    }
    if (inode_data.mode & S_IWUSR) {
        inode->mode |= FS_ACCESS_MODE_WRITE;
    }
    if (inode_data.mode & S_IXUSR) {
        inode->mode |= FS_ACCESS_MODE_EXEC;
    }

    verbose("New inode #%lu mode=0x%x", inode->number, inode->mode);

    return inode;

error_read:
    sb->parent.ops.free_inode(inode);
    return NULL;
}

static int flush_inode(ext2_sb_t *sb, uint32_t inodenum, ext2_inode_data_t *inode) {
    debug("Flushing inode #%lu", inodenum);

    ext2_bg_desc_t *bgd = get_bgd_for_inode(sb, inodenum);
    if (bgd == NULL) {
        error("Couldn't find block group for inode #%lu", inodenum);
        return -1;
    }

    uint32_t iindex = (inodenum - 1) % sb->info.inodes_per_group;
    uint32_t itable_offset = get_phys_block_offset(sb, bgd->inode_table);
    if (dev_write(sb, inode, sizeof(ext2_inode_data_t),
            itable_offset + iindex * sizeof(ext2_inode_data_t)) < 0) {
        error("Failed to flush inode");
        return -1;
    }

    return 0;
}

/**
 * This is overkill, and we use it a lot in the driver, but it simplifies the code
 */
static int flush_metadata(ext2_sb_t *sb) {
    debug("Flushing metadata");

    time_t t;
    time_get_rtc_time(&t);
    sb->info.wtime = t.secs;

    int i;
    for (i = 0; i < sb->num_bg_descs; i++) {
        ext2_bg_desc_t *bgd = get_bg_desc(sb, i);
        if (bgd == NULL) {
            error("Couldn't get block group descriptor #%d", i);
            continue;
        }

        verbose("Block group #%d freeblocks=%u, freeinodes=%u", i,
                bgd->free_blocks_count, bgd->free_inodes_count);
        if (dev_write(sb, bgd, sizeof(ext2_bg_desc_t),
                sb->bgd_pblock_offset + i * sizeof(ext2_bg_desc_t)) < 0) {
            error("Failed to flush block group #%d", i);
            return -1;
        }
    }

    verbose("Superblock freeblocks=%lu, freeinodes=%lu",
            sb->info.free_blocks_count, sb->info.free_inodes_count);
    if (dev_write(sb, &sb->info, sizeof(ext2_sb_info_t), EXT2_SB_OFFSET) < 0) {
        error("Failed to flush superblock");
        return -1;
    }

    return 0;
}

static void populate_inode_mode(ext2_inode_data_t *inode, fs_access_mode_t mode) {
    if (mode & FS_ACCESS_MODE_DIR) {
        inode->mode |= S_IFDIR;
    } else {
        inode->mode |= S_IFREG;
    }
    if (mode & FS_ACCESS_MODE_READ) {
        inode->mode |= S_IRUSR;
    }
    if (mode & FS_ACCESS_MODE_WRITE) {
        inode->mode |= S_IWUSR;
    }
    if (mode & FS_ACCESS_MODE_EXEC) {
        inode->mode |= S_IXUSR;
    }
}

static int set_bitmap_bit_to(ext2_sb_t *sb, uint32_t bmap_offset, uint32_t bitpos, bool value) {
    verbose("Setting bit %lu of bitmap at 0x%lx to %u", bitpos, bmap_offset, value);
    uint8_t buf;
    if (dev_read(sb, &buf, sizeof(buf), bmap_offset + bitpos / (sizeof(buf) * 8)) < 0) {
        error("Failed to read bitmap");
        return -1;
    }
    uint8_t bufbit = bitpos % (sizeof(buf) * 8);
    buf = (buf & ~(1 << bufbit)) | ((value ? 1 : 0) << bufbit);
    if (dev_write(sb, &buf, sizeof(buf), bmap_offset + bitpos / (sizeof(buf) * 8)) < 0) {
        error("Failed to clear bit");
        return -1;
    }
    return 0;
}

static int ffz_in_bitmap_and_set(ext2_sb_t *sb, uint32_t bmap_offset, uint32_t maxbits, uint32_t *bitpos) {
    uint32_t buf;
    int i;
    for (i = 0; i * sizeof(buf) < sb->block_size; i++) {
        if (dev_read(sb, &buf, sizeof(buf), bmap_offset + i * sizeof(buf)) < 0) {
            error("Failed to read bitmap at offset=%lu", bmap_offset + i * sizeof(buf));
            return -1;
        }
        if (buf != 0xffffffff) {
            // Assume 32bits
            bitset_t bs = ~buf;
            uint8_t ffz = bitset_ffz(bs);
            *bitpos = ffz == BITSET_IDX_NOT_FOUND ? 0 : BITSET_NBITS - ffz;
            buf |= 1 << *bitpos;

            verbose("Setting bit %lu of bitmap at 0x%lx to 1", *bitpos, bmap_offset);

            *bitpos += i * sizeof(buf) * 8;

            // Check if the max number of inodes for this bitmap has been exceeded
            if (*bitpos >= maxbits) {
                return -1;
            }

            if (dev_write(sb, &buf, sizeof(buf), bmap_offset + i * sizeof(buf)) < 0) {
                error("Failed to update bitmap at offset=%lu", bmap_offset + i * sizeof(buf));
                return -1;
            }
            return 0;
        }
    }
    return -1;
}

static int allocate_block_from_dev(ext2_sb_t *sb, uint32_t *blocknum, uint32_t bgdidx, ext2_bg_desc_t *bgd) {
    if (bgd->free_blocks_count <= 0) {
        error("No free blocks left");
        return -1;
    }

    uint32_t bmap_offset = get_phys_block_offset(sb, bgd->block_bitmap);
    uint32_t bitpos;
    if (ffz_in_bitmap_and_set(sb, bmap_offset, sb->info.blocks_per_group, &bitpos) < 0) {
        error("No free block in bitmap");
        return -1;
    }

    *blocknum = bitpos + bgdidx * sb->info.blocks_per_group + 1;

    bgd->free_blocks_count--;
    sb->info.free_blocks_count--;

    if (flush_metadata(sb) < 0) {
        error("Failed to flush ext2 metadata");
        goto error_flush;
    }

    debug("Block #%lu allocated", *blocknum);

    return 0;

error_flush:
    set_bitmap_bit_to(sb, bmap_offset, bitpos, 0);
    return -1;
}

static int deallocate_block_from_dev(ext2_sb_t *sb, uint32_t blocknum) {
    ext2_bg_desc_t *bgd = get_bgd_for_block(sb, blocknum);
    if (bgd == NULL) {
        error("No block group for block #%lu", blocknum);
        return -1;
    }

    uint32_t bmap_offset = get_phys_block_offset(sb, bgd->block_bitmap);
    set_bitmap_bit_to(sb, bmap_offset, (blocknum - 1) % sb->info.blocks_per_group, 0);

    bgd->free_blocks_count++;
    sb->info.free_blocks_count++;

    if (flush_metadata(sb) < 0) {
        error("Failed to flush ext2 metadata");
        goto error_flush;
    }

    debug("Block #%lu deallocated", blocknum);

    return 0;

error_flush:
    set_bitmap_bit_to(sb, bmap_offset, (blocknum - 1) % sb->info.blocks_per_group, 1);
    return -1;
}

static int allocate_free_inode_from_bg(ext2_sb_t *sb, uint32_t bgdidx, ext2_bg_desc_t *bgd, uint32_t *inodenum) {
    uint32_t bmap_offset = get_phys_block_offset(sb, bgd->inode_bitmap);
    uint32_t bitpos;
    if (ffz_in_bitmap_and_set(sb, bmap_offset, sb->info.inodes_per_group, &bitpos) < 0) {
        error("No free inode in bitmap");
        return -1;
    }
    *inodenum = bitpos + bgdidx * sb->info.blocks_per_group + 1;
    return 0;
}

static int allocate_free_inode_from_dev(ext2_sb_t *sb, uint32_t *inodenum) {
    if (sb->info.free_inodes_count == 0) {
        error("Free inodes count == 0");
        return -1;
    }

    int i;
    for (i = 0; i < sb->num_bg_descs; i++) {
        ext2_bg_desc_t *bgd = get_bg_desc(sb, i);
        if (bgd == NULL) {
            error("Couldn't get block group descriptor #%d", i);
            continue;
        }

        if (bgd->free_inodes_count > 0 && allocate_free_inode_from_bg(sb, i, bgd, inodenum) >= 0) {
            bgd->free_inodes_count--;
            sb->info.free_inodes_count--;
            flush_metadata(sb);
            debug("Inode #%lu allocated", *inodenum);
            return 0;
        }
    }

    error("Couldn't find free inode");
    return -1;
}

static int deallocate_inode_from_dev(ext2_sb_t *sb, uint32_t inodenum) {
    ext2_inode_data_t physinode;
    if (read_inode_from_dev(sb, inodenum, &physinode) < 0) {
        error("Failed to read inode #%lu", inodenum);
        return -1;
    }

    int i;
    for (i = 0; i < inode_nblocks(sb, &physinode); i++) {
        if (deallocate_block_from_dev(sb, physinode.block[i]) < 0) {
            error("Failed to deallocate logical block #%d (phys block=%lu) from inode #%lu",
                    i, physinode.block[i], inodenum);
            goto error_dealloc_block;
        }
    }

    ext2_bg_desc_t *bgd = get_bgd_for_inode(sb, inodenum);
    if (bgd == NULL) {
        error("Couldn't find block group for inode #%lu", inodenum);
        goto error_bgd;
    }

    uint32_t bitpos = (inodenum - 1) % sb->info.inodes_per_group;
    if (set_bitmap_bit_to(sb, get_phys_block_offset(sb, bgd->inode_bitmap), bitpos, 0) < 0) {
        error("Failed to clear bit #%lu from inode bitmap", bitpos);
        goto error_clear;
    }

    bgd->free_inodes_count++;
    sb->info.free_inodes_count++;
    if (flush_metadata(sb) < 0) {
        error("Failed to flush ext2 metadata");
        goto error_flushmt;
    }

    debug("Inode #%lu deallocated", inodenum);
    return 0;

error_flushmt:
    set_bitmap_bit_to(sb, get_phys_block_offset(sb, bgd->inode_bitmap), bitpos, 1);
error_clear:
error_bgd:
error_dealloc_block:
    // TODO Realloc blocks :(
    return -1;
}

static int allocate_block_for_inode(ext2_sb_t *sb, ext2_inode_data_t *inode, uint32_t inodenum, uint32_t *blocknum) {
    if (inode_nblocks(sb, inode) >= EXT2_NDIR_BLOCKS) {
        // Indirect blocks not supported yet
        error("Indirect blocks not supported yet");
        return -1;
    }

    uint32_t bgdidx = (inodenum - 1) / sb->info.inodes_per_group;
    if (allocate_block_from_dev(sb, blocknum, bgdidx, get_bgd_for_inode(sb, inodenum)) < 0) {
        error("Couldn't allocate block");
        return -1;
    }

    inode->block[inode_nblocks(sb, inode)] = *blocknum;
    inode->blocks += sb->block_size >> 9;

    if (flush_inode(sb, inodenum, inode) < 0) {
        error("Failed to flush inode");
        goto error_flush;
    }

    return 0;

error_flush:
    deallocate_block_from_dev(sb, *blocknum);
    return -1;
}

static int deallocate_lastblock_for_inode(ext2_sb_t *sb, ext2_inode_data_t *inode, uint32_t inodenum) {
    if (inode_nblocks(sb, inode) > EXT2_NDIR_BLOCKS) {
        // Indirect blocks not supported yet
        error("Indirect blocks not supported yet");
        return -1;
    }

    if (inode_nblocks(sb, inode) <= 0) {
        error("Inode has no blocks allocated");
        return -1;
    }

    if (deallocate_block_from_dev(sb, inode->block[inode_nblocks(sb, inode) - 1]) < 0) {
        error("Failed to deallocate block from inode #%lu", inodenum);
        return -1;
    }

    inode->block[inode_nblocks(sb, inode) - 1] = 0;
    inode->blocks -= sb->block_size >> 9;

    if (flush_inode(sb, inodenum, inode) < 0) {
        error("Failed to flush inode");
        return -1;
    }

    return 0;
}

static int rm_dir_entry_from(ext2_sb_t *sb, fs_inode_t *parent, uint32_t childno) {
    debug("Removing inode #%lu directory entry from inode #%lu", childno, parent->number);

    if (!S_ISDIR(parent->mode)) {
        error("Cannot add dir entry: Not a directory");
        return -1;
    }

    // TODO Cannot delete dir if it has any children
    return -1;
}

static int add_dir_entry_to(ext2_sb_t *sb, ext2_inode_data_t *parent, uint32_t parentno, char *name, uint32_t childno, fs_access_mode_t mode) {
    debug("Adding directory entry '%s' to inode #%lu", name, parentno);

    if (!S_ISDIR(parent->mode)) {
        error("Cannot add dir entry: Not a directory");
        return -1;
    }

    uint32_t blkoff;
    // Get last block of the directory file
    if (get_inode_phys_block_offset(sb, parent, inode_nblocks(sb, parent) - 1, &blkoff) < 0) {
        error("Couldn't get inode physical block");
        return -1;
    }

    char dentry_buf[sizeof(ext2_direntry_t) + EXT2_NAME_LEN];
    ext2_direntry_t *dentry = (ext2_direntry_t *) dentry_buf;

    uint32_t next_block = blkoff + sb->block_size;
    while (true) {
        if (dev_read(sb, dentry, sizeof(ext2_direntry_t), blkoff) < 0) {
              error("Couldn't read dentry metadata from device '%s'", sb->parent.dev->parent.id);
              return -1;
        }
        if (dev_read(sb, (char *) dentry + sizeof(ext2_direntry_t),
            dentry->name_len, blkoff + sizeof(ext2_direntry_t)) < 0) {
            error("Couldn't read dentry file name from device '%s'", sb->parent.dev->parent.id);
            return -1;
        }

        if (dentry->name_len == 0 || blkoff + dentry->rec_len >= next_block) {
            break;
        }
        blkoff += dentry->rec_len;
    }

    uint32_t newblockno = 0;

    // name_len == 0 means no entry in the directory yet
    if (dentry->name_len > 0) {
        // There cannot be any directory entry spanning multiple data blocks.
        // If an entry cannot completely fit in one block, it must be pushed
        // to the next data block and the rec_len of the previous entry properly adjusted.
        uint32_t bytes_to_nextblock = dentry->rec_len - sizeof(ext2_direntry_t) - dentry->name_len;
        if (bytes_to_nextblock < sizeof(ext2_direntry_t) + strlen(name)) {
            debug("Allocating new block for directory with inode #%lu", parentno);
            if (allocate_block_for_inode(sb, parent, parentno, &newblockno) < 0) {
                error("Couldn't allocate block");
                return -1;
            }
            parent->size += sb->block_size;
            if (flush_inode(sb, parentno, parent) < 0) {
                error("Failed to flush inode");
                goto error_flush;
            }
            if (get_inode_phys_block_offset(sb, parent, inode_nblocks(sb, parent) - 1, &blkoff) < 0) {
                error("Couldn't get inode physical block");
                return -1;
            }
        } else {
            // The directory entries must be aligned on 4 bytes boundaries
            dentry->rec_len = sizeof(ext2_direntry_t) + dentry->name_len;
            if (dentry->rec_len & 0b11) {
                dentry->rec_len = (dentry->rec_len + 4) & ~0b11;
            }
            if (dev_write(sb, dentry, sizeof(ext2_direntry_t), blkoff) < 0) {
                  error("Couldn't read dentry metadata from device '%s'", sb->parent.dev->parent.id);
                  return -1;
            }
            blkoff += dentry->rec_len;
        }
    }

    // Setup new dentry using the existing dentry buffer
    dentry->file_type = mode & FS_ACCESS_MODE_DIR ? EXT2_FT_DIR : EXT2_FT_REG_FILE;
    dentry->inode = childno;
    dentry->name_len = strlen(name);
    strncpy(dentry->name, name, EXT2_NAME_LEN);
    dentry->rec_len = sb->block_size - (blkoff % sb->block_size);

    if (dev_write(sb, dentry, sizeof(ext2_direntry_t) + strlen(name), blkoff) < 0) {
        error("Failed to add directory entry offset=%lu", blkoff);
        goto error_write;
    }

    return 0;

error_write:
    // TODO Should rollback the dentry->rec_len if updated
error_flush:
    if (newblockno != 0) {
        deallocate_lastblock_for_inode(sb, parent, parentno);
    }
    return -1;
}

static int allocate_inode_from_dev(ext2_sb_t *sb, fs_inode_t *parent, ext2_inode_data_t *child,
        uint32_t *childno, fs_access_mode_t mode, bool isdir) {
    if (allocate_free_inode_from_dev(sb, childno) < 0) {
        error("Couldn't allocate inode");
        return -1;
    }
    populate_inode_mode(child, mode);

    uint32_t blocknum;
    if (allocate_block_for_inode(sb, child, *childno, &blocknum) < 0) {
        error("Couldn't allocate block");
        goto error_balloc;
    }
    time_t t;
    time_get_rtc_time(&t);
    child->ctime = t.secs;

    if (isdir) {
        // Directory size is equal to the number of allocated blocks
        child->size = sb->block_size;

        if (add_dir_entry_to(sb, child, *childno, ".", *childno,
                FS_ACCESS_MODE_DIR | FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC) < 0) {
            error("Failed to add '.' directory entry");
            goto error_dot;
        }
        if (add_dir_entry_to(sb, child, *childno, "..", parent->number,
                FS_ACCESS_MODE_DIR | FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE | FS_ACCESS_MODE_EXEC) < 0) {
            error("Failed to add '..' directory entry");
            goto error_dotdot;
        }
    }

    if (flush_inode(sb, *childno, child) < 0) {
        error("Failed to flush inode");
        goto error_flush;
    }

    return 0;

error_flush:
error_dotdot:
error_dot:
    deallocate_lastblock_for_inode(sb, child, *childno);
error_balloc:
    deallocate_inode_from_dev(sb, *childno);
    return -1;
}

static int ext2_mkinode(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode, bool isdir) {
    ext2_sb_t *sb = (ext2_sb_t *) parent->sb;

    ext2_inode_data_t parent_phys_inode;
    if (read_inode_from_dev(sb, parent->number, &parent_phys_inode) < 0) {
        error("Failed to read inode #%lu", parent->number);
        return -1;
    }

    uint32_t inodenum;
    ext2_inode_data_t child_phys_inode = { 0 };
    if (allocate_inode_from_dev(sb, parent, &child_phys_inode, &inodenum, mode, isdir) < 0) {
        error("Couldn't allocate inode for directory '%s'", dentry->name);
        goto error_alloc;
    }
    dentry->inode->number = inodenum;

    if (add_dir_entry_to(sb, &parent_phys_inode, parent->number, dentry->name, inodenum, mode) < 0) {
        error("Failed to add directory entry '%s'", dentry->name);
        goto error_entry;
    }

    if (flush_metadata(sb) < 0) {
        error("Failed to flush ext2 metadata");
        goto error_flushmt;
    }

    return 0;

error_flushmt:
    rm_dir_entry_from(sb, parent, inodenum);
error_entry:
    deallocate_inode_from_dev(sb, inodenum);
error_alloc:
    flush_metadata(sb);

    return -1;
}

static fs_inode_t *ext2_def_lookup(fs_inode_t *parent, char *name) {
    ext2_sb_t *sb = (ext2_sb_t *) parent->sb;

    ext2_inode_data_t pinode_data;
    if (read_inode_from_dev(sb, parent->number, &pinode_data) < 0) {
        error("Failed to read inode #%lu", parent->number);
        return NULL;
    }

    int i;
    for (i = 0; i < get_inode_num_blocks(sb, &pinode_data); i++) {
        uint32_t blkoff;
        if (get_inode_phys_block_offset(sb, &pinode_data, i, &blkoff) < 0) {
            error("Couldn't get inode physical block");
            return NULL;
        }

        uint32_t next_block = blkoff + sb->block_size;
        while (blkoff < next_block) {
            char dentry_buf[sizeof(ext2_direntry_t) + EXT2_NAME_LEN];
            ext2_direntry_t *dentry = (ext2_direntry_t *) dentry_buf;
            if (dev_read(sb, dentry, sizeof(ext2_direntry_t), blkoff) < 0) {
                error("Couldn't read dentry metadata from device '%s'", sb->parent.dev->parent.id);
                return NULL;
            }
            if (dev_read(sb, (char *) dentry + sizeof(ext2_direntry_t),
                    dentry->name_len, blkoff + sizeof(ext2_direntry_t)) < 0) {
                error("Couldn't read dentry file name from device '%s'", sb->parent.dev->parent.id);
                return NULL;
            }

            if (strncmp(dentry->name, name, dentry->name_len) == 0 &&
                    strlen(name) == dentry->name_len) {
                return alloc_inode_for(sb, dentry);
            }

            blkoff += dentry->rec_len;
        }
    }

    return NULL;
}

static int ext2_def_mkdir(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    debug("Creating dir '%s'", dentry->name);
    return ext2_mkinode(parent, dentry, mode, true);
}

static int ext2_def_mkregfile(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    debug("Creating regular file '%s'", dentry->name);
    return ext2_mkinode(parent, dentry, mode, false);
}

static int ext2_def_read(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    ext2_sb_t *sb = (ext2_sb_t *) f->dentry->inode->sb;

    ext2_inode_data_t pinode_data;
    if (read_inode_from_dev(sb, f->dentry->inode->number, &pinode_data) < 0) {
        error("Failed to read inode #%lu", f->dentry->inode->number);
        return -1;
    }

    if (offset >= pinode_data.size) {
        return 0;
    }

    int nbytes = 0;

    uint32_t log_block = offset >> sb->block_size_bits;
    uint32_t block_offset = offset % sb->block_size;
    uint32_t bytes_to_eof = pinode_data.size - offset;

    for ( ;log_block < get_inode_num_blocks(sb, &pinode_data); log_block++) {
        uint32_t phys_blkoff;
        if (get_inode_phys_block_offset(sb, &pinode_data, log_block, &phys_blkoff) < 0) {
            error("Couldn't get inode physical block for logical block #%lu", log_block);
            return -1;
        }

        int len = min(blen - nbytes, sb->block_size - block_offset);
        len = min(len, bytes_to_eof);

        if (dev_read(sb, (char *) buf + nbytes, len, phys_blkoff + block_offset) < 0) {
            error("Couldn't read inode data from device '%s'", sb->parent.dev->parent.id);
            return -1;
        }

        nbytes += len;
        bytes_to_eof -= len;
        block_offset = 0;

        if (nbytes >= blen || bytes_to_eof == 0) {
            break;
        }
    }

    return nbytes;
}

static int ext2_def_write(fs_file_t *f, void *buf, size_t blen, uint32_t offset) {
    return -1;
}

static int ext2_def_open(fs_inode_t *inode, fs_file_t *f) {
    // Nothing special to do here
    return 0;
}

static int ext2_def_close(fs_inode_t *inode, fs_file_t *f) {
    // Nothing special to do here
    return 0;
}

static inline bool is_dot_double_dot_dentry(ext2_direntry_t *dentry) {
    return (dentry->name_len == 1 && dentry->name[0] == '.') ||
            (dentry->name_len == 2 && dentry->name[0] == '.' &&
                    dentry->name[1] == '.');
}

static int ext2_listdir(fs_file_t *f, uint32_t offset, fs_listdir_t *dirlist, uint32_t listlen) {
    ext2_sb_t *sb = (ext2_sb_t *) f->dentry->inode->sb;
    ext2_inode_data_t inode;
    if (read_inode_from_dev(sb, f->dentry->inode->number, &inode) < 0) {
        error("Failed to read inode %lu", f->dentry->inode->number);
        return -1;
    }

    uint32_t nentries = 0;
    uint32_t entrypos = 0;

    int i;
    for (i = 0; i < get_inode_num_blocks(sb, &inode); i++) {
        uint32_t blkoff;
        if (get_inode_phys_block_offset(sb, &inode, i, &blkoff) < 0) {
            error("Couldn't get inode physical block");
            return -1;
        }

        uint32_t next_block = blkoff + sb->block_size;
        while (blkoff < next_block) {
            char dentry_buf[sizeof(ext2_direntry_t) + EXT2_NAME_LEN];
            ext2_direntry_t *dentry = (ext2_direntry_t *) dentry_buf;
            if (dev_read(sb, dentry, sizeof(ext2_direntry_t), blkoff) < 0) {
                error("Couldn't read dentry metadata from device '%s'", sb->parent.dev->parent.id);
                return -1;
            }
            if (dev_read(sb, (char *) dentry + sizeof(ext2_direntry_t),
                    dentry->name_len, blkoff + sizeof(ext2_direntry_t)) < 0) {
                error("Couldn't read dentry file name from device '%s'", sb->parent.dev->parent.id);
                return -1;
            }

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

            blkoff += dentry->rec_len;
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
    inode->ops.mkdir = ext2_def_mkdir;
    inode->ops.rmdir = NULL;
    inode->ops.mkregfile = ext2_def_mkregfile;
    inode->ops.rmregfile = NULL;

    inode->fops.open = ext2_def_open;
    inode->fops.close = ext2_def_close;
    inode->fops.read = ext2_def_read;
    inode->fops.write = ext2_def_write;
    inode->fops.listdir = ext2_listdir;
    return (fs_inode_t *) inode;
}

static void free_inode(fs_inode_t *inode) {
    verbose("Freeing inode #%lu", inode != NULL ? inode->number : 0);
    free(inode);
}

static int unmount(fs_mount_t *fsm) {
    free(((ext2_sb_t *) fsm->sb)->bg_descs);
    free(fsm->sb);
    return 0;
}

static int populate_ext2_superblock(ext2_sb_t *sb, fs_mount_t *m) {
    if (dev_read(sb, &sb->info, sizeof(ext2_sb_info_t), EXT2_SB_OFFSET) < 0) {
        error("Couldn't read superblock info from device '%s'", sb->parent.dev->parent.id);
        return -1;
    }

    sb->block_size = (uint32_t) 1024 << sb->info.log_block_size;
    bitset_t bs = ~sb->block_size;
    sb->block_size_bits = BITSET_NBITS - bitset_ffz(bs) - 1;

    sb->addr_per_block = sb->block_size / sizeof(uint32_t);
    bs = ~sb->addr_per_block;
    sb->addr_per_block_bits = BITSET_NBITS - bitset_ffz(bs) - 1;

    sb->num_bg_descs = sb->info.blocks_count / sb->info.blocks_per_group;
    if (sb->info.blocks_count % sb->info.blocks_per_group > 0) {
        sb->num_bg_descs++;
    }

    sb->bg_descs = calloc(sb->num_bg_descs, sizeof(ext2_bg_desc_t));
    if (sb->bg_descs == NULL) {
        error("No memory for ext2_bg_desc_t array");
        return -1;
    }

    /**
     * The block group descriptor table starts on the first block following
     * the superblock. This would be the third block on a 1KiB block file
     * system, or the second block for 2KiB and larger block file systems.
     */
    sb->bgd_pblock_offset = sb->block_size;
    if (sb->block_size == 1024) {
        sb->bgd_pblock_offset <<= 1;
    }
    if (dev_read(sb, sb->bg_descs,
            sizeof(ext2_bg_desc_t) * sb->num_bg_descs, sb->bgd_pblock_offset) < 0) {
        error("Couldn't read block group descriptors from device '%s'", sb->parent.dev->parent.id);
        free(sb->bg_descs);
        return -1;
    }

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
    for (i = 0; i < sb->num_bg_descs; i++) {
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

    ext2sb->parent.dev = vfs_get_param(params, "dev");
    if (ext2sb->parent.dev == NULL) {
        error("No device was given");
        goto error_dev;

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

    time_t t;
    time_get_rtc_time(&t);
    ext2sb->info.mtime = t.secs;
    ext2sb->info.mnt_count++;
    flush_metadata(ext2sb);

    return 0;

error_root:
error_malformed:
    free(ext2sb->bg_descs);
error_populate:
error_dev:
    free(ext2sb);
error_sb:
    return -1;
}

FILESYSTEM_MODULE(ext2, mount);



#ifdef CONFIG_TEST_CORE_FS_EXT2_CORE
#include __FILE__
#endif
