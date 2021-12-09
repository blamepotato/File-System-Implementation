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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

extern unsigned char *disk;
extern struct ext2_super_block *sb;
extern struct ext2_group_desc *gd;
extern struct ext2_inode *inode_table;
extern unsigned char *block_bitmap;
extern unsigned char *inode_bitmap;

extern pthread_mutex_t sb_lock;
extern pthread_mutex_t gd_lock;
extern pthread_mutex_t block_bitmap_lock;
extern pthread_mutex_t inode_bitmap_lock;
extern pthread_mutex_t inode_locks[32];


void cp_to_blocks(char* source, char* src_name,char* dst_name, int inode, int blocks_needed, long long size, int mode){
    // overwritting existing file, no need to find new inode
    struct ext2_inode* inode_ptr = &inode_table[inode];
    int currect_block = 0;
    int indirect = 0; // indirect block number 
    int src_bigger = 0; 
    int overwrite = 0;
    struct ext2_dir_entry* file_entry;
    struct ext2_inode* file_inode;

    if(mode == 1){ // create new dir
        file_entry = make_file_entry(src_name, inode);
        file_inode = &inode_table[file_entry->inode];
    }    
    else{ // overwrite file
        overwrite = 1;
        struct ext2_dir_entry** dst_and_last = find_dst_and_last_entry(inode_ptr, dst_name);
        file_entry = dst_and_last[0];
        file_inode = &inode_table[file_entry->inode];
        struct ext2_dir_entry* last_entry = dst_and_last[1];

        int prev_rec_len = (int) file_entry->rec_len;
        file_entry->name_len = strlen(src_name);
        int rec_size = 8 + (int) file_entry->name_len;
        //make it be multiple of 4
        rec_size += (4 - rec_size % 4);
        file_entry->rec_len = rec_size;
        // negative if src len > dst len, positive if src len < dst len
        int diff = prev_rec_len - rec_size;
        last_entry->rec_len += diff;
        file_inode->i_size = size;
        // if dst file is smaller than src, 
        src_bigger = blocks_needed - file_inode->i_blocks / 2 > 0 ? 1 : 0;
    }
    
    for(;currect_block < blocks_needed; currect_block++){
        int block_num;
        if(!overwrite){
            if(currect_block < 12){
                block_num = find_an_unused_block();
                file_inode->i_block[currect_block] = block_num;
            }
            else if(currect_block == 12){
                indirect = find_an_unused_block();
                file_inode->i_block[12] = indirect;
                file_inode->i_blocks += 2;
                block_num = find_an_unused_block();
                unsigned int *indirect_block = (unsigned int *) (disk + indirect * EXT2_BLOCK_SIZE);
                indirect_block[0] = block_num;
            }
            else{
                block_num = find_an_unused_block();
                unsigned int *indirect_block = (unsigned int *) (disk + indirect * EXT2_BLOCK_SIZE);
                indirect_block[currect_block - 12] = block_num;
            }
            file_inode->i_blocks += 2;
            currect_block++;

        }else if(src_bigger){
            indirect = file_inode->i_block[12];
            if(currect_block < 12 && currect_block > file_inode->i_blocks / 2){
                block_num = find_an_unused_block();
                file_inode->i_block[currect_block] = block_num;
            }
            else if(currect_block == 12 && currect_block > file_inode->i_blocks / 2){
                indirect = find_an_unused_block();
                file_inode->i_block[12] = indirect;
                file_inode->i_blocks += 2;
                unsigned int *indirect_block = (unsigned int *) (disk + indirect * EXT2_BLOCK_SIZE);
                indirect_block[0] = block_num;
            }
            else if(currect_block > 12 && currect_block > file_inode->i_blocks / 2){
                block_num = find_an_unused_block();
                unsigned int *indirect_block = (unsigned int *) (disk + indirect * EXT2_BLOCK_SIZE);
                indirect_block[currect_block - 12] = block_num;
            }
            file_inode->i_blocks += 2;
            currect_block++;

        }else{ // dst > src, cleanup blocks 

            indirect = file_inode->i_block[12];

            if(currect_block < 12 && currect_block > blocks_needed){
                rm_block(file_inode->i_block[currect_block]);
            }

            else if(currect_block == 12 && currect_block > blocks_needed){
                rm_block(file_inode->i_block[currect_block]);
                file_inode->i_blocks -= 2;
                unsigned int *indirect_block = (unsigned int *) (disk + indirect * EXT2_BLOCK_SIZE);
                rm_block(indirect_block[0]);
            }
            else if(currect_block > 12 && currect_block > blocks_needed){
                
                unsigned int *indirect_block = (unsigned int *) (disk + indirect * EXT2_BLOCK_SIZE);
                rm_block(indirect_block[currect_block - 12]);
            }
            file_inode->i_blocks -= 2;
            currect_block++;
        }
        // finally... memcpy time 
        char *block = (char*) (disk + block_num * EXT2_BLOCK_SIZE);
        if(currect_block < blocks_needed - 1){
            memcpy(block, &source[currect_block * EXT2_BLOCK_SIZE], EXT2_BLOCK_SIZE);
        }
        else if(currect_block == blocks_needed - 1){
            memcpy(block, &source[currect_block * EXT2_BLOCK_SIZE], size - EXT2_BLOCK_SIZE * (blocks_needed - 1)); // remaining size
        }
    }
}


