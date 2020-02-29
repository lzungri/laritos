#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <fs/vfs/types.h>

#define EXT2_SB_OFFSET 1024
#define EXT2_BG_DESC_OFFSET (EXT2_SB_OFFSET + 1024)

#define EXT2_SB_MAGIC 0xEF53

/**
 * Block group descriptor
 */
typedef struct {
    uint32_t  block_bitmap;        /* Blocks bitmap block */
    uint32_t  inode_bitmap;        /* Inodes bitmap block */
    uint32_t  inode_table;     /* Inodes table block */
    uint16_t  free_blocks_count;   /* Free blocks count */
    uint16_t  free_inodes_count;   /* Free inodes count */
    uint16_t  used_dirs_count; /* Directories count */
    uint16_t  pad;
    uint32_t  reserved[3];
} ext2_bg_desc_t;

typedef struct ext2_sb_info {
    /**
     * Taken from Linux source
     */
    uint32_t  inodes_count;     /* Inodes count */
    uint32_t  blocks_count;     /* Blocks count */
    uint32_t  r_blocks_count;   /* Reserved blocks count */
    uint32_t  free_blocks_count;    /* Free blocks count */
    uint32_t  free_inodes_count;    /* Free inodes count */
    uint32_t  first_data_block; /* First Data Block */
    uint32_t  log_block_size;   /* Block size */
    uint32_t  log_frag_size;    /* Fragment size */
    uint32_t  blocks_per_group; /* # Blocks per group */
    uint32_t  frags_per_group;  /* # Fragments per group */
    uint32_t  inodes_per_group; /* # Inodes per group */
    uint32_t  mtime;        /* Mount time */
    uint32_t  wtime;        /* Write time */
    uint16_t  mnt_count;        /* Mount count */
    uint16_t  max_mnt_count;    /* Maximal mount count */
    uint16_t  magic;        /* Magic signature */
    uint16_t  state;        /* File system state */
    uint16_t  errors;       /* Behaviour when detecting errors */
    uint16_t  minor_rev_level;  /* minor revision level */
    uint32_t  lastcheck;        /* time of last check */
    uint32_t  checkinterval;    /* max. time between checks */
    uint32_t  creator_os;       /* OS */
    uint32_t  rev_level;        /* Revision level */
    uint16_t  def_resuid;       /* Default uid for reserved blocks */
    uint16_t  def_resgid;       /* Default gid for reserved blocks */

} ext2_sb_info_t;

typedef struct {
    fs_superblock_t parent;

    ext2_sb_info_t info;
} ext2_sb_t;
