#!/bin/bash
set -e

mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_BUILD_TYPE=Release
cmake --build .
cmake --build . --target install