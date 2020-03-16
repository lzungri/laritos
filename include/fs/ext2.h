#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <fs/vfs/types.h>
#include <component/blockdev.h>

/**
 * -----------------------------------------------
 * Most structures were ported from Linux fs/ext2.h
 * -----------------------------------------------
 */


#define EXT2_SB_OFFSET 1024

#define EXT2_SB_MAGIC 0xEF53

/*
 * Constants relative to the data blocks
 */
#define EXT2_NDIR_BLOCKS        12
#define EXT2_IND_BLOCK          EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK         (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK         (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS           (EXT2_TIND_BLOCK + 1)

/*
 * Special inode numbers
 */
#define EXT2_BAD_INO         1  /* Bad blocks inode */
#define EXT2_ROOT_INO        2  /* Root inode */
#define EXT2_BOOT_LOADER_INO     5  /* Boot loader inode */
#define EXT2_UNDEL_DIR_INO   6  /* Undelete directory inode */

#define EXT2_INODE_SIZE 128
#define EXT2_NAME_LEN 255

#define EXT2_FT_UNKNOWN  0      // Unknown File Type
#define EXT2_FT_REG_FILE 1      // Regular File
#define EXT2_FT_DIR     2       // Directory File
#define EXT2_FT_CHRDEV  3       // Character Device
#define EXT2_FT_BLKDEV  4       // Block Device
#define EXT2_FT_FIFO    5       // Buffer File
#define EXT2_FT_SOCK    6       // Socket File
#define EXT2_FT_SYMLINK 7       // Symbolic Link


/**
 * Block group descriptor on the disk
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

/**
 * Structure of a superblock on the disk
 */
typedef struct ext2_sb_info {
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
    uint32_t block_size;
    uint8_t block_size_bits;
    uint32_t addr_per_block;
    uint8_t addr_per_block_bits;
    uint32_t num_bg_descs;
    ext2_bg_desc_t *bg_descs;

    blockdev_t *dev;
} ext2_sb_t;

/*
 * The new version of the directory entry.  Since EXT2 structures are
 * stored in intel byte order, and the name_len field could never be
 * bigger than 255 chars, it's safe to reclaim the extra byte for the
 * file_type field.
 */
typedef struct {
    uint32_t  inode;          /* Inode number */
    /**
     * 16bit unsigned displacement to the next directory entry from the start
     * of the current directory entry. This field must have a value at least equal
     * to the length of the current record.
     *
     * The directory entries must be aligned on 4 bytes boundaries and there cannot
     * be any directory entry spanning multiple data blocks. If an entry cannot
     * completely fit in one block, it must be pushed to the next data block and
     * the rec_len of the previous entry properly adjusted.
     */
    uint16_t  rec_len;
    /**
     * 8bit unsigned value indicating how many bytes of character data are
     * contained in the name.
     */
    uint8_t    name_len;
    uint8_t    file_type;
    char    name[];         /* File name, up to EXT2_NAME_LEN */
} ext2_direntry_t;

/*
 * Structure of an inode on the disk
 */
typedef struct {
    uint16_t  mode;     /* File mode */
    uint16_t  uid;      /* Low 16 bits of Owner Uid */
    uint32_t  size;     /* Size in bytes */
    uint32_t  atime;    /* Access time */
    uint32_t  ctime;    /* Creation time */
    uint32_t  mtime;    /* Modification time */
    uint32_t  dtime;    /* Deletion Time */
    uint16_t  gid;      /* Low 16 bits of Group Id */
    uint16_t  links_count;  /* Links count */
    /**
     * 32-bit value representing the total number of 512-bytes blocks reserved
     * to contain the data of this inode, regardless if these blocks are used or
     * not. The block numbers of these reserved blocks are contained in the i_block
     * array.
     *
     * Since this value represents 512-byte blocks and not file system blocks, this
     * value should not be directly used as an index to the i_block array. Rather,
     * the maximum index of the i_block array should be computed from
     *      i_blocks / ((1024<<s_log_block_size)/512),
     * or once simplified, i_blocks/(2<<s_log_block_size).
     **/
    uint32_t  blocks;   /* Blocks count */
    uint32_t  flags;    /* File flags */
    uint32_t  i_reserved1;
    uint32_t  block[EXT2_N_BLOCKS];/* Pointers to blocks */
    uint32_t  generation;   /* File version (for NFS) */
    uint32_t  file_acl; /* File ACL */
    uint32_t  dir_acl;  /* Directory ACL */
    uint32_t  faddr;    /* Fragment address */
} ext2_inode_data_t;

typedef struct {
    fs_inode_t parent;


} ext2_inode_t;
