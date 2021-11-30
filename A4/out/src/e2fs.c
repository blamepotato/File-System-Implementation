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
            struct ext2_dir_entry* dir_entry = get_dir_entry();
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

struct ext2_dir_entry* get_dir_entry(){
    // TODO: implement this with proper variables
    // exits with proper errors, e.g. exit(ENOENT)
    
}