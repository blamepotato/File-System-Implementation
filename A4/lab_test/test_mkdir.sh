#!/bin/bash
cd out/
# 17 level 2 exists
cd util/
./ext2umfs_mkdir /level1/level2/; echo "here:"$?
cd ..
cd img/
./ext2_dump twolevel.img
# 2 not abs path 
cd ..
cd util/
./ext2umfs_mkdir level1/level2/level3; echo "here:"$?
cd ..
cd img/
./ext2_dump twolevel.img
# 2 is a file
cd ..
cd util/
./ext2umfs_mkdir /level1/level2/bfile; echo "here:"$?
cd ..
cd img/
./ext2_dump twolevel.img
echo $?
# 2 path doesn't exist
cd ..
cd util/
./ext2umfs_mkdir /level1/level100/level5; echo "here:"$?
cd ..
cd img/
./ext2_dump twolevel.img
# no error 
cd ..
cd util/
./ext2umfs_mkdir //level1//level2////level3/////; echo "here:"$?
cd ..
cd img/
./ext2_dump twolevel.img
# 17 exists 
cd ..
cd util/
./ext2umfs_mkdir //level1//level2////level3/////; echo "here:"$?
cd ..
cd img/
./ext2_dump twolevel.img
