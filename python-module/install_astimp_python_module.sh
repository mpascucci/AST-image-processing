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

if [[ ! $(pwd) =~ python-module ]]; then
	echo 'Please run from the "python-module" directory.'
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

echo "================ CHECKING INSTALL ================"
echo

# check installation of opencv-python bindings
python -c "import cv2"
if [[ $? != 0 ]]; then
  echo
  echo "ERROR: the python opencv2 module is not installed in this enviromnent.
  Did you install OpenCV and its python bindings?

  Maybe you did, but you are working in a virtualenv. In this case you need
  to link the cv2.cpython-*.so system file in the lib folder of the virtualenv."
fi

echo
# check the installation of this module
python -c "import astimp"
if [[ $? == 0 ]]; then
echo "Python module successfully installed!

Please add \"$PATH_TO_ASTLIB\" to your LD_LIBRARY_PATH

For example type:
  export LD_LIBRARY_PATH=$PATH_TO_ASTLIB:\$LD_LIBRARY_PATH

or add it to yout shell rc script"
else
  echo
  echo "ERROR. Python module not installed."
  echo "Possible causes:
  - check if the opencv lib and include folders are correct \"setup.py\" in this folder
  - be sure you can import opencv"
fi
echo
