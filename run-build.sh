#!/bin/bash

# Builds the improc project into the 'build' subdirectory.

set -e

cd "$(dirname "$0")"
[[ -d build ]] || mkdir build
cd build
cmake ..
make
cd ..
