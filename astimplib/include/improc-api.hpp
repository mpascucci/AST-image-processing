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
// This file is part of the mobile GUI of the ASTapp project

#ifndef ASTAPP_IMPROC_API_HPP
#define ASTAPP_IMPROC_API_HPP

#include "astimp.hpp"
#include "pellet_label_recognition.hpp"

using namespace std;

namespace astimp {

struct Pellet {
    Circle circle;
    InhibDisk disk;
    Label_match labelMatch;

    bool operator==(const Pellet &y) const {
        return circle == y.circle && disk == y.disk &&
               labelMatch == y.labelMatch;
    }
};

struct PelletsAndPxPerMm {
    vector<Pellet> pellets;
    // Measured on the original (non-preprocessed) image.
    float original_img_px_per_mm;
};

//! DEPRECATED: use findPetriDishWithRoi instead
cv::Rect findPetriDish(const string &astPicturePath);

PetriDish findPetriDishWithRoi(const string &astPicturePath, cv::Rect2i roi);

// Note: findPellet, findPelletFromApproxCoordinates, and findPellets
// always use InhibMeasureMode=INSCRIBED.
// measureOnePelletDiameter has the option to measure with InhibMeasureMode
// INSCRIBED or CIRCUMSCRIBED.
Pellet findPellet(const Circle &circle, const vector<Circle> &otherCircles,
                  const string &croppedPetriImgPath, bool isRound = false);

Pellet findPelletFromApproxCoordinates(const float centerX, const float centerY,
                                       const vector<Circle> &otherCircles,
                                       const string &croppedPetriImgPath,
                                       float mm_per_px);

PelletsAndPxPerMm findPellets(const string &croppedPetriImgPath,
                              bool isRound = false);

// Measures the diameter of the pellet at index pelletIdx in allCircles.
// Do not use this method in a loop to measure all diameters, because
// it runs preprocessing on each call (slow).
InhibDisk measureOnePelletDiameter(const string &croppedPetriImgPath,
                                   const vector<Circle> &allCircles,
                                   int pelletIdx, InhibMeasureMode mode,
                                   bool isRound = false);

float getPelletDiamInMm();

}  // namespace astimp

#endif  // ASTAPP_IMPROC_API_HPP
