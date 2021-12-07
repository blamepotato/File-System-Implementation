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

extern unsigned char *disk;
extern struct ext2_super_block *sb;
extern struct ext2_group_desc *gd;
extern struct ext2_inode *inode_table;
extern unsigned char *block_bitmap;
extern unsigned char *inode_bitmap;

extern pthread_mutex_t sb_lock;
extern pthread_mutex_t gd_lock;
extern pthread_mutex_t inode_table_lock;
extern pthread_mutex_t block_bitmap_lock;
extern pthread_mutex_t inode_bitmap_lock;

char* get_source(char* src_copy, int* error){
    // saves the content of a file into a pointer 
	FILE *fp = fopen(src_copy, "r");

	if (fp == NULL) {
        *error = ENOENT;
		return 0;
	}
    // https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
	fseek(fp, (long)0, SEEK_END);

	long long size = (long long)ftell(fp);
	rewind(fp);

	char* ptr = calloc(1, size + 1);
	if (fread(ptr, size, 1, fp) != 1) {
        *error = ENOENT;
		return 0;
	}
	fclose(fp);

	return ptr;
}

void update_block_bitmap_in_rm(struct ext2_inode* inode_dir){
    for (int i = 0; i < inode_dir->i_blocks / 2; i++){
        int block_num = inode_dir->i_block[i];
        int count = 1;
        //block_num starts at 1?
        for (int byte=0; byte<(128/8); byte++){
            for (int bit=0; bit<8; bit++){
                if (count == block_num){
                    block_bitmap[byte] &= (0<<bit);
                    //should end these two for loop.
                }
                count++;
            }
        }
        sb->s_free_blocks_count++;
        gd->bg_free_blocks_count++;
    }
}

