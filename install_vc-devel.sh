#!/bin/bash

git clone https://github.com/VcDevel/Vc.git
cd Vc || { echo "Failed to enter Vc directory"; exit 1; }

mkdir build
cd build || { echo "Failed to enter build directory"; exit 1; }

cmake -DCMAKE_INSTALL_PREFIX=~/Vc ..

make
make install

export LD_LIBRARY_PATH=~/Vc/lib:$LD_LIBRARY_PATH
export PATH=~/Vc/bin:$PATH
export CPLUS_INCLUDE_PATH=~/Vc/include:$CPLUS_INCLUDE_PATH

# clang++ Matrix_solution.cpp -o matrix.out -I~/Vc/include -L~/Vc/lib -O3 -fno-tree-vectorize -msse ~/Vc/lib/libVc.a
