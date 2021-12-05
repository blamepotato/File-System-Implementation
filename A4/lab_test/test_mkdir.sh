#!/bin/bash
cd out/
cd util/
./ext2umfs_mkdir /level1/level2/
cd ..
cd img/
./ext2_dump onedirectory.img