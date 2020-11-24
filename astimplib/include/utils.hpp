// Copyright 2019 Copyright 2019 The ASTapp Consortium
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    <http://www.apache.org/licenses/LICENSE-2.0>
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Marco Pascucci <marpas.paris@gmail.com>.

#ifndef ASTAPP_UTILS_HPP
#define ASTAPP_UTILS_HPP

#include <vector>

#include "astimp.hpp"

vector<vector<float>> distance_matrix_2d(const vector<astimp::Circle> &circles);

bool is_circle(vector<cv::Point> &contour, double maxThinness);
bool is_circle(const cv::Mat &contour, double maxThinness);

bool is_ast_blood_based(cv::Mat &crop);

cv::Rect getBoundingRectFromCircle(cv::Point2f center, float radius);

/*@Brief Return a 2D matrix which values are the radial distances from
 *center rounded as integers The matrix has size (s,s)*/
vector<vector<uint>> r_matrix(int s);

vector<float> first_neighbour_distance(const vector<astimp::Circle> &circles);

vector<int> masked_k_means(cv::Mat img, int k);

// Return the smallest element of a non empty vector.
template <typename T>
T vector_min(const vector<T> &v) {
    if (v.size() == 0) {
        throw astimp::Exception::generic(
            "vector_min() was given an empty vector");
    }
    return *min_element(v.begin(), v.end());
}

template <typename T>
T sumvector(const vector<T> &v, int start, int end) {
    int sum = 0;
    for (int i = start; i != end; i++) {
        sum = sum + v[i];
    }
    return sum;
}

#endif  // ASTAPP_UTILS_HPP
