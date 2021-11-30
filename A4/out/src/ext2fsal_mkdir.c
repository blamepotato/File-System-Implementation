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
    struct ext2_dir_entry *new;
    unsigned int inode = find_last_inode(dir_path);
    
    //Error checking
    if (inode == -2){
        //If any component on the path to the location where the final directory is 
        //to be created does not exist (or is not a directory) then this operation should 
        //return an appropriate error: ENOENT.
        return ENOENT;
    } else if (inode == -1){
        //If the specified directory already exists, 
        //then this operation should return EEXIST.
        return EEXIST;
    }
    //The following variables should be set to be extern?
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + 1024*2);
    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + 1024 * gd->bg_inode_table);
    struct ext2_inode ext2_inode = inode_table[inode];
    unsigned char *block_bitmap = (unsigned char *) (disk + 1024 * gd->bg_block_bitmap);


    int last_block = (ext2_inode.i_blocks / 2) - 1;
    int block_num = ext2_inode.i_block[last_block];
    struct ext2_dir_entry *dir_entry = (struct ext2_dir_entry *) (disk + 1024 * block_num);
    int used_size = 0;
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
                int unused;
                for (int byte=0; byte<(128/8); byte++){
                    for (int bit=0; bit<8; bit++){
                        if ((block_bitmap[byte]&(1<<bit)) == 0){
                            unused = 8 * byte + bit;
                        }
                    }
                }

                //Initialize .
                dir_entry = (struct ext2_dir_entry *) (disk + 1024 * unused);
                //? should not be inode? not sure
                dir_entry->inode = inode;
                dir_entry->rec_len = 12;
                dir_entry->name_len = 1;
                dir_entry->file_type = EXT2_FT_DIR;
                dir_entry->name[0] = '.';
                dir_entry->name[1] = '\0';

                //Initialize ..
                dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
                //Have problem here, I cannot get parent's inode.
                //dir_entry->inode = ;
                dir_entry->rec_len = 12;
                dir_entry->name_len = 2;
                dir_entry->file_type = EXT2_FT_DIR;
                dir_entry->name[0] = '.';
                dir_entry->name[1] = '.';
                dir_entry->name[2] = '\0';

                //Initialize added directory.
                dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
                dir_entry->inode = inode;
                dir_entry->rec_len = 1000;
                dir_entry->name_len = strlen(dir_name);
                dir_entry->file_type = EXT2_FT_DIR;
                for (int i=0; i<strlen(dir_name); i++){
                    dir_entry->name[i] = dir_name[i];
                }
                dir_entry->name[strlen(dir_name)] = '\0';

            } else{
                dir_entry->rec_len = size;
                //still have available space.
                //Initialize a directory entry and add it to the last
                new = (struct ext2_dir_entry *) (((char*) dir_entry) + size);
                new->inode = inode;
                new->rec_len = tmp;
                new->file_type = EXT2_FT_DIR;
                new->name_len = strlen(dir_name);
                for (int i=0; i<strlen(dir_name); i++){
                    new->name[i] = dir_name[i];
                }
                new->name[strlen(dir_name)] = '\0';
            }
        }
        dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
    }
    return 0;
}

