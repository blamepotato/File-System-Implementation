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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>

int fd;
unsigned char *disk;
struct ext2_super_block* sb;
struct ext2_group_desc* gd;
struct ext2_inode* inode_table;
unsigned char* block_bitmap;
unsigned char* inode_bitmap;

pthread_mutex_t sb_lock;
pthread_mutex_t gd_lock;
pthread_mutex_t inode_locks[32];
pthread_mutex_t block_bitmap_lock;
pthread_mutex_t inode_bitmap_lock;

void ext2_fsal_init(const char* image)
{
    /**
     * TODO: Initialization tasks, e.g., initialize synchronization primitives used,
     * or any other structures that may need to be initialized in your implementation,
     * open the disk image by mmap-ing it, etc.
     */
    
    fd = open(image, O_RDWR);
    if (fd < 0){	
		return;
	}

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        return;
    }

    sb = (struct ext2_super_block *)(disk + 1024);
    gd = (struct ext2_group_desc *)(disk + 1024 * 2);
    inode_table = (struct ext2_inode *) (disk + 1024 * gd->bg_inode_table);
    block_bitmap = (unsigned char *) (disk + 1024 * gd->bg_block_bitmap);
    inode_bitmap = (unsigned char *) (disk + 1024 * gd->bg_inode_bitmap);

    pthread_mutex_init(&sb_lock, NULL);
    pthread_mutex_init(&gd_lock, NULL);
    pthread_mutex_init(&block_bitmap_lock, NULL);
    pthread_mutex_init(&inode_bitmap_lock, NULL);
    for(int i = 0; i < 32; i++){
        pthread_mutex_init(&inode_locks[i], NULL);
    }
    
}

void ext2_fsal_destroy()
{
    pthread_mutex_destroy(&sb_lock);
    pthread_mutex_destroy(&gd_lock);
    pthread_mutex_destroy(&block_bitmap_lock);
    pthread_mutex_destroy(&inode_bitmap_lock);
    for(int i = 0; i < 32; i++){
        pthread_mutex_destroy(&inode_locks[i]);
    }

    if (munmap(disk, 4096) == -1) {
        return;
    }
    close(fd);
}
