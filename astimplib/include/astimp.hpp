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

#ifndef ASTIMP
#define ASTIMP

#include <map>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <string>

#include "astExceptions.hpp"

namespace astimp {

enum MEDIUM_TYPE {
    MEDIUM_HM,    // Mueller-Hinton standard growth medium
    MEDIUM_BLOOD  // blood enriched medium
};

class ImprocConfig {
    // configuration parameters for image processing
   private:
    struct PetriDishSettings {
        // === CROPPING PARAMETERS
        bool img_is_centered{true};
        // resized image longest side in px
        int MaxSize{200};
        // grubcut border (percent of shortest dimension)
        float gcBorder{0.03};
        // grubcut iterations
        int gcIters{10};
        // ===
        // Distance from the border of the Petri dish to the closest pellet
        // This value is used to crop the Petri dish image during preprocessing.
        // The cropped image will contain all the visible pellets plus a border
        // the size of borderPelletDistance_in_mm.
        int borderPelletDistance_in_mm{2};
        /* The ast growth medium is blood based (red)
         * this value is used during image preprocessing before diameter
         * measurement. it should be set to `true` if the growth medium is
         * blood-based (`false` otherwise) in order to obtain more accurate
         * results.
         * // TODO(marco) develop is_ast_blood_based(...) function (see below)
         * The `is_ast_blood_based(...)` function in utils.cpp can be used to
         * guess the kind of medium based on the Petri-dish image. Its return
         * value can be set to growthMedium */
        MEDIUM_TYPE growthMedium{MEDIUM_HM};
    };

    struct PelletsSettings {
        // percent of max image intensity beyond which the pixels belong to the
        // pellets used as threshold value in finding pellets
        float PelletIntensityPercent{0.97};
        // Max thinness allowed for an antibiotic disk
        float MaxThinness{1.05};
        // Pellet diameter in millimeters
        float DiamInMillimeters{6};
        // number of pellets that would fit aligned along the smallest
        // side of the image
        int minPictureToPelletRatio{30};
        // Circles larger than (min(width/height)/maxPictureToPelletRatio) will
        // not be considered as antibiotic pellets.
        int maxPictureToPelletRatio{4};
    };

    struct InhibitionSettings {
        // maximum possible inhibition diameter in millimeters
        int maxInhibitionDiameter{40};
        // The resolution of the preprocessed image for
        // inhibition diameter measurement
        int preprocImg_px_per_mm{10};
        // min and max difference between bacteria and inhibition intensity
        // used in diameter measurement to identify cases of no- and full
        // inhibition values in [0,255]
        int maxInhibToBacteriaIntensityDiff{90};
        int minInhibToBacteriaIntensityDiff{25};
        // Sensibility of diameter measurement.
        // This value must always be in the interval [0,1]
        // If st to zero, the sensibility will be automatically
        // adjusted based on the iamge contrast.
        float diameterReadingSensibility{0};
        // Number of attempts for kmeans.
        int kmeansNumAttempts{10};
        // Min pellet intensity.
        // Pixels above this intensity will be masked during preprocessing
        int minPelletIntensity{160};
    };

   public:
    PetriDishSettings PetriDish;
    PelletsSettings Pellets;
    InhibitionSettings Inhibition;

    ImprocConfig() = default;
};

struct PetriDish {
    cv::Mat img;           // cropped image of the petri dish
    cv::Rect boundingBox;  // bounding box
    bool isRound;          // the petri dish has the shape of a disk
    PetriDish(cv::Mat img, cv::Rect boundingBox, bool isRound)
        : img(img), boundingBox(boundingBox), isRound(isRound){};
    PetriDish(){};
};

class Circle {
   public:
    cv::Point2f center;  // center of the circle
    float radius;        // radius of the circle
    Circle(cv::Point2f center, float radius) : center(center), radius(radius){};

    Circle(){};

    bool operator==(const Circle &y) const {
        return center == y.center && radius == y.radius;
    }
};

class InhibDisk {
   public:
    // measurement of a diameter
    float diameter;
    float confidence;

