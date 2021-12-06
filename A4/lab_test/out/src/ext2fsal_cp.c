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
#include "ext2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern unsigned char *disk;
extern struct ext2_super_block *sb;
extern struct ext2_group_desc *gd;
extern struct ext2_inode *inode_table;
extern unsigned char *block_bitmap;
extern unsigned char *inode_bitmap;

int32_t ext2_fsal_cp(const char *src,
                     const char *dst)
{
    /**
    int error = 0;
    char dst_copy[strlen(dst) + 1];
    strcpy(dst_copy, dst);
    dst_copy[strlen(dst)] = '\0';

    char src_copy[strlen(src) + 1];
    strcpy(src_copy, src);
    dst_copy[strlen(src)] = '\0';

    char* trimmed_dst = escape_path(dst_copy, &error);
    if(error != 0){
        return error;
    }
    char** path_and_name = get_path_and_name(dst_copy);
    char* dst_path = path_and_name[0];
    char* dst_name = path_and_name[1];

    char* source = get_source(src_copy, &error);
    **/
   (void)src;
   (void)dst;
   return 0;
}