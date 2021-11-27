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
    char* file_path = path_and_name[0];
    char* file_name = path_and_name[1];

    return 0;
}