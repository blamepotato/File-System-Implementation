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
    // 1. check and reformat input path 
    // 2. Validate path 
    // 3. mkdir (how?)
    char* trimmed_path = escape_path(path);
    char** path_and_name = get_path_and_name(trimmed_path);
    char* dir_path = path_and_name[0];
    char* dir_name = path_and_name[1];
    struct ext2_dir_entry new;
    unsigned int inode = find_last_inode(dir_path);
    
    //should not be here
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + 1024*2);
    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + 1024 * gd->bg_inode_table);
    struct ext2_inode ext2_inode = inode_table[inode];

    int last_block = (ext2_inode.i_blocks / 2) - 1;
    struct ext2_dir_entry *dir_entry = (struct ext2_dir_entry *) (disk + 1024 * last_block);
    int used_size = 0;
    //https://piazza.com/class/ks5i8qv0pqn139?cid=736
    while (used_size < EXT2_BLOCK_SIZE){
        used_size += dir_entry->rec_len;
        if (used_size == EXT2_BLOCK_SIZE){
            //dir_entry is the last one.

        }
        dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
    }


    new.inode = inode;
    //new.rec_len = 
    new.file_type = EXT2_FT_DIR;
    new.name_len = strlen(dir_name);
    for (int i=0; i<strlen(dir_name); i++){
        new.name[i] = dir_name[i];
    }
    new.name[strlen(dir_name)] = '\0';
    return 0;
}

