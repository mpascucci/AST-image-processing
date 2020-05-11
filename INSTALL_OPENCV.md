# OpenCV INSTALL

Some notes on how to install opencv.

Download [OpenCV source code](https://opencv.org/releases/) (version >= 4.1.0).

NOTE: Yow may want to keep the source files after install in order to be able to uninstall with `make uninstall`.

## Configure

**NOTE** Use the CMake-GUI instead of the command
line, it is much easier to find relevant parameters and spot errors.

Download and install the latest version of CMake from [cmake.org](https://cmake.org/download/).
You shouldn't need to compile from the sources: binary distributions are available for Linux, MacOS and Windows.

## Build

Once finished configuring with the CMake-Gui, open a terminal, go to the
OpenCV-source-folder/build and run

`$ make -j7` to compile

`$ make install` to install in the system as a shared library

## Contrib modules, CVV and Qt5

Contrib modules are not necessary for `improc`.

CVV is a useful visual debug tool. Qt (and qt_dev) must be installed
otherwise the CVV module can not be built.
Unfortunately I could not have it work on OS X.

### installing CVV on OSX

- download the [opencv_contrib modules](https://github.com/opencv/opencv_contrib)

- set the path of the modules with the CMake-GUI to `OPENCV_EXTRA_MODULE_PATH = <opencv_contrib>/modules`

- install qt5 with brew

- search for the Qt5 cmake file

        $ find / -name "Qt5Config.cmake" -print

- set this path as value of `Qt5_DIR` in CMake-GUI

- reconfigure and check that `BUILD_opencv_cvv` is selected