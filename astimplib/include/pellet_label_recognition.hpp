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

#ifndef ASTAPP_PELLET_LABEL_RECOGNITION_HPP
#define ASTAPP_PELLET_LABEL_RECOGNITION_HPP

#include <opencv2/imgproc.hpp>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace astimp {

// A template image of a pellet label text.
struct Label_template {
    string text;
    cv::Mat image;
};

struct LabelAndConfidence {
    string label;
    float confidence;

    bool operator==(const LabelAndConfidence &y) const {
        return label == y.label && confidence == y.confidence;
    }
};

// Comparator for Label match: labels_and_confidence set
struct ConfidenceComparator {
    bool operator()(const LabelAndConfidence &lhs,
                    const LabelAndConfidence &rhs) const {
        return lhs.confidence > rhs.confidence;
    }
};

// Match of a label: texts and match confidence value.
// Label match sorts the labels in descending order of confidence
class Label_match {
   public:
    set<LabelAndConfidence, ConfidenceComparator> labelsAndConfidence;

    Label_match(const set<LabelAndConfidence, ConfidenceComparator>
                    &labelsAndConfidence)
        : labelsAndConfidence(labelsAndConfidence){};
    Label_match() = default;

    bool operator==(const Label_match &y) const {
        return labelsAndConfidence == y.labelsAndConfidence;
    }
};

// Returns the text on a pellet image.
Label_match getOnePelletText(const cv::Mat &pelletImg);

// Returns the texts on the pellet images.
vector<Label_match> getPelletsText(const vector<cv::Mat> &pelletImages);

class PelletLabelRecognitionUsingML {
   public:
    // Returns the TFLite model inputs, one vector per each pellet image.
    static vector<vector<float> > getPelletModelInputs(
        const vector<cv::Mat> &pelletImages);

    static Label_match getOnePelletText(const cv::Mat &pelletImg);

    static vector<Label_match> getPelletsText(
        const vector<cv::Mat> &pelletImages);
};

}  // namespace astimp

#endif  // ASTAPP_PELLET_LABEL_RECOGNITION_HPP
