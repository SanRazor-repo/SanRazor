#!/bin/bash

cd llvm
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="X86" ..
make -j $(nproc)
sudo make install