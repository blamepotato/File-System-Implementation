#!/bin/bash
cd out/
cd util/
./ext2umfs_mkdir /level1/level2/level3
cd ..
cd img/
./ext2_dump twolevel.img