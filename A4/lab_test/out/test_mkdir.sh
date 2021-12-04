#!/bin/bash
cd util/
./ext2umfs_mkdir /level1
cd ..
cd img/
./ext2_dump emptydisk.img