    bool operator==(const InhibDisk &y) const {
        return diameter == y.diameter && confidence == y.confidence;
    }
};

class InhibDiamPreprocResult {
   public:
    // The standardised version of the AST image for inhibition diameter
    // measurement
    cv::Mat img;
    // pellets position in the standardized image
    vector<Circle> circles;
    // ROI of the inhibition zones
    vector<cv::Rect> ROIs;
    // intensity gray levels for inhibition, bacteria and pellets
    vector<int> km_centers;
    // intensity threshold value between inhibition and bacteria
    int km_threshold;
    // same as km_centers, but per ROI
    vector<vector<int>> km_centers_local;
    // same as km_threshold, but per ROI
    vector<int> km_thresholds_local;
    // the scale factor used for resize
    float scale_factor;
    // the padding in pixel
    int pad;
    // value of 1 millimeter in pixels in the preprocessed AST image
    float px_per_mm;
    // value of 1 millimeter in pixels in the original AST image, before
    // preprocessing
    float original_img_px_per_mm;
    InhibDiamPreprocResult(cv::Mat img, vector<Circle> circles,
                           vector<cv::Rect> ROIs, vector<int> km_centers,
                           int km_threshold,
                           vector<vector<int>> km_centers_local,
                           vector<int> km_thresholds_local, float scale_factor,
                           int pad, float px_per_mm,
                           float original_img_px_per_mm)
        : img(img),
          circles(circles),
          ROIs(ROIs),
          km_centers(km_centers),
          km_threshold(km_threshold),
          km_centers_local(km_centers_local),
          km_thresholds_local(km_thresholds_local),
          scale_factor(scale_factor),
          pad(pad),
          px_per_mm(px_per_mm),
          original_img_px_per_mm(original_img_px_per_mm){};
    InhibDiamPreprocResult(){};
};

enum PROFILE_TYPE {
    PROFILE_MEAN,
    PROFILE_MAX,
    PROFILE_SWITCH,  // Spatially Weighted Intensity Threshold CHangepoint
    PROFILE_MAXAVERAGE
};

/** Describes the appproach used to measure an inhibition diameter. */
enum InhibMeasureMode {
    /**
     * Diameter of the largest circle that can be inscribed inside
     * the inhibition zone without touching any bacteria.
     * INSCRIBED diameter is <= CIRCUMSCRIBED diameter.
     */
    INSCRIBED,
    /**
     * Diameter of the smallest circle that circumscribes (covers or
     * exceeds) the transition between bacteria and inhibition.
     * i.e. the transition between bacteria and inhibition is always
     * inside or touching a circle of the given diameter.
     * CIRCUMSCRIBED diameter is >= INSCRIBED diameter.
     */
    CIRCUMSCRIBED,
};

/* ---------------------------------- INIT ---------------------------------- */
/* Returns the current library configuration object. */
const ImprocConfig *getConfig();
/* Returns the current library configuration object, writable. */
ImprocConfig *getConfigWritable();

/* ---------------------------------- PETRI --------------------------------- */
PetriDish getPetriDish(const cv::Mat &img);
PetriDish getPetriDishWithRoi(const cv::Mat &ast_picture, const cv::Rect2i roi);
void calcDominantColor(const cv::Mat &img, int* hsv);
bool isGrowthMediumBlood(const cv::Mat &ast_crop);

/* --------------------------------- PELLETS -------------------------------- */
vector<Circle> find_atb_pellets(const cv::Mat &img);
cv::Mat cutOnePelletInImage(const cv::Mat &img, const Circle &circle,
                            bool clone = false);
vector<cv::Mat> cutPelletsInImage(const cv::Mat &img, vector<Circle> &circles);
float get_mm_per_px(const vector<astimp::Circle> &circles);
cv::Mat standardizePelletImage(const cv::Mat &pelletImg);

Circle searchOnePellet(const cv::Mat &img, int center_x, int center_y,
                       float mm_per_px);

/* -------------------------------- DIAMETERS ------------------------------- */
vector<cv::Rect> inhibition_disks_ROIs(const vector<Circle> &circles,
                                       const cv::Mat &img, float max_diam);
vector<cv::Rect> inhibition_disks_ROIs(const vector<Circle> &circles, int nrows,
                                       int ncols, float max_diam);
InhibDiamPreprocResult inhib_diam_preprocessing(PetriDish petri,
                                                vector<Circle> &circles);
InhibDiamPreprocResult inhib_diam_preprocessing(cv::Mat cropped_plate_img,
                                                bool isRound,
                                                vector<Circle> &circles);
InhibDisk measureOneDiameter(InhibDiamPreprocResult inhib_preproc,
                             int pellet_idx, InhibMeasureMode mode);
vector<InhibDisk> measureDiameters(InhibDiamPreprocResult inhib_preproc);

/// DEBUG ===============
vector<float> radial_profile(const cv::Mat &img, PROFILE_TYPE type,
                             int pellet_r_in_px, float th_value = 0);
vector<float> radial_profile(const InhibDiamPreprocResult &preproc,
                             unsigned int num, PROFILE_TYPE type,
                             float th_value = 0);

inline void throw_custom_exception(const string &s) {
    throw astimp::Exception::generic(s);
}
float calcDiameterReadingSensibility(InhibDiamPreprocResult preproc,
                                     int pellet_idx);
}  // namespace astimp

#endif  // ASTIMP
