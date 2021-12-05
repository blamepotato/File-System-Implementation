#!/bin/bash

cd out/
git pull
cd src/
rm *.o
rm *.so
make
cd ..
cp -f src/libext2fsal.so bin/libext2fsal.so
cp -f clean/onedirectory.img img/onedirectory.img
cd img
PATH=`pwd`
cd ..
cd bin/
./ext2kmfs $PATH/onedirectory.img