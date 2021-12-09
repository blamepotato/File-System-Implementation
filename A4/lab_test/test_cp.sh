#!/bin/bash
cd out/
# remove bfile
cd util/

./ext2umfs_cp gg.txt /level1/level2/; echo "return value is:"$?
cd ..
cd img/
./ext2_dump twolevel.img
