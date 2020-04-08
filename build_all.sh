#!/bin/bash

if [ "$1" != "" ] ; then
source config_all.sh
else
source config_all.sh $1
fi

cd build
make
sudo make install
cd ..
