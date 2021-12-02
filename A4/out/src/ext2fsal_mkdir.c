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
    int error = 0;
    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);
    path_copy[strlen(path)] = '\0';

    char* trimmed_path = escape_path(path_copy, &error);
    if(error != 0){
        return error;
    }
    char** path_and_name = get_path_and_name(trimmed_path);
    char* dir_path = path_and_name[0];
    char* dir_name = path_and_name[1];
    // 2. Validate path 
    unsigned int inode = find_last_inode(dir_path, &error);
    if(error != 0){
        return error;
    }
    
    // 3. mkdir
    struct ext2_dir_entry *new;
    if (inode == -1){
        //If the specified directory already exists, 
        //then this operation should return EEXIST.
        return EEXIST;
    }
    //The following variables should be set to be extern?
    //struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + 1024*2);
    //struct ext2_inode *inode_table = (struct ext2_inode *) (disk + 1024 * gd->bg_inode_table);
    struct ext2_inode ext2_inode = inode_table[inode];


    int last_block = (ext2_inode.i_blocks / 2) - 1;
    int block_num = ext2_inode.i_block[last_block];
    struct ext2_dir_entry *dir_entry = (struct ext2_dir_entry *) (disk + 1024 * block_num);
    int used_size = 0;
    //.'s inode
    int itself_inode = dir_entry->inode;
    //..'s inode
    int parent_inode = ((struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len))->inode;
    //https://piazza.com/class/ks5i8qv0pqn139?cid=736
    while (used_size < EXT2_BLOCK_SIZE){
        used_size += dir_entry->rec_len;
        if (used_size == EXT2_BLOCK_SIZE){

            //dir_entry is the last one at this time.
            int size = 8 + strlen(dir_entry->name);

            //make it be multiple of 4
            size += size % 4;

            //Left size.
            int tmp = dir_entry->rec_len - size;
            if (tmp < 8 + strlen(dir_name)){
                ///full
                //make a new directory block. (how?)
                //Initialize . , .. and its directory block.
                //update block_bitmap and inode_bitmap

                //find an unused block and add it to inode info.
                int unused_block_num = find_an_unused_block();

                //Initialize .
                //might have problem here.
                dir_entry = (struct ext2_dir_entry *) (disk + 1024 * unused_block_num);
                init_first_dir(dir_entry, inode);

                //Initialize ..
                dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
                //Have problem here, I cannot get parent's inode.
                init_second_dir(dir_entry, parent_inode);

                //Initialize added directory.
                dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
                init_new_dir_in_new_block(dir_entry, dir_name, itself_inode);

                //update inode info.
                update_inode_blocks(&ext2_inode, unused_block_num);
            } else{
                dir_entry->rec_len = size;
                //still have available space.
                //Initialize a directory entry and add it to the last
                new = (struct ext2_dir_entry *) (((char*) dir_entry) + size);
                init_new_dir_in_old_block(new, dir_name, tmp, itself_inode);
            }
        }
        dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
    }
    return 0;
}

