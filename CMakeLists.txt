# // Copyright 2019 Fondation Medecins Sans Frontières https://fondation.msf.fr/
# //
# // Licensed under the Apache License, Version 2.0 (the "License");
# // you may not use this file except in compliance with the License.
# // You may obtain a copy of the License at
# //
# //    http://www.apache.org/licenses/LICENSE-2.0
# //
# // Unless required by applicable law or agreed to in writing, software
# // distributed under the License is distributed on an "AS IS" BASIS,
# // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# // See the License for the specific language governing permissions and
# // limitations under the License.
# //
# // This file is part of the ASTapp image processing library
# // Author: Marco Pascucci <marpas.paris@gmail.com>

cmake_minimum_required(VERSION 3.4.1)

#/* ---------------------------- COMPILER OPTIONS ---------------------------- */
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-std=c++11 -O2 -pthread -Wall -Werror")

#/* --------------------------- PROJECT DEFINITION --------------------------- */
project(astimp)
option (WITH_IMPROC_TESTS "build tests" ON) # build tests

set(CVV_DEBUG_MODE OFF)  # enable/disable openCV visual debug

#/* ----------------------------- OPENCV LIBRARY ----------------------------- */
if(NOT OpenCV_LIBS)
        message("OPENCV not loaded so far... Looking for local version!")
        find_package(OpenCV 4.0.0 REQUIRED)
        if(NOT OpenCV_LIBS)
                message(FATAL_ERROR "OpenCV not found")
        endif()
        message("OPENCV cmake path: ${OpenCV_DIR}\n")

        set(CMAKE_BUILD_TYPE Debug)

        message("CVV DEBUG is ${CVV_DEBUG_MODE}")
        if(CVV_DEBUG_MODE)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCVVISUAL_DEBUGMODE")
                add_compile_definitions(USE_CVV)
                message("Enabled CVV includes.")
        endif(CVV_DEBUG_MODE)

        set(CUSTOM_INSTALL_DIR "~/local")
endif()

#/* ------------------------- COMPILE SUBDIRECTORIES ------------------------- */
include_directories(astimplib/include)
include_directories(astimplib/vendor)
add_subdirectory(astimplib)

if (WITH_IMPROC_TESTS)
        enable_testing()
        add_subdirectory(tests)
endif()