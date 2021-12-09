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
#include <sys/stat.h>
#include <math.h>
#include <libgen.h>

extern unsigned char *disk;
extern struct ext2_super_block *sb;
extern struct ext2_group_desc *gd;
extern struct ext2_inode *inode_table;
extern unsigned char *block_bitmap;
extern unsigned char *inode_bitmap;

extern pthread_mutex_t sb_lock;
extern pthread_mutex_t gd_lock;
extern pthread_mutex_t inode_locks[32];
extern pthread_mutex_t block_bitmap_lock;
extern pthread_mutex_t inode_bitmap_lock;

int32_t ext2_fsal_cp(const char *src,
                     const char *dst)
{
    
    int error = 0;
    int has_slash = 0;
    // processing dst
    char dst_copy[strlen(dst) + 1];
    strcpy(dst_copy, dst);
    dst_copy[strlen(dst)] = '\0';
    char* trimmed_dst = escape_path(dst_copy, &error, &has_slash);
    if(error != 0){
        return 100;
        return error;
    }
    // processing src
    char src_copy[strlen(src) + 1];
    strcpy(src_copy, src);
    src_copy[strlen(src)] = '\0';
    if(src_copy[strlen(src_copy) - 1] == '/'){ // src is a dir 
        return EISDIR;
    }
    char* src_name = basename(src_copy);
    if(strcmp(src_copy, src) != 0){
        return 77;
    }
    unsigned int inode;
    int mode; // 1 if create new dir entry, 2 if overwrite dir entry
    char* dst_name;
    // target the a dir 
    if(has_slash){
        mode = 1;
        trimmed_dst[strlen(trimmed_dst)] = '/';
        trimmed_dst[strlen(trimmed_dst)] = '\0';
        inode = find_last_inode(trimmed_dst, &error);
        if(strlen(src_name) > EXT2_NAME_LEN){
            return ENAMETOOLONG;
        } 
        if(error != 0){
            return error;
        }
        if (sb->s_free_inodes_count <= 0) {
            return ENOSPC;
        }

    }else{
        mode = 2;
        char** path_and_name = get_path_and_name(trimmed_dst);
        char* dst_path = path_and_name[0];
        dst_name = path_and_name[1];
        if(strlen(dst_name) > EXT2_NAME_LEN){
            return ENAMETOOLONG;
        } 
        inode = find_last_inode(dst_path, &error);
        if(error != 0){
            return 102;
            return error;
        }
        int check = check_current_inode(inode, dst_name);

        // not found or is a link 
        if (check == 0 || check == 3){
            return 103;
            return ENOENT;
        }
        // found a dir but doesn't have a slash
        else if (check == 1){
            return 104;
            return ENOENT;
        }
        // found a file but has a slash 
        else if (check == 2 && has_slash){
            return 105;
            return ENOENT;
        }
    }
    
    // get ptr and size of src file
    long long size = 0;
    char* source = get_source(src_copy, &size, &error);
    if(error != 0){
    
        return error;
    }
    
    int blocks_needed = floor(size / EXT2_BLOCK_SIZE);

    // Check if there is enough blocks for Data
    if (sb->s_free_blocks_count < blocks_needed){
        return ENOSPC;
    }

    cp_to_blocks(source, src_name, dst_name, inode, blocks_needed, size, mode);

    return 0;
}