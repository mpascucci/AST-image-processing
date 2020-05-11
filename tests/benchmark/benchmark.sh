#!/bin/bash
# Optionally builds, and runs benchmark.py.
# must be run from improc/tests/benchmark directory.

# "usage: sh benchmark.sh [-nb] [args_to_benchmark.py]"
# -nb or --nobuild  : Skip build steps and just run benchmark.py"

# LD_LIBRARY_PATH will be updated only within the scope of this script.
# This is to avoid polluting the user's LD_LIBRARY_PATH with old astimplibs.
# To access astimplib python module outside this script,
# run source install_astimp_python_module.sh separately,
# or run source benchmark.sh.

if [[ ! $(pwd) =~ improc/tests/benchmark ]]; then
	echo "Please run from improc/tests/benchmark directory."
	exit 1
fi

if [[ "$1" == "--nobuild" || "$1" == "-nb" ]]; then # run only
    PATH_TO_ASTLIB="$(pwd)/../../build/astimplib"
    # Update LD_LIBRARY_PATH only within the scope of this script
    if [[ ! $LD_LIBRARY_PATH == $PATH_TO_ASTLIB* ]]
    then 
        # If there are multiple astimplibs, the first will take priority.
        export LD_LIBRARY_PATH=$PATH_TO_ASTLIB:$LD_LIBRARY_PATH
    fi  

    # run benchmark python script
    # "${@:2}" forwards all args except the first, which was '--nobuild'.
    python3 benchmark.py "${@:2}"
else # build and run
    # cd to improc/python-module directory
    cd ../../python-module

    # Install the python module
    #* Use the flag -s here if you get `fatal error: 'cstddef' file not found #include <cstddef>`
    # source is used so that LD_LIBRARY_PATH is updated within the scope
    # of *this* script. "" avoids forwarding args.
    source ./install_astimp_python_module.sh ""

    # get back to improc/tests/benchmark
    cd ../tests/benchmark

    # "${@}" forwards all args to benchmark.py.
    python3 benchmark.py "$@"
fi