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
#include <time/timeconv.h>
#include <time/core.h>

// TODO: This is hardcoded until we append the data.img right after
// the kernel.img
static char *dataimg_base = (char *) 0xA0000;


static fs_inode_t *ext2_def_lookup(fs_inode_t *parent, char *name) {
    return NULL;
}

static int ext2_def_mkdir(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    return 0;
}

static int ext2_def_rmdir(fs_inode_t *parent, fs_dentry_t *dentry) {
    return 0;
}

static int ext2_def_mkregfile(fs_inode_t *parent, fs_dentry_t *dentry, fs_access_mode_t mode) {
    return 0;
}

static int ext2_def_rmregfile(fs_inode_t *parent, fs_dentry_t *dentry) {
    return 0;
}

static int ext2_def_open(fs_inode_t *inode, fs_file_t *f) {
    return 0;
}

static int ext2_def_close(fs_inode_t *inode, fs_file_t *f) {
    return 0;
}

static int ext2_listdir(fs_file_t *f, uint32_t offset, fs_listdir_t *dirlist, uint32_t listlen) {
    return 0;
}

static fs_inode_t *alloc_inode(fs_superblock_t *sb) {
    fs_inode_t *inode = calloc(1, sizeof(fs_inode_t));
    if (inode == NULL) {
        error("No memory available for ext2_inode_t structure");
        return NULL;
    }

    inode->sb = sb;

    inode->ops.lookup = ext2_def_lookup;
    inode->ops.mkdir = ext2_def_mkdir;
    inode->ops.rmdir = ext2_def_rmdir;
    inode->ops.mkregfile = ext2_def_mkregfile;
    inode->ops.rmregfile = ext2_def_rmregfile;

    inode->fops.open = ext2_def_open;
    inode->fops.close = ext2_def_close;
    inode->fops.listdir = ext2_listdir;
    return (fs_inode_t *) inode;
}

static void free_inode(fs_inode_t *inode) {
    free(inode);
}

static int unmount(fs_mount_t *fsm) {
    free(fsm->sb);
    return 0;
}

static inline uint32_t get_num_bgs(ext2_sb_t *sb) {
    uint32_t n = sb->info.blocks_count / sb->info.blocks_per_group;
    if (sb->info.blocks_count % sb->info.blocks_per_group > 0) {
        n++;
    }
    return n;
}

static int get_bg_desc(ext2_sb_t *sb, uint32_t index, ext2_bg_desc_t *bgd) {
    if (index >= get_num_bgs(sb)) {
        return -1;
    }
    memcpy(bgd,
            dataimg_base + EXT2_BG_DESC_OFFSET + sizeof(ext2_bg_desc_t) * index,
            sizeof(ext2_bg_desc_t));
    return 0;
}

static bool is_valid_superblock(ext2_sb_t *sb) {
    if (sb->info.magic != EXT2_SB_MAGIC) {
        error("Invalid ext2 magic number");
        return false;
    }

    // Maybe in the future...
    if (sb->info.log_block_size > 0) {
        error("%lu bytes block size not supported", (uint32_t) 1024 << sb->info.log_block_size);
        return false;
    }

    return true;
}

static int populate_ext2_superblock(ext2_sb_t *sb, fs_mount_t *m) {
    memcpy(&sb->info, dataimg_base + EXT2_SB_OFFSET, sizeof(ext2_sb_info_t));

    debug("Ext2 superblock info:");
    debug("  magic: 0x%X", sb->info.magic);
    debug("  inodes_count: %lu", sb->info.inodes_count);
    debug("  blocks_count: %lu", sb->info.blocks_count);
    debug("  block_size: %lu", (uint32_t) 1024 << sb->info.log_block_size);
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
        ext2_bg_desc_t bgd;
        get_bg_desc(sb, i, &bgd);

        verbose("  #%d:", i);
        verbose("    block_bitmap: %lu", bgd.block_bitmap);
        verbose("    inode_bitmap: %lu", bgd.inode_bitmap);
        verbose("    inode_table: %lu", bgd.inode_table);
        verbose("    free_blocks_count: %u", bgd.free_blocks_count);
        verbose("    inode_bitmap: %u", bgd.free_inodes_count);
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

    return 0;

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