struct ext2_dir_entry** find_dst_and_last_entry(struct ext2_inode* inode, char* dst_name){
    struct ext2_dir_entry** curr_and_last = calloc(2, sizeof(struct ext2_dir_entry*));
    int block_num;
    struct ext2_dir_entry *dir_entry;

    for(int i = 0; i < inode->i_blocks / 2; i++){
        block_num = inode->i_block[i];
        dir_entry = (struct ext2_dir_entry *) (disk + 1024 * block_num);
        int used_size = 0;
        while (used_size < EXT2_BLOCK_SIZE){

            char name[dir_entry->name_len + 1];
            name[dir_entry->name_len] = '\0';
            for(int i = 0;i < dir_entry->name_len;i++){
                name[i] = dir_entry->name[i];
            }

            if (strcmp(name, dst_name) == 0){
                curr_and_last[0] = dir_entry;
            }

            used_size += dir_entry->rec_len;
            dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
        }
    }
    curr_and_last[1] = dir_entry;
    
    return curr_and_last;
}

struct ext2_dir_entry* make_file_entry(char* src_name, int inode){
    struct ext2_inode ext2_inode = inode_table[inode];
    int last_block = (ext2_inode.i_blocks / 2) - 1;
    int block_num = ext2_inode.i_block[last_block];
    struct ext2_dir_entry *dir_entry = (struct ext2_dir_entry *) (disk + 1024 * block_num);
    int used_size = 0;

    int itself = dir_entry->inode;
    struct ext2_dir_entry *new;
    //https://piazza.com/class/ks5i8qv0pqn139?cid=736
    while (used_size < EXT2_BLOCK_SIZE){
        used_size += dir_entry->rec_len;
        if (used_size == EXT2_BLOCK_SIZE){

            //dir_entry is the last one at this time.
            int size = 8 + (int) dir_entry->name_len;
            //make it be multiple of 4
            size += (4 - size % 4);

            //Left size.
            int tmp = dir_entry->rec_len - size;
            if (tmp < 8 + strlen(src_name)){
                //full
                int unused_block_num = find_an_unused_block();
                new = (struct ext2_dir_entry *) (disk + 1024 * unused_block_num);
                new->file_type = EXT2_FT_REG_FILE;
                new->inode = find_an_unused_inode();
                new->name_len = strlen(src_name);
                for(int i = 0;i < strlen(src_name);i++){
                    new->name[i] = src_name[i];
                }
                new->name[strlen(src_name)] = '\0';
                new->rec_len = 1024;

                struct ext2_inode *inode = &inode_table[new->inode - 1];
                init_an_inode_for_file(inode);

                struct ext2_inode *itself_inode = &inode_table[itself - 1];
                itself_inode->i_block[itself_inode->i_blocks/2] = unused_block_num;
                itself_inode->i_blocks += 2;
                return new;

            } else{
                dir_entry->rec_len = size;
                new = (struct ext2_dir_entry *) (((char*) dir_entry) + size);
                new->file_type = EXT2_FT_REG_FILE;
                new->inode = find_an_unused_inode();
                new->name_len = strlen(src_name);
                for(int i = 0;i < strlen(src_name);i++){
                    new->name[i] = src_name[i];
                }
                new->name[strlen(src_name)] = '\0';
                new->rec_len = tmp;

                struct ext2_inode *inode = &inode_table[new->inode - 1];
                init_an_inode_for_file(inode);
                return new;
            }
        }
        dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
    }

    //Should not be here;
    return 0;
}


void init_an_inode_for_file(struct ext2_inode *inode){
    inode->i_mode = EXT2_S_IFREG;
    inode->i_uid = 0;
    //might have problem here
    inode->i_size = 0;
    inode->i_dtime = 0;
    inode->i_gid = 0;
    inode->i_links_count = 1;
    inode->osd1 = 0;
    inode->i_generation = 0;
    inode->i_file_acl = 0;
    inode->i_dir_acl = 0;
    inode->i_faddr = 0;
    inode->extra[0] = 0;
    inode->extra[1] = 0;
    inode->extra[2] = 0;

    //Dont have any blocks in i_block;
    inode->i_blocks = 0;
}


