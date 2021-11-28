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
    /*
     *  returns a path with trailing slashes trimmed 
     *  and some error checking 
     */
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
     *  e.g. trimmed_path = "/first/second/third/file"
     *  path_and_name[0] is the path to the file: /first/second/third/
     *  path_and_name[1] is the name of the file: file
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

//check if the path is valid.

//check if the file/directory is exist or not.