![ASTimp banner](docs/images/Github_social.png)


[![DOI](https://zenodo.org/badge/299842477.svg)](https://zenodo.org/badge/latestdoi/299842477)

A library for processing and measuring Disk Diffusion Antibiotic Susceptibility Testing.
Its functions can measure the inhibition diameter in a picture of a Petri Dish.
The library can be used in C++ code or as a Python module.

This library was created for [Antibiogo](https://fondation.msf.fr/projets/antibiogo) an AST reading offline mobile application developed by a consortium of The Fundation Médecins Sans Frontières, l'Université d'Évry, le [LAMME](http://www.math-evry.cnrs.fr/doku.php), the Hénri Mondor University Hospital end the Génoscope. The project won the [Google AI impact challenge 2019](https://www.blog.google/outreach-initiatives/google-org/ai-impact-challenge-grantees/). Google has been supporting and helping the development of this project untill October 2020.

# Installation and Quick Start
Please visit the [Quick Start Page](https://mpascucci.github.io/AST-image-processing/)

## Description
This image processing library contains functions to measures the inhibition diameters in the picture of a disk diffusion AST.

The image should be acquired according to [this protocol](https://mpascucci.github.io/ASTapp-protocol/)

## Project tree

```{}
|- astimp/              # astimp sources
   |- include/
      |- astimp_version.hpp.in   # contains astimp version
      |- astException.hpp  # custom astimp exception
      |- astimp.hpp        # main library header
      |- stand_labels.hpp  # headers for the generated label templates
  |- src/
      |- debug.hpp         # debug functions, not necessary build
      |- utils.cpp         # auxiliary functions used by the library
      |- astimp.cpp        # main astimp translation unit
|- docs/                   # [QuickStart](https://mpascucci.github.io/AST-image-processing/) page sources
|- python-module/          # python API code for astimp
      | - install_astimp_python_module.sh # install script fot the python module
|- tests/                  # test code
   |- example/             # full AST processing from picture to measurements
   |- images/              # test images
   |- include/             # tests level includes
      |- test_config.h     # contains a reference to the test image folder
   |- unit_tests/
      |- *.hpp             # astimp functions unit tests
      |- astimp_tests.cpp  # unit tests entry point
|- README.md               # this file
|- INSTALL_OPENCV.md       # install notes for OpenCV
```

## Detailed compiling and troubleshooting

### Build the library

If you want to build OpenCV from source, check `INSTALL_OPENCV.md`.
Run the `run-build.sh` script to build the library.

### Build targets
`make` is equivalent to `make all` and it will build all targets.

- `cd build` cd to build dir

- `make astimp` builds the library

- `make runUnitTests` builds the unit test executable

- `make fullExample` builds the full AST processing example

### Python API

To install `astimp` Python module, run `source ./install_astimp_python_module.sh` from `improc/python-module`.

Do not run `pip install` manually.

Re-run `source ./install_astimp_python_module.sh` whenever you want to bring new C++ changes into the python module.

#### Troubleshooting python
- You must use Python 3 and Numpy 1.7. On some systems, this might mean run `pip3` and `python3` above instead of `pip` or `python`.
- If you see `expected a readable buffer object`, make sure you're using numpy1.7.
- If you get `cannot open shared object file: No such file or directory` make sure that your `LD_LIBRARY_PATH` contains the path to `improc/build/astimplib` and the one to the `opencv` shared libraries in yout system (most likely `/usr/local/lib`). Then restart your terminal.
- If you get `...CMakeLists.txt does not match CMakeLists.txt used to generate cache`, `rm -rf build`.
- If you get `fatal error: 'cstddef' file not found #include <cstddef>` while building the python module's wheel, run the install script with `-s` flag, this will set `CFLAGS='-stdlib=libc++'`
- If anything does not update after install, rerun install with `-c` (clean) flag.
- `Using deprecated NumPy API` can be ignored.

### Run

- Run the unit tests the same way they are run by pipelines: `improc/ $ ./run-tests.sh`

- Run the full example: `improc/ $ ./build/tests/fullExample ./build/tests/test0.jpg`

- Run benchmarking: `improc/tests/benchmark$ sh ./benchmark.sh -d`. See also improc/tests/benchmark/README.md.

## Disclaimer

The Software and code samples available on this repository are provided "as is" without warranty of any kind, either express or implied.
Use at your own risk.

The use of the software and scripts downloaded on this site is done at your own discretion and risk and with agreement that you will be solely responsible for its use and possible damages to you or others.
You are solely responsible for adequate protection and backup of the data and equipment used in connection with any of the software, and we will not be liable for any damages that you may suffer in connection with using, modifying or distributing any of this software. No advice or information, whether oral or written, obtained by you from us or from this website shall create any warranty for the software.

We make makes no warranty that:
- he software will meet your requirements
- the software will be uninterrupted, timely, secure or error-free
- the results that may be obtained from the use of the software will be effective, accurate or reliable
- the quality of the software will meet your expectations
- any errors in the software obtained from us will be corrected. 

The software, code sample and their documentation made available on this website:
- could include technical or other mistakes, inaccuracies or typographical errors. We may make changes to the software or documentation made available on its web site at any time without prior-notice.
- may be out of date, and we make no commitment to update such materials. 

We assume no responsibility for errors or omissions in the software or documentation available from its web site.

In no event shall we be liable to you or any third parties for any special, punitive, incidental, indirect or consequential damages of any kind, or any damages whatsoever, including, without limitation, those resulting from loss of use, data or profits, and on any theory of liability, arising out of or in connection with the use of this software. 

## Copyright and Licence

Copyright 2019 The ASTapp Consortium

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   <http://www.apache.org/licenses/LICENSE-2.0>

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Author: Marco Pascucci <marpas.paris@gmail.com>.
Principal contributors:
- Guillaume Boniface-Chang (antibiotic disks label)
- Ellen Sebastian (benchmark)
- Jakub Adamek (C++ Tests)
