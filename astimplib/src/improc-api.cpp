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

#include "improc-api.hpp"

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

namespace astimp {

class ImgCache {
   private:
    cv::Mat img;
    string imgPath;

   public:
    // Reads the image and throws an exception if that fails.
    cv::Mat get(const string &path) {
        if (imgPath != path) {
            imgPath = path;
            img = cv::imread(path, cv::IMREAD_COLOR);
        }
        if (img.empty()) {
            throw astimp::Exception::generic("Failed to read image", __FILE__,
                                             __LINE__);
        }
        return img;
    }
};

static ImgCache imgCache;

//! DEPRECATED: use findPetriDishWithRoi instead
cv::Rect findPetriDish(const string &astPicturePath) {
    /* return the bounding box of the petri dish in the image
     *
     * - astPicturePath pints to a whole AST picture (not cropped)
     */

    cv::Mat srcmat = imgCache.get(astPicturePath);
    PetriDish petri = getPetriDish(srcmat);
    return petri.boundingBox;
}

PetriDish findPetriDishWithRoi(const string &astPicturePath, cv::Rect2i roi) {
    /* return the bounding box of the petri dish in the image
     *
     * - astPicturePath points to a whole AST picture (not cropped)
     *
     * - roi defines a rectangle within the picture. The Petri dish should be
     * approximately within this rectangle̦
     */

    cv::Mat srcmat = imgCache.get(astPicturePath);
    PetriDish petri = getPetriDishWithRoi(srcmat, roi);
    return petri;
}

Pellet findPellet(const Circle &circle, const vector<Circle> &otherCircles,
                  const string &croppedPetriImgPath, bool isRound) {
    /*
     * @Brief Get the label and diameter of a new pellet
     * which has been manually found.
     *
     * - circle : The circle describing the new pellet
     *
     * - otherCircles : The other pellet (automatically found)
     * NOTE: using an empty vactor here will make the detection fail
     * because the inhibition_preprocessing needs to know where the pellets are.
     *
     * - croppedPetriImgPath points to the image of a cropped Petri dish
     */

    cv::Mat img = imgCache.get(croppedPetriImgPath);
    Pellet pellet;
    pellet.circle = circle;
    vector<Circle> pellets(otherCircles);
    pellets.push_back(circle);

    cv::Mat pelletMat = cutOnePelletInImage(img, circle);
    pellet.labelMatch = getOnePelletText(pelletMat);

    InhibDiamPreprocResult inhib =
        inhib_diam_preprocessing(img, isRound, pellets);
    pellet.disk = measureOnePelletDiameter(croppedPetriImgPath, pellets,
                                           pellets.size() - 1, INSCRIBED);
    return pellet;
}

Pellet findPelletFromApproxCoordinates(const float centerX, const float centerY,
                                       const vector<Circle> &otherCircles,
                                       const string &croppedPetriImgPath,
                                       float mm_per_px) {
    /* find a pellet py specifying its approximative center coordinates.
     *
     * - croppedPetriImgPath points to the image of a cropped Petri dish
     */

    cv::Mat img = imgCache.get(croppedPetriImgPath);
    const astimp::Circle c =
        astimp::searchOnePellet(img, centerX, centerY, mm_per_px);

    return findPellet(c, otherCircles, croppedPetriImgPath);
}

InhibDisk measureOnePelletDiameter(const string &croppedPetriImgPath,
                                   const vector<Circle> &allCircles,
                                   int pellet_idx, InhibMeasureMode mode,
                                   bool isRound) {
    vector<Circle> pellets(allCircles);
    InhibDiamPreprocResult preproc = inhib_diam_preprocessing(
        imgCache.get(croppedPetriImgPath), isRound, pellets);
    return measureOneDiameter(preproc, pellet_idx, mode);
}

PelletsAndPxPerMm findPellets(const string &croppedPetriImgPath, bool isRound) {
    /* find the antibiotic disks in a petri dish image
     *
     * - croppedPetriImgPath points to the image of a cropped Petri dish
     */

    cv::Mat img = imgCache.get(croppedPetriImgPath);
    vector<Circle> pellets = find_atb_pellets(img);

    InhibDiamPreprocResult inhib =
        inhib_diam_preprocessing(img, isRound, pellets);
    vector<astimp::InhibDisk> disks = astimp::measureDiameters(inhib);

    vector<cv::Mat> cutPellets = cutPelletsInImage(img, pellets);
    vector<Label_match> matches = astimp::getPelletsText(cutPellets);

    PelletsAndPxPerMm pelletsAndPxPerMm;
    vector<Pellet> result;
    pelletsAndPxPerMm.original_img_px_per_mm = inhib.original_img_px_per_mm;
    pelletsAndPxPerMm.pellets.reserve(pellets.size());
    for (size_t i = 0; i < pellets.size(); i++) {
        Pellet pellet{pellets[i], disks[i], matches[i]};
        pelletsAndPxPerMm.pellets.emplace_back(pellet);
    }
    return pelletsAndPxPerMm;
}

float getPelletDiamInMm() { return getConfig()->Pellets.DiamInMillimeters; }

}  // namespace astimp
