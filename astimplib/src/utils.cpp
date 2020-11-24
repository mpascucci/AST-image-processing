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

#ifndef ASTIMP_UTILS
#define ASTIMP_UTILS

#include "utils.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;

bool is_circle(vector<cv::Point> &contour, double maxThinness) {
    vector<cv::Point> hull;
    cv::Mat contourMat;
    cv::convexHull(cv::Mat(contour), hull, false);
    cv::approxPolyDP(cv::Mat(hull), contourMat, 1, 1);
    contourMat.reshape(-1, 2);
    return is_circle(contourMat, maxThinness);
}

bool is_circle(const cv::Mat &contour, double maxThinness) {
    /* check if the contour has a circular shape
    (based on its thinnes) */
    double A = contourArea(contour);
    double P = arcLength(contour, true);
    double thisThinness = pow(P, 2) / A / 4 / 3.1415926535897932384626433;
    return thisThinness < maxThinness;
}

cv::Rect getBoundingRectFromCircle(cv::Point2f center, float radius) {
    // calc the bounding boc (rectangle) given the center and radius of a circle
    return cv::Rect(center.x - radius, center.y - radius, radius * 2,
                    radius * 2);
}

vector<vector<uint>> r_matrix(int s) {
    vector<vector<uint>> R(s, vector<uint>(s));
    for (int i = 0; i < s; i++) {
        for (int j = 0; j < s; j++) {
            R[i][j] = (uint)round(sqrt(pow(i - s / 2, 2) + pow(j - s / 2, 2)));
        }
    }
    return R;

    // THIS CODE TO TRANSFORM the matrix IN cv::MAT
    // cv::Mat temp(n,n,CV_8U);
    // // cout << "channels " << temp.channels() << endl;
    // for (uint i=0; i<n; i++) {
    //   for (uint j=0; j<n; j++) {
    //     printf("%x, %x\n", i,j);
    //     temp.at<uchar>(i,j) = R[i][j];
    //   }
    // }
    // cv::imshow("display", temp);
    // cv::waitKey(0);
    // cout << temp << endl;
}

vector<vector<float>> distance_matrix_2d(
    const vector<astimp::Circle> &circles) {
    /* return the distance matrix of the Circles centers */

    size_t n = circles.size();

    vector<vector<float>> distance(n, vector<float>(n, 0));

    float x1, y1, x2, y2;

    for (size_t i = 0; i < n; i++) {
        x1 = circles[i].center.x;
        y1 = circles[i].center.y;

        for (size_t j = i + 1; j < n; j++) {
            x2 = circles[j].center.x;
            y2 = circles[j].center.y;
            // distance[i][j] = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2)); //
            // EUCLIDEAN
            distance[i][j] = max(abs(x2 - x1), abs(y2 - y1));
            distance[j][i] = distance[i][j];  // the matrix is symmetric
        }
    }

    return distance;
}

vector<float> first_neighbour_distance(const vector<astimp::Circle> &circles) {
    /* @brief calculate the first neigbour distances for each Circle in input
     *
     * Distance = min(x,y)
     *
     */
    vector<float> out(circles.size(), 0);
    vector<vector<float>> distance = distance_matrix_2d(circles);

    float min_value;
    for (size_t i = 0; i < out.size(); i++) {
        // the first element of the first row is zero because it is the distance
        // of the first circle with itself
        if (i == 0) {
            min_value = distance[i][1];
        } else {
            min_value = distance[i][0];
        }

        for (size_t j = 0; j < distance[i].size(); j++) {
            if (j == i)
                continue;  // skip auto-distance, which is obviously zero
            if (distance[i][j] < min_value) {
                min_value = distance[i][j];
            }
        }
        out[i] = min_value;
    }

    return out;
}

bool is_ast_blood_based(cv::Mat &crop) {
    /* given the image of a (cropped) petri dish
     * classify the growth medium based on dominant color
     * and return true if the ast is probably blood based */

    // TODO(marco) implement this function based on experimental python code
    return false;
}

vector<int> masked_k_means(cv::Mat img, int k) {
    // return the centers of a k-means classification of the
    // intensity values in img (a CV_32F image).
    // pixels with value -1 are ignored
    cv::Mat temp = img.reshape(0, 1);
    // temp.convertTo(temp, CV_32F);
    uint non_negative_n = 0;
    // count non negative values
    for (int j = 0; j < temp.rows * temp.cols; ++j) {
        if (temp.at<float>(j) >= 0) {
            non_negative_n++;
        }
    }
    // copy non negative values to selection
    cv::Mat selection = cv::Mat(1, non_negative_n, CV_32F);
    size_t this_idx = 0;
    float this_value{};
    for (int j = 0; j < temp.rows * temp.cols; ++j) {
        this_value = temp.at<float>(j);
        if (this_value >= 0) {
            selection.at<float>(this_idx) = round(this_value * UCHAR_MAX);
            this_idx++;
        }
    }
    // apply k-means
    int kmeans_attempts = 10;
    cv::Mat labels;
    vector<int> km_centers;
    cv::kmeans(
        selection, k, labels,
        cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER,
                         100, 0.01),
        kmeans_attempts, cv::KMEANS_PP_CENTERS, km_centers);

    sort(km_centers.begin(), km_centers.end());

    return km_centers;
}

#endif  // ASTIMP_UTILS