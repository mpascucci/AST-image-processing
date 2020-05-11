#! /usr/bin/python3

# Copyright 2019 Fondation Medecins Sans Frontieres https://fondation.msf.fr/
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

import os, sys
from trainer.pellet_list import PELLET_LIST

TFLITE_MODEL = sys.argv[1]

FILE_HEADER = """
#include "pellet_label_tflite_model.hpp"
"""

def write_bytes(name, contents):
    # Needs to be aligned in order not to crash on Samsung A10 and other phones.
    # 4-byte alignment is required (for execution), and 16-byte is recommended (for performance).
    print("__attribute__((__aligned__(16))) const unsigned char %s_UNSIGNED[] = {\n  " % name, end='')

    linecount = 0

    for ch in contents:
        # Format as 0xAB,
        print('%-06s ' % ('0x%0X,' % ch), end='')
        linecount += 1
        if linecount == 11:
            print("\n  ", end='')
            linecount = 0

    # Write footer
    print('\n};')
    print("const char *%s = (const char *) %s_UNSIGNED;" % (name, name))
    print("size_t %s_SIZE = sizeof(%s_UNSIGNED);" % (name, name))


def print_strings(strings):
    length = 100
    for s in strings:
        if length + len(s) > 80:
            print()
            print("  ", end='')
            length = 2
        formatted = '"%s", ' % s
        print(formatted, end='')
        length += len(formatted)


if __name__ == '__main__':
    print(FILE_HEADER)
    print("const vector<string> PELLET_LABELS = {", end='')
    print_strings([pellet.replace(" ", "") for pellet in PELLET_LIST])
    print("};")
    print()
    with open(TFLITE_MODEL, 'rb') as in_file:
        file_content = in_file.read()
        write_bytes("PELLET_LABEL_TFLITE_MODEL", file_content)
