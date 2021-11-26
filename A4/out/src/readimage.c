#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;


int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);

    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + 1024*2);
    printf("Block group:\n");
    printf("block bitmap: %d\n", gd->bg_block_bitmap);
    printf("inode bitmap: %d\n", gd->bg_inode_bitmap);
    printf("inode table: %d\n", gd->bg_inode_table);
    printf("free blocks: %d\n", sb->s_free_blocks_count);
    printf("free inodes: %d\n", sb->s_free_inodes_count);
    printf("used_dirs: %d\n", gd->bg_used_dirs_count);
    
    printf("Block bitmap: ");
    int count = 0;
    unsigned char *block_bitmap = (unsigned char *) (disk + 1024 * gd->bg_block_bitmap);
    for (int byte=0; byte<(128/8); byte++){
        for (int bit=0; bit<8; bit++){
            if ((block_bitmap[byte]&(1<<bit)) != 0){
                printf("1");
            } else{
                printf("0");
            }
            if (count == 7){
                printf(" ");
                count = 0;
            } else{
                count++;
            }
        }
    }
    printf("\n");

    printf("Inode bitmap: ");
    count = 0;
    unsigned char *inode_bitmap = (unsigned char *) (disk + 1024 * gd->bg_inode_bitmap);
    for (int byte=0; byte<(32/8); byte++){
        for (int bit=0; bit<8; bit++){
            if ((inode_bitmap[byte]&(1<<bit))!=0){
                printf("1");
            } else{
                printf("0");
            }
            if (count == 7){
                printf(" ");
                count = 0;
            } else{
                count++;
            }
        }
    }
    printf("\n");

    printf("Inodes:\n");
    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + 1024 * gd->bg_inode_table);

    //printf("%d", sb->s_inodes_count);
    struct ext2_inode ext2_root = inode_table[1];
    if(ext2_root.i_mode & EXT2_S_IFDIR){
        printf("[%d] type: d size: %d links: %d blocks: %d \n", EXT2_ROOT_INO, ext2_root.i_size, ext2_root.i_links_count, ext2_root.i_blocks);
        printf("[%d] Blocks: ", 2);
        for(int i = 0; i < ext2_root.i_blocks / 2; i++){
            printf("%d ", ext2_root.i_block[i]);
        }
        printf("\n");
    } else if (ext2_root.i_mode & EXT2_S_IFREG){
        printf("[%d] type: f size: %d links: %d blocks: %d \n", EXT2_ROOT_INO, ext2_root.i_size, ext2_root.i_links_count, ext2_root.i_blocks);
        printf("[%d] Blocks: ", 12);
        for(int i = 0; i < ext2_root.i_blocks / 2; i++){
            printf("%d ", ext2_root.i_block[i]);
        }
        printf("\n");
    }

    count = 0;
    for (int byte=0; byte<(32/8); byte++){
        for (int bit=0; bit<8; bit++){
            if ((count >= EXT2_GOOD_OLD_FIRST_INO) && ((inode_bitmap[byte]&(1<<bit))!=0)){
                struct ext2_inode inode = inode_table[count];
                if(inode.i_mode & EXT2_S_IFDIR){
                    printf("[%d] type: d size: %d links: %d blocks: %d \n", count+1, inode.i_size, inode.i_links_count, inode.i_blocks);
                    printf("[%d] Blocks: ", count+1);
                    for(int i = 0; i < inode.i_blocks / 2; i++){
                        printf("%d ", inode.i_block[i]);
                    }
                    printf("\n");
                } else if (inode.i_mode & EXT2_S_IFREG){
                    printf("[%d] type: f size: %d links: %d blocks: %d \n", count+1, inode.i_size, inode.i_links_count, inode.i_blocks);
                    printf("[%d] Blocks: ", count+1);
                    for(int i = 0; i < inode.i_blocks / 2; i++){
                        printf("%d ", inode.i_block[i]);
                    }
                    printf("\n");
                }
            }
            count++;
        }
    }

    printf("\n");
    //TE10
    printf("Directory Blocks:\n");

    int block_num;
    //printf("%d", ext2_root.i_blocks);
    for(int i = 0; i < ext2_root.i_blocks / 2; i++){
        block_num = ext2_root.i_block[i];
    }

    printf("DIR BLOCK NUM: %d (for inode %d)\n", block_num, EXT2_ROOT_INO);

    if(ext2_root.i_mode & EXT2_S_IFDIR){
        struct ext2_dir_entry *dir_entry = (struct ext2_dir_entry *) (disk + 1024 * block_num);
        int used_size = 0;
        while (used_size < EXT2_BLOCK_SIZE){
            char name[dir_entry->name_len + 1];
            name[dir_entry->name_len] = '\0';
            for(int i = 0;i < dir_entry->name_len;i++){
                name[i] = dir_entry->name[i];
            }
            char dtype = (dir_entry->file_type == 2)?'d':'f';
            printf("Inode: %d rec_len: %d name_len: %d type= %c name=%s\n", dir_entry->inode, dir_entry->rec_len, dir_entry->name_len, dtype ,name);
            //printf("Inode: %d rec_len: %d name_len: %d type= f name=%s\n", dir_entry->inode, dir_entry->rec_len, dir_entry->name_len, dir_entry->name);
            used_size += dir_entry->rec_len;
            dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
        }
    }

    count = 0;

    for (int byte=0; byte<(32/8); byte++){
        for (int bit=0; bit<8; bit++){
            //printf("%d\n", (inode_bitmap[byte]&(1<<bit)));
            if ((count >= EXT2_GOOD_OLD_FIRST_INO) && ((inode_bitmap[byte]&(1<<bit))!=0)){
                struct ext2_inode inode = inode_table[count];
                for(int i = 0; i < inode.i_blocks / 2; i++){
                    block_num = inode.i_block[i];
                }
                if(inode.i_mode & EXT2_S_IFDIR){
                    printf("DIR BLOCK NUM: %d (for inode %d)\n", block_num, count+1);
                    int used_size = 0;
                    struct ext2_dir_entry *dir_entry = (struct ext2_dir_entry *) (disk + 1024 * block_num);
                    while (used_size < EXT2_BLOCK_SIZE){
                        char name[dir_entry->name_len + 1];
                        name[dir_entry->name_len] = '\0';
                        for(int i = 0;i < dir_entry->name_len;i++){
                            name[i] = dir_entry->name[i];
                        }

                        char dtype = (dir_entry->file_type == 2)?'d':'f';
                        printf("Inode: %d rec_len: %d name_len: %d type= %c name=%s\n", dir_entry->inode, dir_entry->rec_len, dir_entry->name_len, dtype ,name);
                        used_size += dir_entry->rec_len;
                        dir_entry = (struct ext2_dir_entry *) (((char*) dir_entry)+ dir_entry->rec_len);
                    }
                }
            }
            count++;
        }
    }

    return 0;
}