char* get_source(char* src_copy, long long* size, int* error){
    // saves the content of a file into a pointer 
	FILE *fp = fopen(src_copy, "r");
    // fail is file doesn't exist or is dir
	if (fp == NULL) {
        *error = 1;
		return 0;
	}
    // is src is a dir 
    struct stat statbuf;
    stat(src_copy, &statbuf);
    if (S_ISDIR(statbuf.st_mode)){
        *error = 2;
        return 0;
    }

    // https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
	fseek(fp, (long)0, SEEK_END);

	*size = (long long)ftell(fp);

    if(size == 0){
        *error = 3;
        return 0;
    }

	rewind(fp);

	char* ptr = calloc(1, *size + 1);
	if (fread(ptr, *size, 1, fp) != 1) {
        *error = 4;
		return 0;
	}
	fclose(fp);
	return ptr;
}

void update_block_bitmap_in_rm(struct ext2_inode* inode_dir){
    for (int i = 0; i < inode_dir->i_blocks / 2; i++){
        int block_num = inode_dir->i_block[i];

        int count = 1;
        int found = 0;
        for (int byte=0; byte<(128/8); byte++){
            if (found == 1){
                break;
            }
            for (int bit=0; bit<8; bit++){
                if (count == block_num){
                    block_bitmap[byte] &= ~(1<<bit);
                    found = 1;
                    break;
                }
                count++;
            }
        }

        sb->s_free_blocks_count++;
        gd->bg_free_blocks_count++;

        if (i == 12){
            int num = inode_dir->i_blocks / 2 - 13;
            unsigned int* new_block_list = (unsigned int *) (disk + 1024 * block_num);
            update_new_block_list(new_block_list, num);
            break;
        }
    }
}

void update_new_block_list(unsigned int* new_block_list, int num){
    for (int i = 0; i < num; i++){
        int block_num = new_block_list[i];
        int count = 1;
        int found = 0;

        for (int byte=0; byte<(128/8); byte++){
            if (found == 1){
                break;
            }
            for (int bit=0; bit<8; bit++){
                if (count == block_num){
                    block_bitmap[byte] &= ~(1<<bit);
                    found = 1;
                    break;
                }
                count++;
            }
        }

        sb->s_free_blocks_count++;
        gd->bg_free_blocks_count++;

    }
}

