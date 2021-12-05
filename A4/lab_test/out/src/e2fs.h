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

#ifndef CSC369_E2FS_H
#define CSC369_E2FS_H

#include "ext2.h"
#include <string.h>

/**
 * TODO: add in here prototypes for any helpers you might need.
 * Implement the helpers in e2fs.c
 */

extern char* escape_path(char* path, int* error);
extern char** get_path_and_name(char* trimmed_path);
extern unsigned int find_last_inode(char *dir_path, int* error);
extern void get_curr_dir_name(char** current_path, char** current_name);
extern struct ext2_dir_entry* get_dir_entry(struct ext2_inode* inode, char * current_name, int* error); 
extern int find_an_unused_block();
extern int find_an_unused_inode();
extern void init_first_dir(struct ext2_dir_entry * dir_entry, int inode);
extern void init_second_dir(struct ext2_dir_entry * dir_entry, int inode);
extern void init_new_dir_in_new_block(struct ext2_dir_entry * dir_entry, char* dir_name, int parent_inode);
extern void init_new_dir_in_old_block(struct ext2_dir_entry * dir_entry, char* dir_name, unsigned short rec_len, int parent_inode);
extern void update_inode_blocks(struct ext2_inode *inode, int unused_block_num);
extern char* get_source(char* src_copy, int* error);

#endif