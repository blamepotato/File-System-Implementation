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

char* escape_path(char* path){
    /*
     *  returns a path with trailing slashes trimmed 
     *  and some error checking 
     */
    int length = strlen(path);
    // empty input
    if(length == 0){
        exit(ENOENT);
    }
    // not an absolute path
    if (path[0] != '/'){
        exit(ENOENT);
    }
    // mkdir root 
    if (length == 1){
        exit(EEXIST);
    }
    // get rid of trailing slashes 
    char* trimmed_path = calloc(length + 1, sizeof(char));
    strncpy(trimmed_path, path, length);
    int last_idx = length - 1;
    while(trimmed_path[last_idx] == '/'){
        trimmed_path[last_idx] = '\0';
        last_idx--;
    }
    return trimmed_path;
}

char** get_path_and_name(char* trimmed_path){
    /*
     *  return a 2d array path_and_name
     *  e.g. trimmed_path = "/first/second/third/dir"
     *  path_and_name[0] is the path to the dir: /first/second/third/
     *  path_and_name[1] is the name of the dir: dir
     */
    char** path_and_name = calloc(2, sizeof(char*));
    int length = strlen(trimmed_path);
    char* path = calloc(length + 1, sizeof(char));
    char* name = calloc(length + 1, sizeof(char));
    strncpy(path, trimmed_path, length);
    strncpy(name, trimmed_path, length);
    int last_idx = length - 1;
    while(trimmed_path[last_idx] != '/'){
        path[last_idx] = '\0';
        last_idx--;
    }
    name += last_idx + 1;
    path_and_name[0] = path;
    path_and_name[1] = name;
    return path_and_name;
}

unsigned find_last_inode(char *dir_path){
    // Find inode of the last dir in dir_path
    // exits with proper error if one of them does not exist or is a file
    int length = strlen(dir_path);
    char* current_path = calloc(length + 1, sizeof(char));
    strncpy(current_path, dir_path, length);
    current_path += 1;
    char* current_name = calloc(length + 1, sizeof(char));
    get_curr_dir_name(&current_path, &current_name);
    unsigned int inode_index = EXT2_ROOT_INO - 1;
    struct ext2_inode* inode_entry = get_inode_by_index(inode_index);

    while(strlen(current_path) != 0){
        if(inode_entry->i_mode & EXT2_S_IFDIR){
            //Fill this
            //struct ext2_dir_entry* dir_entry = get_dir_entry(inode_entry, );
            //Delete this.
            struct ext2_dir_entry* dir_entry;
            inode_index = dir_entry->inode - 1;
			inode_entry = get_inode_by_index(inode_index);
        }
        else{
            exit(ENOENT);
        }
    }
    return inode_index;
}

void get_curr_dir_name(char** current_path, char** current_name){
    // modify current_path, current_name
    // e.g. current_path = "/foo/bar/lol/"
    // returns "foo", and modifies current_path to /bar/lol/
    if (strlen(*current_path) == 0){
       *current_name = NULL;
       return;
    }
    char* temp = calloc(strlen(*current_path), sizeof(char));
    strcpy(temp, *current_path);
    char* token = strtok(temp, "/");
    strcpy(*current_name, token);
    *current_path += strlen(token) + 1;
    free(temp);
}

struct ext2_inode* get_inode_by_index(unsigned int index){
    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
    return (struct ext2_inode*)(&inode_table[index]);
}

struct ext2_dir_entry* get_dir_entry(struct ext2_inode* inode, char * current_name){
    // TODO: implement this with proper variables
    // exits with proper errors, e.g. exit(ENOENT)
    
    int block_num;
    for(int i = 0; i < inode->i_blocks / 2; i++){
        block_num = inode->i_block[i];
        struct ext2_dir_entry *dir_entry = (struct ext2_dir_entry *) (disk + 1024 * block_num);
        int used_size = 0;
        //https://piazza.com/class/ks5i8qv0pqn139?cid=736
        while (used_size < EXT2_BLOCK_SIZE){
            if (dir_entry->name == current_name && dir_entry->file_type != EXT2_FT_DIR){
                //The file is not a directory file.
                exit(ENOENT);
            } else if (dir_entry->name == current_name && dir_entry->file_type == EXT2_FT_DIR){
                return dir_entry;
            }
            used_size += dir_entry->rec_len;
            dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
        }
    }
    //The current_name is not exist.
    exit(ENOENT);
}

int find_an_unused_block(){
    //By Bbitmap
    int count = 0;
    for (int byte=0; byte<(32/8); byte++){
        for (int bit=0; bit<8; bit++){
            //Skip the reserved blocks.
            if ((count >= EXT2_GOOD_OLD_FIRST_INO) && ((inode_bitmap[byte]&(1<<bit))==0)){
                return 8 * byte + bit;
            }
            count++;
        }
    }
    //Should not be here, the blocks are all in used except for reserved blocks
    return 0;
}

int find_an_unused_inode(){
    for (int byte=0; byte<(128/8); byte++){
        for (int bit=0; bit<8; bit++){
            if ((block_bitmap[byte]&(1<<bit)) == 0){
                return 8 * byte + bit;
            }
        }
    }
    //Should not be here, all inodes are in used.
    return 0;
}

void init_first_dir(struct ext2_dir_entry * dir_entry, int inode){
    dir_entry->inode = inode;
    dir_entry->rec_len = 12;
    dir_entry->name_len = 1;
    dir_entry->file_type = EXT2_FT_DIR;
    dir_entry->name[0] = '.';
    dir_entry->name[1] = '\0';
    return;
}

void init_second_dir(struct ext2_dir_entry * dir_entry, int inode){
    //Have problem here, I cannot get parent's inode.
    dir_entry->inode = inode;
    dir_entry->rec_len = 12;
    dir_entry->name_len = 2;
    dir_entry->file_type = EXT2_FT_DIR;
    dir_entry->name[0] = '.';
    dir_entry->name[1] = '.';
    dir_entry->name[2] = '\0';
    return;
}

void init_new_dir(struct ext2_dir_entry * dir_entry, char* dir_name){
    //update bitmap
    dir_entry->inode = find_an_unused_inode;
    dir_entry->rec_len = 1000;
    dir_entry->name_len = strlen(dir_name);
    dir_entry->file_type = EXT2_FT_DIR;
    for (int i=0; i<strlen(dir_name); i++){
        dir_entry->name[i] = dir_name[i];
    }
    dir_entry->name[strlen(dir_name)] = '\0';
}