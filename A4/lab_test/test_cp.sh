#!/bin/bash
cd out/
# remove bfile
cd util/

./ext2umfs_cp /student/gaoyi12/group_0551/A4/lab_test/out/util /level1/level2/; echo "return value is:"$?
cd ..
cd img/
./ext2_dump twolevel.img
