#!/bin/bash
# installs astimp python module.

# Usage: source install_astimp_python_module.sh
# Use source instead of sh to ensure that your LD_LIBRARY_PATH is updated.
usage() {
	echo "Usage:
	$ source install_astmp_python_module.sh [-cs]

	options:

	-c clean build files (to rebuild)

	-s set CFLAGS='-stdlib=libc++'
	set this flag if you get `fatal error: 'cstddef' file not found #include <cstddef>`
	"
}

if [[ ! $(pwd) =~ improc/python-module ]]; then
	echo "Please run from improc/python-module directory."
	exit 1
fi

CFLAGS=''

# remove the generated files to allow to regenerate them
while getopts "cs" option; do
case ${option} in
c ) #For option c
sh clean.sh
;;
s )
CFLAGS='-stdlib=libc++'
;;
* ) #For invalid option
usage
exit 1
;;
esac
done

echo "================ PY DEPENDENCIES ================"
pip3 install -r requirements.txt -v
echo "done."
echo


# cd to improc directory
cd ..

# build astimp library
echo "================ BUILD ASTIMP ================"
PATH_TO_ASTLIB="$(pwd)/build/astimplib"
echo "PATH_TO_ASTLIB ${PATH_TO_ASTLIB}"
if ! test -d build; then
	mkdir build
fi
cd build
cmake ..;
make astimp
echo

# cd to python-module directory
cd ../python-module

# build wheel
echo "================ BUILD WHEEL ================"

CFLAGS="$CFLAGS" python3 setup.py bdist_wheel

echo

# install astimp python module
echo "================ INSTAL MODULE ================"
pip3 install --upgrade --force-reinstall dist/*.whl
echo

echo "================ LD LIBRARY PATH ================"

if [[ ! $LD_LIBRARY_PATH == $PATH_TO_ASTLIB* ]]
then 
	export LD_LIBRARY_PATH=$PATH_TO_ASTLIB:$LD_LIBRARY_PATH
	echo "astlimp lib path added to your LD_LIBRARY_PATH"
else
	echo "check LD_LIBRARY_PATH for astimplib --> already installed."
fi
