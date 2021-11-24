/*
 *------------
 * This code is provided solely for the personal and private use of
 * students taking the CSC369H5 course at the University of Toronto.
 * Copying for purposes other than this use is expressly prohibited.
 * All forms of distribution of this code, whether as given or with
 * any changes, are expressly prohibited.
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019 MCS @ UTM
 * -------------
 */

#include "ext2fsal.h"
#include "e2fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ext2.h"

int32_t ext2_fsal_mkdir(const char *path)
{

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);

    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + 1024*2);
    printf("Block group:\n");
    printf("block bitmap: %d\n", gd->bg_block_bitmap);
    printf("inode bitmap: %d\n", gd->bg_inode_bitmap);
    printf("inode table: %d\n", gd->bg_inode_table);
    printf("free blocks: %d\n", sb->s_free_blocks_count);
    printf("free inodes: %d\n", sb->s_free_inodes_count);
    printf("used_dirs: %d\n", gd->bg_used_dirs_count);   
    
    // 1. check and reformat input path 
    // 2. Validate path 
    // 3. mkdir (how?)
   

    return 0;
}