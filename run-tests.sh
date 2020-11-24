#!/bin/bash

# Runs tests in the improc project.
set -e

cd "$(dirname "$0")"
cd build
CTEST_OUTPUT_ON_FAILURE=1 make test
cd ..