char* escape_path(char* path, int* error, int* has_slash){
    /*
     *  returns a path with trailing slashes trimmed 
     *  and some error checking 
     */
    int length = strlen(path);
    char* trimmed_path = calloc(length + 1, sizeof(char));
    // empty input
    if(length == 0){
        *error = ENOENT;
        return 0;
    }
    // not an absolute path
    if (path[0] != '/'){
        *error = ENOENT;
        return 0;
    }
    // mkdir root 
    if (length == 1){
        *error = EEXIST;
        return 0;
    }
    // get rid of extra slashes 
    char prev = '\0';
    for(int i = 0; i < length; i++){
        if(prev != '/' || path[i] != '/'){
            strncat(trimmed_path, &path[i], 1);
            prev = path[i];
        }  
    }
    if(trimmed_path[strlen(trimmed_path) - 1] == '/'){
        *has_slash = 1;
        trimmed_path[strlen(trimmed_path) - 1] = '\0';
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

unsigned int find_last_inode(char *dir_path, int* error){
    // Find inode of the last dir in dir_path
    // exits with proper error if one of them does not exist or is a file
    int length = strlen(dir_path);
    char* current_path = calloc(length + 1, sizeof(char));
    strncpy(current_path, dir_path, length);
    char* current_name = calloc(length + 1, sizeof(char));
    get_curr_dir_name(&current_path, &current_name);
    unsigned int inode_index = EXT2_ROOT_INO - 1;
    struct ext2_inode* inode_entry = (struct ext2_inode*)(&inode_table[inode_index]);

    while(current_path){
        if(inode_entry->i_mode & EXT2_S_IFDIR){
            struct ext2_dir_entry* dir_entry = get_dir_entry(inode_entry, current_name, error);
            if (*error != 0){
                return 0;
            }
            inode_index = dir_entry->inode - 1;
			inode_entry = (struct ext2_inode*)(&inode_table[inode_index]);
        }
        else{
            *error = ENOENT;
            return 0; 
        }
        get_curr_dir_name(&current_path, &current_name);
    }
    //return inode_index;
    return inode_index;
}

void get_curr_dir_name(char** current_path, char** current_name){
    // modify current_path, current_name
    // e.g. current_path = "/foo/bar/lol/"
    // returns "foo", and modifies current_path to /bar/lol/
    if (strlen(*current_path) <= 1){
       *current_name = NULL;
       *current_path = NULL;
       return;
    }
    *current_path += 1;
    char* temp = calloc(strlen(*current_path), sizeof(char));
    strcpy(temp, *current_path);
    char* token = strtok(temp, "/");
    strcpy(*current_name, token);
    *current_path += strlen(token);
    free(temp);
}



struct ext2_dir_entry* get_dir_entry(struct ext2_inode* inode, char * current_name, int* error){
    int block_num;
    struct ext2_dir_entry *dir_entry;
    for(int i = 0; i < inode->i_blocks / 2; i++){
        block_num = inode->i_block[i];
        dir_entry = (struct ext2_dir_entry *) (disk + 1024 * block_num);
        int used_size = 0;
        //https://piazza.com/class/ks5i8qv0pqn139?cid=736
        while (used_size < EXT2_BLOCK_SIZE){

            char name[dir_entry->name_len + 1];
            name[dir_entry->name_len] = '\0';
            for(int i = 0;i < dir_entry->name_len;i++){
                name[i] = dir_entry->name[i];
            }

            if (strcmp(name, current_name) == 0 && dir_entry->file_type != EXT2_FT_DIR){
                //The file is not a directory file.
                *error = ENOENT;
                return 0;

            } else if (strcmp(name, current_name) == 0 && dir_entry->file_type == EXT2_FT_DIR){
                return dir_entry;
            }
            used_size += dir_entry->rec_len;
            dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
        }
    }
    //The current_name is not exist.
    *error = ENOENT;
    return 0;
}


int check_current_inode(unsigned int inode, char* current_name){
    // 1 = dir 2 = file 0 = not found 
    struct ext2_inode ext2_inode = inode_table[inode];
    int block_num;
    struct ext2_dir_entry *dir_entry;
    for(int i = 0; i < ext2_inode.i_blocks / 2; i++){
        block_num = ext2_inode.i_block[i];
        dir_entry = (struct ext2_dir_entry *) (disk + 1024 * block_num);
        int used_size = 0;
        //https://piazza.com/class/ks5i8qv0pqn139?cid=736
        while (used_size < EXT2_BLOCK_SIZE){
            char name[dir_entry->name_len + 1];
            name[dir_entry->name_len] = '\0';
            for(int i = 0;i < dir_entry->name_len;i++){
                name[i] = dir_entry->name[i];
            }

            if (strcmp(name, current_name) == 0 && dir_entry->file_type != EXT2_FT_DIR){
            
                return 2;
            } else if (strcmp(name, current_name) == 0 && dir_entry->file_type == EXT2_FT_DIR){
                return 1;
            }
            used_size += dir_entry->rec_len;
            dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
        }
    }
    return 0;
}

int find_an_unused_block(){
    //We call this if and only if we must use a new unused block.
    int count = 0;
    for (int byte=0; byte<(128/8); byte++){
        for (int bit=0; bit<8; bit++){
            if ((block_bitmap[byte]&(1<<bit)) == 0){
                block_bitmap[byte] |= (1<<bit);
                sb->s_free_blocks_count--;
                gd->bg_free_blocks_count--;
                return count + 1;
            }
            count++;
        }
    }
    return 0;
}

int find_an_unused_inode(){
    //We call this helper function if and only if we must use a new unused inode.
    int count = 0;
    for (int byte=0; byte<(32/8); byte++){
        for (int bit=0; bit<8; bit++){
            //Skip the reserved blocks.
            if ((count >= EXT2_GOOD_OLD_FIRST_INO) && ((inode_bitmap[byte]&(1<<bit))==0)){
                inode_bitmap[byte] |= (1<<bit);
                sb->s_free_inodes_count--;
                gd->bg_free_inodes_count--;
                return count + 1;
            }
            count++;
        }
    }
    //Should not be here, the blocks are all in used except for reserved blocks
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
    dir_entry->inode = inode;
    dir_entry->rec_len = 1012;
    dir_entry->name_len = 2;
    dir_entry->file_type = EXT2_FT_DIR;
    dir_entry->name[0] = '.';
    dir_entry->name[1] = '.';
    dir_entry->name[2] = '\0';
    inode_table[inode-1].i_links_count++;
    return;
}

void init_new_dir_in_new_block(struct ext2_dir_entry *dir_entry, char* dir_name, int parent_inode){
    dir_entry->inode = find_an_unused_inode();
    dir_entry->rec_len = 1000;
    dir_entry->name_len = strlen(dir_name);
    dir_entry->file_type = EXT2_FT_DIR;
    for (int i=0; i<strlen(dir_name); i++){
        dir_entry->name[i] = dir_name[i];
    }
    dir_entry->name[strlen(dir_name)] = '\0';

    //find an unused block and init.
    struct ext2_inode ext2_inode = inode_table[dir_entry->inode];
    //find an unused block and add it to inode info.
    int unused_block_num = find_an_unused_block();

    //Initialize .
    //might have problem here.
    struct ext2_dir_entry * new_dir_entry = (struct ext2_dir_entry *) (disk + 1024 * unused_block_num);
    init_first_dir(new_dir_entry, dir_entry->inode);

    //Initialize ..
    dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
    init_second_dir(dir_entry, parent_inode);

    update_inode_blocks(&ext2_inode, unused_block_num);
}

void init_new_dir_in_old_block(struct ext2_dir_entry * dir_entry, char* dir_name, unsigned short rec_len, int parent_inode){
    dir_entry->inode = find_an_unused_inode();
    dir_entry->rec_len = rec_len;
    dir_entry->file_type = EXT2_FT_DIR;
    dir_entry->name_len = strlen(dir_name);
    for (int i=0; i<strlen(dir_name); i++){
        dir_entry->name[i] = dir_name[i];
    }
    dir_entry->name[strlen(dir_name)] = '\0';

    //find an unused block and add it to inode info.
    int unused_block_num = find_an_unused_block();
    //Initialize .
    //might have problem here.
    struct ext2_dir_entry * new_dir_entry = (struct ext2_dir_entry *) (disk + 1024 * unused_block_num);

    init_first_dir(new_dir_entry, dir_entry->inode);

    //Initialize ..
    new_dir_entry = (struct ext2_dir_entry *) (((char*) new_dir_entry)+ new_dir_entry->rec_len);
    init_second_dir(new_dir_entry, parent_inode);

    struct ext2_inode* ext2_inode = &inode_table[dir_entry->inode - 1];
    //ext2_inode.i_blocks = 2;
    //ext2_inode.i_block[0] = unused_block_num;
    update_inode_blocks(ext2_inode, unused_block_num);

    gd->bg_used_dirs_count++;
}

void update_inode_blocks(struct ext2_inode *inode, int unused_block_num){
    inode->i_mode = EXT2_S_IFDIR;
    inode->i_uid = 0;
    inode->i_size = EXT2_BLOCK_SIZE;
    inode->i_dtime = 0;;
    inode->i_gid = 0;
    inode->i_links_count = 2;
    inode->osd1 = 0;
    inode->i_generation = 0;
    inode->i_file_acl = 0;
    inode->i_dir_acl = 0;
    inode->i_faddr = 0;
    inode->extra[0] = 0;
    inode->extra[1] = 0;
    inode->extra[2] = 0;

    inode->i_block[0] = unused_block_num;
    inode->i_blocks = 2;
}