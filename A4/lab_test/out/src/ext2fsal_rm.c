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
            //might have problem here.
            before_that_entry = dir_entry;
            dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
        }
    }

    printf("that entry: %d\n", that_entry->inode);
    printf("before that entry: %d\n", before_that_entry->inode);
    //that_dir and before_that_dir should be what we want
    //update inode bitmap
    int count = 1;
    for (int byte=0; byte<(32/8); byte++){
        for (int bit=0; bit<8; bit++){
            //Skip the reserved blocks.
            if (count == that_entry->inode){
                inode_bitmap[byte] &= (0<<bit);
                //break? better to end the for loops here.
            }
            count++;
        }
    }

    //links can also be deleted, how?

    //// update: sb gd imap bmap inodetable
    struct ext2_inode* inode_dir = &inode_table[that_entry->inode - 1];
    //block bitmap, zero the blocks that that_file uses.
    update_block_bitmap_in_rm(inode_dir);

    if (used_size == 0){
        //The file is the first file in the block.
        that_entry->inode = 0;
    } else {
        //Other
        before_that_entry->rec_len += that_entry->rec_len;
    }

    inode_table[that_entry->inode].i_dtime = time(NULL);

    sb->s_free_inodes_count++;
    gd->bg_free_inodes_count++;
    //what if the file is the only file in the block and we delete it, the block should be free?
    
    //check if the inode number is 0 or not.
    return 0;
}