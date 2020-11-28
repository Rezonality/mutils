#!/bin/bash

if [ "$1" != "" ] ; then
source config_all.sh
set CONFIG=Release
else
source config_all.sh $1
set CONFIG=$1
fi

cd build
cmake --build . --config $CONFIG
cmake --install . --config $CONFIG --prefix ../../vcpkg/packages/mutils_x64-linux
cd ..

