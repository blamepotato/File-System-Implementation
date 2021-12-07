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

extern unsigned char *disk;
extern struct ext2_super_block *sb;
extern struct ext2_group_desc *gd;
extern struct ext2_inode *inode_table;
extern unsigned char *block_bitmap;
extern unsigned char *inode_bitmap;


extern pthread_mutex_t sb_lock;
extern pthread_mutex_t gd_lock;
extern pthread_mutex_t inode_table_lock;
extern pthread_mutex_t block_bitmap_lock;
extern pthread_mutex_t inode_bitmap_lock;
int32_t ext2_fsal_rm(const char *path)
{
    // 1. check and reformat input path
    int error = 0;
    int has_slash = 0;
    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);
    path_copy[strlen(path)] = '\0';
    char* trimmed_path = escape_path(path_copy, &error, &has_slash);

    if (strlen(trimmed_path) == 1){
        return EISDIR;
    }

    if(error != 0){
        return error;
    }

    char** path_and_name = get_path_and_name(trimmed_path);
    char* dir_path = path_and_name[0];
    char* dir_name = path_and_name[1];
    if(strlen(dir_name) > EXT2_NAME_LEN){
        return ENAMETOOLONG;
    }
    // 2. Validate path 
    unsigned int inode = find_last_inode(dir_path, &error);
    if(error != 0){
        return error;
    }
    
    int check = check_current_inode(inode, dir_name);
    // not found
    if (check == 0){
        return ENOENT;
    }
    // found a dir
    else if (check == 1){
        return EISDIR;
    }
    // found a file
    else if (check == 2 && has_slash){
        return ENOENT;
    }
    
    //file dir entry
    //before file dir entry
    // update: sb gd imap bmap inodetable

    struct ext2_inode ext2_inode = inode_table[inode];
    int block_num;
    struct ext2_dir_entry *dir_entry;
    for(int i = 0; i < ext2_inode.i_blocks / 2; i++){
        block_num = ext2_inode.i_block[i];
        dir_entry = (struct ext2_dir_entry *) (disk + 1024 * block_num);

        int used_size = 0;
        //https://piazza.com/class/ks5i8qv0pqn139?cid=736
        while (used_size < EXT2_BLOCK_SIZE){
            char name[dir_entry->name_len + 1];
            name[dir_entry->name_len] = '\0';
            for (int i = 0;i < dir_entry->name_len;i++){
                name[i] = dir_entry->name[i];
            }

            if (strcmp(name, dir_name) == 0 && dir_entry->file_type != EXT2_FT_DIR){
            
                return 2;
            } else if (strcmp(name, dir_name) == 0 && dir_entry->file_type == EXT2_FT_DIR){
                return 1;
            }
            used_size += dir_entry->rec_len;
            dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);

            //how to set i_dtime
            //first get a dirctory with the same name, and then get a file with the same name.
            //what should I do.
    }

    return 0;
}