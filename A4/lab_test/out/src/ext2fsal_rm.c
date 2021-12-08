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
    unsigned int new_inode = -1; 
    int check = check_current_inode(inode, dir_name, &new_inode);
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

    struct ext2_dir_entry *that_entry;
    struct ext2_dir_entry *before_that_entry;
    int used_size;
    int found = 0;

    for(int i = 0; i < ext2_inode.i_blocks / 2; i++){
        if (found == 1){
            break;
        }
        block_num = ext2_inode.i_block[i];
        dir_entry = (struct ext2_dir_entry *) (disk + 1024 * block_num);

        before_that_entry = dir_entry;
        used_size = 0;
        //https://piazza.com/class/ks5i8qv0pqn139?cid=736
        while (used_size < EXT2_BLOCK_SIZE){
            char name[dir_entry->name_len + 1];
            name[dir_entry->name_len] = '\0';
            for (int i = 0;i < dir_entry->name_len;i++){
                name[i] = dir_entry->name[i];
            }

            if (strcmp(name, dir_name) == 0){
                that_entry = dir_entry;
                found = 1;
                break;
            }
            used_size += dir_entry->rec_len;
            before_that_entry = dir_entry;
            dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
        }
    }

    //printf("that entry: %d\n", that_entry->inode);
    //printf("before that entry: %d\n", before_that_entry->inode);
    //that_dir and before_that_dir should be what we want
    
    int deleted_inode = that_entry->inode;

    if (used_size == 0){
        //The file is the first file in the block.
        that_entry->inode = 0;
    } else {
        //Other
        before_that_entry->rec_len += that_entry->rec_len;
    }

    //Here Correct.
    int count = 1;
    int changed = 0;
    for (int byte=0; byte<(32/8); byte++){
        if (changed == 1){
            break;
        }
        for (int bit=0; bit<8; bit++){
            //Skip the reserved blocks.
            if (count == deleted_inode){
                changed = 1;
                //Have problem here.
                inode_bitmap[byte] &= ~(1<<bit);
                break;
            }
            count++;
        }
    }

    struct ext2_inode* inode_dir = &inode_table[that_entry->inode - 1];

    update_block_bitmap_in_rm(inode_dir);

    inode_dir->i_dtime = time(NULL);

    sb->s_free_inodes_count++;
    gd->bg_free_inodes_count++;
    return 0;
}