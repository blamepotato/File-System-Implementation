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
    
    // 思路： 找到dir_name 

    
    

    return 0;
}