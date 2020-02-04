#!/bin/bash

sudo apt install libasound2-dev

mkdir build > nul
pushd ./build
cmake -G "Unix Makefiles" ../
sudo make
popd

