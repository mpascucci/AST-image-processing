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

#include "pellet_label_recognition.hpp"

#include <numeric>

#include "astimp.hpp"

const size_t IMG_SIZE = 64;

namespace astimp {
Label_match getOnePelletText(const cv::Mat &pelletImg) {
    return PelletLabelRecognitionUsingML::getOnePelletText(pelletImg);
}

vector<Label_match> getPelletsText(const vector<cv::Mat> &pelletImages) {
    return PelletLabelRecognitionUsingML::getPelletsText(pelletImages);
}

// Crops and resizes into an IMG_SIZE square.
cv::Mat formatPelletForInference(const cv::Mat &pellet) {
    cv::Mat result = pellet;
    if (pellet.rows != pellet.cols) {
        int smallerDimension = min(pellet.rows, pellet.cols);
        cv::Rect roi(0, 0, smallerDimension, smallerDimension);
        result = pellet(roi);
    }
    if (result.rows == IMG_SIZE) {
        return result;
    }
    cv::Mat resized;
    // TODO(jakuba): Consider different sampling strategies if shrinking or
    // growing the image – they can have significant effects on the sharpness of
    // the resulting text.
    cv::resize(result, resized, {IMG_SIZE, IMG_SIZE});
    return resized;
}

// Returns a grey 1-channel matrix.
cv::Mat mapToCommonSpace(const cv::Mat &pellet) {
    if (pellet.channels() == 1) {
        return pellet;
    }

    cv::Mat gray;
    cv::cvtColor(pellet, gray, cv::COLOR_BGR2GRAY);
    return gray;
}

// Normalizes the data: subtracts mean and divides by stdev.
void normalize(float *begin, float *end) {
    // Based on ImageDataGenerator.
    // samplewise_center=True means x -= np.mean(x)
    // samplewise_std_normalization=True means x /= (np.std(x) + 1e-7)
    size_t size = end - begin;
    auto mean = float(accumulate(begin, end, 0.0) / size);

    float accum = 0.0;
    for_each(begin, end, [&](float d) { accum += (d - mean) * (d - mean); });

    float stdev = sqrt(accum / float(size));
    for (float *i = begin; i != end; ++i) {
        *i = (*i - mean) / (stdev + 1e-6);
    }
}

void copyMatToFloatArray(const cv::Mat &mat, float *output) {
    if (mat.isContinuous()) {
        const uchar *data = mat.ptr(0);
        for (size_t i = 0; i < mat.total(); ++i) {
            *output++ = float(*data++);
        }
    } else {
        for (int row = 0; row < mat.rows; ++row) {
            const uchar *data = mat.ptr(row);
            for (int col = 0; col < mat.cols; ++col) {
                *output++ = float(*data++);
            }
        }
    }
}

// TODO(jakuba): Run recognition on all inputs at once, not one by one.
vector<vector<float>> PelletLabelRecognitionUsingML::getPelletModelInputs(
    const vector<cv::Mat> &pelletImages) {
    vector<vector<float>> inputsList;
    inputsList.reserve(pelletImages.size());
    for (const cv::Mat &pelletImage : pelletImages) {
        cv::Mat pellet = formatPelletForInference(pelletImage);
        pellet = mapToCommonSpace(pellet);
        vector<float> inputs(pellet.rows * pellet.cols);
        copyMatToFloatArray(pellet, &inputs.front());
        normalize(&inputs.front(), &inputs.back());
        inputsList.emplace_back(inputs);
    }
    return inputsList;
}
}  // namespace astimp