void rm_block(int block_num){
    int count = 1;
    int found = 0;

    for (int byte=0; byte<(128/8); byte++){
        if (found == 1){
            break;
        }
        for (int bit=0; bit<8; bit++){
            if (count == block_num){
                block_bitmap[byte] &= ~(1<<bit);
                found = 1;
                break;
            }
            count++;
        }
    }

    sb->s_free_blocks_count++;
    gd->bg_free_blocks_count++;

    return;
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
            pthread_mutex_lock(&inode_locks[inode_index]);
            struct ext2_dir_entry* dir_entry = get_dir_entry(inode_entry, current_name, error);
            pthread_mutex_unlock(&inode_locks[inode_index]);
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

            if (strcmp(name, current_name) == 0 && dir_entry->file_type != EXT2_FT_DIR && dir_entry->inode != 0){
                //The file is not a directory file.
                *error = ENOENT;
                return 0;

            } else if (strcmp(name, current_name) == 0 && dir_entry->file_type == EXT2_FT_DIR && dir_entry->inode != 0){
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
    // 1 = dir 2 = file 0 = not found 3 link
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

            if (strcmp(name, current_name) == 0 && dir_entry->file_type == EXT2_FT_REG_FILE){
                return 2;
            } else if (strcmp(name, current_name) == 0 && dir_entry->file_type == EXT2_FT_DIR){
               
                return 1;
            }

            else if(strcmp(name, current_name) == 0 && dir_entry->file_type == EXT2_FT_SYMLINK){

                return 3;
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
    //put the new dir_entry in the beginning of the block
    dir_entry->inode = find_an_unused_inode();
    dir_entry->rec_len = 1024;
    dir_entry->name_len = strlen(dir_name);
    dir_entry->file_type = EXT2_FT_DIR;
    for (int i=0; i<strlen(dir_name); i++){
        dir_entry->name[i] = dir_name[i];
    }
    dir_entry->name[strlen(dir_name)] = '\0';

    //find an unused block and init.
    int unused_block_num = find_an_unused_block();
    //Initialize .
    struct ext2_dir_entry * new_dir_entry = (struct ext2_dir_entry *) (disk + 1024 * unused_block_num);

    init_first_dir(new_dir_entry, dir_entry->inode);

    //Initialize ..
    new_dir_entry = (struct ext2_dir_entry *) (((char*) new_dir_entry)+ new_dir_entry->rec_len);
    init_second_dir(new_dir_entry, parent_inode);

    pthread_mutex_lock(&inode_locks[dir_entry->inode - 1]);
    struct ext2_inode* ext2_inode = &inode_table[dir_entry->inode - 1];
    update_inode_blocks(ext2_inode, unused_block_num);
    pthread_mutex_unlock(&inode_locks[dir_entry->inode - 1]);

    pthread_mutex_lock(&gd_lock);
    gd->bg_used_dirs_count++;
    pthread_mutex_unlock(&gd_lock);
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
    struct ext2_dir_entry * new_dir_entry = (struct ext2_dir_entry *) (disk + 1024 * unused_block_num);

    init_first_dir(new_dir_entry, dir_entry->inode);

    //Initialize ..
    new_dir_entry = (struct ext2_dir_entry *) (((char*) new_dir_entry)+ new_dir_entry->rec_len);
    init_second_dir(new_dir_entry, parent_inode);

    pthread_mutex_lock(&inode_locks[dir_entry->inode - 1]);
    struct ext2_inode* ext2_inode = &inode_table[dir_entry->inode - 1];
    update_inode_blocks(ext2_inode, unused_block_num);
    pthread_mutex_unlock(&inode_locks[dir_entry->inode - 1]);

    pthread_mutex_lock(&gd_lock);
    gd->bg_used_dirs_count++;
    pthread_mutex_unlock(&gd_lock);
}

void update_inode_blocks(struct ext2_inode *inode, int unused_block_num){
    inode->i_mode = EXT2_S_IFDIR;
    inode->i_uid = 0;
    inode->i_size = EXT2_BLOCK_SIZE;
    inode->i_dtime = 0;
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

void make_entry(unsigned int inode, char* dir_name, int* error){
    struct ext2_inode ext2_inode = inode_table[inode];
    // no space 
    pthread_mutex_lock(&sb_lock);
    if (sb->s_free_inodes_count <= 0) {
        *error = ENOSPC;
        return;
    }
    pthread_mutex_unlock(&sb_lock);

    int last_block = (ext2_inode.i_blocks / 2) - 1;
    int block_num = ext2_inode.i_block[last_block];
    struct ext2_dir_entry *dir_entry = (struct ext2_dir_entry *) (disk + 1024 * block_num);
    int used_size = 0;
    //.'s inode
    int itself_inode = dir_entry->inode;
    //..'s inode
    //int parent_inode = ((struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len))->inode;

    struct ext2_dir_entry *new;
    //https://piazza.com/class/ks5i8qv0pqn139?cid=736
    while (used_size < EXT2_BLOCK_SIZE){
        used_size += dir_entry->rec_len;
        if (used_size == EXT2_BLOCK_SIZE){

            //dir_entry is the last one at this time.
            int size = 8 + (int) dir_entry->name_len;
            //make it be multiple of 4
            size += (4 - size % 4);

            //Left size.
            int tmp = dir_entry->rec_len - size;
            if (tmp < 8 + strlen(dir_name)){
                //Initialize . , .. and its directory block.
                //update block_bitmap and inode_bitmap

                //no blocks 
                if(sb->s_free_blocks_count <= 0){
                    *error = ENOSPC;
                    return;
                }
                //find an unused block and add it to inode info.
                int unused_block_num = find_an_unused_block();

                struct ext2_dir_entry* new_dir_entry_in_new_block = (struct ext2_dir_entry *) (disk + 1024 * unused_block_num);

                //No need to initialize . and .. in the new block

                //Initialize added directory.
                init_new_dir_in_new_block(new_dir_entry_in_new_block, dir_name, itself_inode);

                //update inode info.
                pthread_mutex_lock(&inode_locks[itself_inode - 1]);
                struct ext2_inode* dir_inode = &inode_table[itself_inode - 1];
                dir_inode->i_block[dir_inode->i_blocks / 2] = unused_block_num;
                dir_inode->i_blocks += 2;
                pthread_mutex_unlock(&inode_locks[itself_inode - 1]);
                return;

            } else{
                dir_entry->rec_len = size;
                //still have available space.
                //Initialize a directory entry and add it to the last
                new = (struct ext2_dir_entry *) (((char*) dir_entry) + size);
                init_new_dir_in_old_block(new, dir_name, tmp, itself_inode);
                return;
            }
        }
        dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
    }

}
