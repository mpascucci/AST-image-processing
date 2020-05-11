// Copyright 2019 Fondation Medecins Sans Frontières https://fondation.msf.fr/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the ASTapp image processing library
// Author: Marco Pascucci <marpas.paris@gmail.com>

#include <fstream>
#include <iostream>
#include <opencv2/highgui.hpp>

// Logging functions
template <class T>
void log(T message) {
    std::cout << ">>> ASTimpTest : " << message << std::endl;
}

template <class T, class U>
void log(T message, U value) {
    std::cout << ">>> ASTimpTest : " << message << ' ' << value << std::endl;
}

template <typename T>
void print_matrix(const vector<vector<T>> &m) {
    // print a matrix
    // it is assumed that the rows have all the same length
    size_t rows = m.size();
    size_t cols = m[0].size();
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            cout << m[i][j] << "\t";
        }
        cout << endl;
    }
}

template <typename T>
void print_vector(const vector<T> &v) {
    // print a matrix
    // it is assumed that the rows have all the same length
    size_t n = v.size();
    for (size_t i = 0; i < n; i++) {
        cout << v[i] << ", ";
    }
    cout << endl;
}

template <typename T>
void vector_to_csv(string filename, vector<T> v) {
    ofstream ofs(filename);
    char s[128];
    for (size_t i = 0; i < v.size(); i++) {
        sprintf(s, "%e.3\n", v[i]);
        ofs << s;
    }
    ofs.close();
}
