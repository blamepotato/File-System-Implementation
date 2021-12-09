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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int32_t ext2_fsal_ln_hl(const char *src,
                        const char *dst)
{
    /**
     * TODO: implement the ext2_ln_hl command here ...
     * src and dst are the ln command arguments described in the handout.
     */

     /* This is just to avoid compilation warnings, remove these 3 lines when you're done. */
    // 1. check and reformat input path
    int error = 0;
    int src_has_slash = 0;
    int dst_has_slash = 0;
    char src_copy[strlen(src) + 1];
    strcpy(src_copy, src);
    src_copy[strlen(src_copy)] = '\0';

    char dst_copy[strlen(dst) + 1];
    strcpy(dst_copy, dst);
    dst_copy[strlen(dst_copy)] = '\0';

    char* src_trimmed_path = escape_path(dst_copy, &error, &src_has_slash);
    if(error != 0){
        return error;
    }

    char* dst_trimmed_path = escape_path(dst_copy, &error, &dst_has_slash);
    if(error != 0){
        return error;
    }

    if (strlen(src_trimmed_path) == 1){
        return EISDIR;
    }

    char** src_path_and_name = get_path_and_name(src_trimmed_path);
    char* src_dir_path = src_path_and_name[0];
    char* src_dir_name = src_path_and_name[1];

    char** dst_path_and_name = get_path_and_name(dst_trimmed_path);
    char* dst_dir_path = dst_path_and_name[0];
    char* dst_dir_name = dst_path_and_name[1];


    if(strlen(src_dir_name) > EXT2_NAME_LEN || strlen(dst_dir_name) > EXT2_NAME_LEN){
        return ENAMETOOLONG;
    }

    // 2. Validate src path 
    unsigned int inode = find_last_inode(src_dir_path, &error);
    if(error != 0){
        return error;
    }

    int check = check_current_inode(inode, src_dir_name);
    if(check == 0){
        return ENOENT;
    }
    if (check == 1){
        return EISDIR;
    }
    else if (check == 3){
        return ENOENT;
    }
    // dst path
    unsigned int dst_inode = find_last_inode(dst_dir_path, &error);
    if(error != 0){
        return error;
    }

    int dst_check = check_current_inode(inode, dst_dir_name);
    if(dst_check != 0){
        return EEXIST;
    }
    


    return 0;
}
