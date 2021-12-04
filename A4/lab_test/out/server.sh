#!/bin/bash

cd src/
rm *.o
rm *.so
make
cd ..
cp -f src/libext2fsal.so bin/libext2fsal.so
cp -f clean/emptydisk.img img/emptydisk.img
cd bin/
./ext2kmfs /student/gaoyi12/group_0551/A4/lab_test/out/img/emptydisk.img