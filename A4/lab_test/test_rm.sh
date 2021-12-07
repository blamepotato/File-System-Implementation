#!/bin/bash
cd out/
# remove bfile
cd util/
./ext2umfs_rm /level1/level2/bfile; echo "return value is:"$?
cd ..
cd img/
./ext2_dump twolevel.img
