#!/usr/bin/env python
# coding: utf-8
#
# Copyright 2019 Fondation Medecins Sans Frontières https://fondation.msf.fr/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This file is part of the ASTapp image processing library
# Author: Marco Pascucci

import os

cwd = os.getcwd()
import re
import sys
import platform
import subprocess


from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion
from Cython.Build import cythonize
import glob
import numpy
import setuptools.command.build_py


# ---------------------------------------------------------------------------- #
#                                CYTHON MODULES                                #
# ---------------------------------------------------------------------------- #


# ------------------------------ INCLUDE FOLDERS ----------------------------- #

# Specify here the lib and include folder of OpenCV installed on your System
# ON UBUNTU
cv_include_folder = "/usr/include/opencv4"
cv_lib_folder = "/usr/lib/x86_64-linux-gnu"

# ON MAC
# cv_include_folder = "/usr/local/include/opencv4"
# cv_lib_folder = "/usr/local/lib"

astimp_include_folder = os.path.join(cwd, "../astimplib/include")
astimp_lib_folder = os.path.join(cwd, "../build/astimplib")


# --------------------- check that include folders exits --------------------- #
for path in [cv_lib_folder, cv_include_folder, astimp_lib_folder, astimp_include_folder]:
    if not os.path.exists(path):
        raise FileNotFoundError(path, "not found")


# -------------------- Find opencv libraries in lib_folder ------------------- #
cvlibs = list()
for file in glob.glob(os.path.join(cv_lib_folder, 'libopencv_*')):
    cvlibs.append(file.split('.')[0])
cvlibs = list(set(cvlibs))
# cvlibs = ['-L{}'.format(cv_lib_folder)] + \
cvlibs = ['opencv_{}'.format(
    lib.split(os.path.sep)[-1].split('libopencv_')[-1]) for lib in cvlibs]


# --------------------------- Extensions definition -------------------------- #
ext_opencv_mat = Extension("opencv_mat",
                           sources=["opencv_mat.pyx",
                                    "opencv_mat.pxd"],
                           language="c++",
                           extra_compile_args=["-std=c++11"],
                           extra_link_args=[],
                           include_dirs=[numpy.get_include(),
                                         cv_include_folder,
                                         os.path.join(
                                             cv_include_folder, "opencv2")
                                         ],
                           library_dirs=[cv_lib_folder],
                           libraries=cvlibs,
                           )

ext_astimp = Extension("astimp",
                       sources=["astimp.pyx"],
                       language="c++",
                       extra_compile_args=["-std=c++11"],
                       extra_link_args=[],
                       include_dirs=[numpy.get_include(),
                                     cv_include_folder,
                                     astimp_include_folder,
                                     os.path.join(cv_include_folder, "opencv2")
                                     ],
                       library_dirs=[cv_lib_folder, astimp_lib_folder],
                       libraries=cvlibs + ["astimp"],
                       )


# ---------------------------------------------------------------------------- #
#                                     SETUP                                    #
# ---------------------------------------------------------------------------- #

setup(
    name='astimp',
    version='0.0.3',
    author='Marco Pascucci',
    author_email='marpas.paris@gmail.com',
    description='image processing fot antibiotic susceptibility testing',
    long_description='',
    packages= ['astimp_tools'],
    ext_modules=cythonize([ext_opencv_mat, ext_astimp]),
    # cmdclass=dict(build_astimplib=CMakeBuild),
    zip_safe=False,
)
