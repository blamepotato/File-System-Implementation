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

char* escape_path(char* path){
    int length = strlen(path);
    // empty input
    if(length == 0){
        return ENOENT;
    }
    // not an absolute path
    if (path[0] != '/'){
        return ENOENT;
    }
    // mkdir root 
    if (length == 1){
        return EEXIST;
    }
    // get rid of trailing slashes 
    char new_path[length + 1];
    memset(new_path, '\0', length + 1);
    strncpy(new_path, path, length);
    int last_idx = length - 1;
    while(new_path[last_idx] == '/'){
        new_path[last_idx] = '\0';
        last_idx--;
    }
    
    return new_path;
    
}
