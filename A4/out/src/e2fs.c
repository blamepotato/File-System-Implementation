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

    if(length == 0){
        return ENOENT;
    }
    if (path[0] != '/'){
        return ENOENT;
    }
    if (length == 1){
        return EEXIST;
    }
    
}
