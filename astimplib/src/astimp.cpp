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

#include "astimp.hpp"

#include <cmath>
#include <limits>
// #include <algorithm>    // std::minmax
// #include <array>        // std::array
// #include <functional>   // std::minus
#include <cmath>
#include <numeric>  // std::accumulate
#include <vector>

#include "debug.hpp"
#include "utils.hpp"

#ifndef PROD_MODE
#ifdef USE_CVV
#include <opencv2/cvv/debug_mode.hpp>
#include <opencv2/cvv/dmatch.hpp>
#include <opencv2/cvv/filter.hpp>
#include <opencv2/cvv/final_show.hpp>
#include <opencv2/cvv/show_image.hpp>
#endif  // USE_CVV
#endif  // PROD_MODE

using namespace std;

namespace astimp {

struct ResizedImage {
    cv::Mat img;
    double scale;
    ResizedImage(const cv::Mat &img, double scale) : img(img), scale(scale){};
};

struct GrabCutResult {
    // store the grubcut processed results
    vector<cv::Point> contour;
    cv::Rect boundingbox;
    GrabCutResult(vector<cv::Point> contour, cv::Rect boundingbox)
        : contour(contour), boundingbox(boundingbox){};
};

ResizedImage resizeLongestDimension(cv::Mat img, uint maxSize);
GrabCutResult petri_bb_grabcut(cv::Mat ast_picture, bool useRoi = false,
                               cv::Rect2i roi = cv::Rect2i{0, 0, 0, 0});
typedef cv::Point3_<uint8_t> Pixel;

// TODO ----------------- REMOVE THIS BLOCK ----------------------- */
// configuration file name
// const char* configFileName = "./ASTappAPIConfig.yaml";
// configuration constants
// cv::FileStorage getConfig() {
//     cv::FileStorage config;
//     config.open(configFileName, cv::FileStorage::READ);
//     if (!config.isOpened()) {
//         throw astimp::Exception::generic("Config file not found", __FILE__,
//         __LINE__);
//     }
//     return config;
// }
// cv::FileStorage config = getConfig();
/* -------------------------------------------------------------------------- */

ImprocConfig improcConfig;

const ImprocConfig *getConfig() { return &improcConfig; }

ImprocConfig *getConfigWritable() { return &improcConfig; }
// class DebugOutput {
// public:
//     DebugOutput() {};
//     std::vector<cv::Mat> imgs;
//     std::vector<vector<cv::Point> > contours;
//     std::vector<cv::Rect> rectangles;
// };

ResizedImage resizeLongestDimension(cv::Mat img, uint maxSize) {
    cv::Mat resized;
    uint h = img.rows;
    uint w = img.rows;
    double scale = (double)maxSize / max<uint>(h, w);
    if (scale < 1) {
        cv::resize(img, resized, cv::Size(0, 0), scale, scale);
        return ResizedImage(resized, scale);
    } else {
        return ResizedImage(img, scale);
    }
}

vector<cv::Point> getGrabCutContour(cv::Mat mask) {
    typedef uint8_t Pixel;

    for (Pixel &p : cv::Mat_<Pixel>(mask)) {
        if (p % 2 == 0) {
            p = 0;
        }
    }

    // Extract contours
    std::vector<std::vector<cv::Point>> contours;
    findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // there must be at least one object in the image
    if (contours.size() == 0) {
        throw astimp::Exception::generic("no contours found", __FILE__,
                                         __LINE__);
    }

    std::vector<double> areas = std::vector<double>(contours.size());
    for (std::vector<cv::Point>::size_type i = 0; i != contours.size(); i++) {
        areas[i] = contourArea(contours[i]);
    }

    // select the longhest contours
    uint index = std::distance(areas.begin(),
                               std::max_element(areas.begin(), areas.end()));

    // DEBUG : draw the coutour
    // cv::Mat imageandcontours = img.clone();
    // drawContours(imageandcontours, contours, index, cv::Scalar(0,0,255));
    // cv::imshow("display", imageandcontours); cv::waitKey(0);

    return contours[index];
}

// TODO use const reference in arguments
GrabCutResult petri_bb_grabcut(cv::Mat ast_picture, bool useRoi,
                               cv::Rect2i roi) {
    cv::Rect2f rect;

    ResizedImage rimg =
        resizeLongestDimension(ast_picture, getConfig()->PetriDish.MaxSize);
    cv::Mat img = rimg.img;
    cv::Rect boundingBox;
    double scale = rimg.scale;

    if (useRoi) {
        // the boundingbox is specified by the user
        rect = cv::Rect2f(roi);
        rect.x *= scale;
        rect.y *= scale;
        rect.width *= scale;
        rect.height *= scale;

    } else {
        // define the rectangle of probable foreground for the grubCut algorithm
        float grubCutBorder = getConfig()->PetriDish.gcBorder;
        // log("gcBorder lib", grubCutBorder);
        // if true, the vertical and horizontal border will be different so that
        // the ROI is a square centered in the image
        bool adapted_borders = getConfig()->PetriDish.img_is_centered;
        int dimDelta = 0;
        int ROIsize = 0;
        if (adapted_borders) {
            dimDelta = abs(img.size[0] - img.size[1]) /
                       2;  // half the difference of the image dimensions
            ROIsize = (max({img.size[0], img.size[1]}) - 2 * dimDelta) *
                      (1 - 2 * grubCutBorder);  // size of the ROI

            if (img.size[0] > img.size[1]) {
                // image is vertical
                rect.x = img.size[1] * grubCutBorder;
                rect.y = dimDelta + rect.x;
            } else {
                // image is horizontal (or squared)
                rect.y = img.size[0] * grubCutBorder;
                rect.x = dimDelta + rect.y;
            }
            rect.width = ROIsize;
            rect.height = ROIsize;
        } else {
            if (img.size[0] > img.size[1]) {
                // image is vertical
                rect.x = img.size[1] * grubCutBorder;
                rect.y = img.size[0] * grubCutBorder;
            } else {
                // image is horizontal (or squared)
                rect.y = img.size[0] * grubCutBorder;
                rect.x = img.size[1] * grubCutBorder;
            }
            rect.width = img.size[1] - 2 * rect.x;
            rect.height = img.size[0] - 2 * rect.y;
        }
    }

    // DEBUG
    // cv::Mat dbg = img.clone();
    // cv::Scalar color( 0,0,UCHAR_MAX );
    // cv::rectangle(dbg, rect, color, 1);
    // cv::imshow("display",dbg); cv::waitKey(0);

    // Initialize the opencv random function's seed
    // for consistency of cropping result
    cv::Mat mask, bgModel, fgModel;

    cv::theRNG().state = 4294967295;

    // EXECUTE GRUBCUT (init with rect)
    grabCut(img, mask, rect, bgModel, fgModel, getConfig()->PetriDish.gcIters,
            cv::GC_INIT_WITH_RECT);

    vector<cv::Point> contour = getGrabCutContour(mask);

    try {
        boundingBox = boundingRect(contour);
    } catch (astimp::Exception::generic &e) {
        string message =
            "An error occurred while looking for the Petri dish in the "
            "image:\n" +
            e.message();
        throw astimp::Exception::generic(message, __FILE__, __LINE__);
    };

    // rescale the rectangle to bring it back to the original image size
    boundingBox.x /= scale;
    boundingBox.y /= scale;
    boundingBox.width /= scale;
    boundingBox.height /= scale;

    // cv::Point2f bbox_center = cv::Point2f(boundingBox.x + boundingBox.width /
    // 2,
    //                              boundingBox.y + boundingBox.height / 2),
    // float bbox_radius = min(boundingBox.width, boundingBox.height) / 2,

    return GrabCutResult(contour, boundingBox);
}

cv::Mat cropPetriDish(const cv::Mat &ast_picture, cv::Rect boundingBox) {
    return ast_picture(boundingBox).clone();
}

PetriDish getPetriDish(const cv::Mat &ast_picture) {
    /* @Brief get the Petri dish in the image.
     * The Petri dish is assumed to be the largest (possibly only) object in the
     * picture against a uniform background. The Petri dish is considered to be
     * entirely in the picture and not touching the image borders. */

    GrabCutResult gcres = petri_bb_grabcut(ast_picture);

    return PetriDish(cropPetriDish(ast_picture, gcres.boundingbox),
                     gcres.boundingbox, is_circle(gcres.contour, 1.1));
}

PetriDish getPetriDishWithRoi(const cv::Mat &ast_picture,
                              const cv::Rect2i roi) {
    /* @brief crop the Petri dish, which is approximately located in the roi
     * where roi is a rectangle (x,y,width,height) contained within the image.
     */

    if ((roi.width == 0) || (roi.height == 0)) {
        throw astimp::Exception::generic(
            "The custom bounding box has not been set", __FILE__, __LINE__);
    }
    if ((roi.width + roi.x > ast_picture.cols) ||
        (roi.height + roi.y > ast_picture.rows) || (roi.x > ast_picture.cols) ||
        (roi.y > ast_picture.rows)) {
        throw astimp::Exception::generic(
            "The custom bounding box must be contained in the image "
            "dimensions",
            __FILE__, __LINE__);
    }

    GrabCutResult gcres = petri_bb_grabcut(ast_picture, true, roi);

    return PetriDish(cropPetriDish(ast_picture, gcres.boundingbox),
                     gcres.boundingbox, is_circle(gcres.contour, 1.1));
}


void calcDominantColor(const cv::Mat &img, int* hsv) {
    /* Get the dominant hue,saturation values of the input image.
     * 
     * Only the 70% most saturated pixel are considered
     * 
     * This function will work only for red-yellow hues
     * (because of the peridicity of the hue space)
     * 
     * img : brg image
     * hs : output int array of size 3 (hue, saturation, value)
     */

    cv::Mat hsv_img;
    cv::cvtColor(img, hsv_img, cv::COLOR_BGR2HSV);

    // take only the most saturated pixels
    // caluclate the distribution function of the saturation
    cv::Mat hist;
    int channels[] = {1};
    int histSize[] = {256,};
    float sranges[] = { 0, 256 };
    const float* ranges[] = {sranges};

    cv::calcHist(&hsv_img, 1, channels, cv::Mat(), hist, 1, histSize,  ranges);
    // cumulative sum of the histogram
    vector<float> cum_hist = vector<float>(histSize[0],0);
    float old_value = 0;
    for (int i = 0; i<histSize[0]; i++){
        cum_hist[i] = old_value + hist.at<float>(i);
        old_value += hist.at<float>(i);
    }

    // find the saturation quantile at specified threshold
    int saturation_threshold = 0;
    for (int i = 0; i<histSize[0]; i++){
        if (cum_hist[i]/ cum_hist.back() > 0.5) {
            saturation_threshold = i;
            break;
        }
    }

    // Select only pixels that have a saturation higher than the threshold value
    cv::Mat sat_img;
    cv::extractChannel(hsv_img,sat_img,1);
    cv::Mat mask = cv::Mat::zeros(img.rows, img.cols, CV_8UC1);
    mask.setTo(1, sat_img > saturation_threshold);

    // Find mean hue, saturation and value
    cv::Mat pixels = cv::Mat::zeros(img.rows, img.cols, CV_32FC1);
    int N = cv::sum(mask)[0]; // number of selected pixels
    sat_img.copyTo(pixels, mask);
    int s = (int) round(cv::sum(pixels)[0]/N);  
    
    cv::Mat val_img;
    cv::extractChannel(hsv_img,val_img,2);
    val_img.copyTo(pixels, mask);
    int v = (int) round(cv::sum(pixels)[0]/N);   

    cv::Mat hue_img = cv::Mat::zeros(img.rows, img.cols, CV_8UC1);
    cv::extractChannel(hsv_img,hue_img,0);
    // rotate the hue space to remove the red discontinuity
    int temp;
    for (int r=0; r<hue_img.rows; r++) {
        for (int c=0; c<hue_img.cols; c++) {
            temp = hue_img.at<unsigned char>(r,c);
            temp = (temp +90)%180;
            hue_img.at<unsigned char>(r,c) = (unsigned char) temp;
        }
    }

    hue_img.copyTo(pixels, mask);
    // Compensate the hue rotation
    int h = ((int) round(cv::sum(pixels)[0]/N + 90))%180;  

    hsv[0] = h;
    hsv[1] = s;
    hsv[2] = v;
}

bool isGrowthMediumBlood(const cv::Mat &ast_crop) {
    /* Determine if the growth medium of an AST picture is blood enriched (HM-F).
    *
    * return boolean:
    *   true if the growth madium is red (which is the case of MH-F growth medium)
    *   false otherwise
    *  
    * Parameters:
    *   img: the cropped bgr Petri-dish image of an antibiogram picture.
    */

    // resize image
    cv::Mat resized_img;
    cv::resize(ast_crop, resized_img , cv::Size2i(50,50), 0,0);

    // get dominant color
    int hsv[3];
    calcDominantColor(resized_img, hsv);
    int h = hsv[0];
    int s = hsv[1];
    int v = hsv[2];

    // transform the hue coordinate so that red is centered and corresponds to hue=0
    // (useful for the logistic regression)
    h = (h+90)%180-90;

    // Classify.
    /* These coefficients are estimated with a logistic regression on Creteil's pictures.
    *
    * INSTRUCTIONS for updating these coefficients:
    *
    * Run he following operations for each image of a data-set containing both standard and
    * blood enriched antibiogram pictures:
    * 1. Crop the Petri dish from the image
    * 2. Extract mean hue, saturation and value with the calcDominantColor(...) function
    * 
    * Once the color extraction is done on the whole data-set, operate a logstic regression
    * in order to separate the values corresponding to MH and MH-F antibiograms.
    * (e.g. use sci-kit learn's linear_model.LogisticRegression function).
    * 
    * Copy the regression's parameter to use here for inference.
    */

    return (-0.00067274 + 0.04034141*s -0.28679131*v -0.17540224*pow(h,2) +0.00467521*pow(s,2)) > 0.5;

}


/*
 * ==================
 * FIND PELLETS
 * ==================
 */

bool isCircleOutsideImage(const Circle &circle, const cv::Mat &img) {
    if (circle.center.x < circle.radius || circle.center.y < circle.radius) {
        return true;
    }
    // Later, cutOnePelletInImage will cut out a rectangle of size
    // 2 * circle.radius + 2. +2 is needed to ensure that rectangle will not
    // fall off the image.
    int pellet_dim = circle.radius + 2;
    int circle_max_x = circle.center.x + pellet_dim;
    int circle_max_y = circle.center.y + pellet_dim;
    return circle_max_x > img.cols || circle_max_y > img.rows;
}

vector<Circle> find_atb_pellets(const cv::Mat &img) {
    /* @Brief Find the pellets in a cropped ast picture
     *
     * if the image is not grayscale, the blue channel only will be used.
     *
     * A cropped ast pictures displays only the Petri dish (no borders) */
    cv::Mat imgenorm, imgth, gray;

    // convert the input image to a one-channel image
    if (img.channels() > 1) {
        cv::extractChannel(
            img, gray,
            0);  // select the blule channel, which is more significant here
    } else {
        gray = img;
    }

    // cv::imshow("display", gray);cv::waitKey(0);

    // equalization of the image
    cv::equalizeHist(gray, gray);

    // normalization of the image
    cv::normalize(gray, imgenorm, 0, UCHAR_MAX, cv::NORM_MINMAX);
    // cv::imshow("display", gray);cv::waitKey(0);
    // threshold to select the pellets
    cv::threshold(imgenorm, imgth,
                  UCHAR_MAX * getConfig()->Pellets.PelletIntensityPercent,
                  UCHAR_MAX, cv::THRESH_BINARY);

    /////////////////////////////////////////////////////////
    // OPEN: eliminate whatever is smaller than the pellets
    /////////////////////////////////////////////////////////
    cv::Mat mask;

    /* NOTE We introduce the concept of minimum pellet size in terms of how many
    pellet would fit aligned one afther the other along the smallest side of the
    picture*/
    float minPelletDiam = min({img.size[0], img.size[1]}) /
                          (getConfig()->Pellets.minPictureToPelletRatio);
    float maxPelletDiam = min({img.size[0], img.size[1]}) /
                          (getConfig()->Pellets.maxPictureToPelletRatio);
    // use a structuring element which is 10 times smaller than the pellet
    int seSize = (int)(minPelletDiam / 10.0);
    // log("estimed pellet size", estimedPelletDiam);
    // log("SE size:", d);
    if (seSize <= 0) {
        throw astimp::Exception::generic(
            "Cropped AST image is too small (not enough pellets found).",
            __FILE__, __LINE__);
    }
    // structuring element for math-morphology operations
    cv::Mat se =
        getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(seSize, seSize));
    cv::morphologyEx(imgth, mask, cv::MORPH_OPEN, se);

    // cv::imshow("display", mask); cv::waitKey(0);
    // cvv::showImage(mask, CVVISUAL_LOCATION, "mask");

    vector<vector<cv::Point>> contours;
    vector<cv::Vec4i> hierarchy;
    // Finding coutours.
    cv::findContours(mask, contours, hierarchy, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);

    // VISUAL DEBUG
    // cv::Mat dbg;
    // cv::cvtColor(mask, dbg, cv::COLOR_GRAY2BGR);
    // cv::drawContours(dbg, contours, -1, cv::Scalar(0,UCHAR_MAX,0),10);
    // cv::imshow("display",dbg);cv::waitKey(0);
    // cvv::showImage(dbg, CVVISUAL_LOCATION, "contours");

    cv::Mat circleMat;
    vector<vector<cv::Point>> hull(contours.size());
    vector<Circle> circles;
    float radius;
    cv::Point2f p;

    float maxThinnes = getConfig()->Pellets.MaxThinness;
    // float diamTolerance = getConfig()->Pellets.DiameterTolerance;

    // For each contour found, is it a circle?
    for (int i = 0; i < (int)contours.size(); i++) {
        cv::convexHull(cv::Mat(contours[i]), hull[i], false);
        cv::approxPolyDP(cv::Mat(hull[i]), circleMat, 1, 1);
        circleMat.reshape(-1, 2);

        cv::minEnclosingCircle(circleMat, p, radius);

        Circle circle = Circle{p, radius};
        // Ignore pellets that would fall off the edge of the image.
        if (isCircleOutsideImage(circle, img)) {
            // log("excluded pellet outside bounds of cropped image: ", p);
            continue;
        }

        // keep only contours which size is larger than the estimed min pellet
        // size
        if (radius * 2 < minPelletDiam) {
            // log("excluded too-small pellet with diameter", radius*2);
            continue;
        }
        if (radius * 2 > maxPelletDiam) {
            // log("excluded too-big pellet with diameter", radius*2);
            continue;
        }
        // keep only contours which shape is close to that of a circle
        if (!is_circle(circleMat, maxThinnes)) {
            // log("excluded pellet with thinness:", thisThinness);
            continue;
        }
        circles.push_back(circle);
    }

    // cvv::finalShow();
    return circles;
}

/*
 * ==================
 * FIND PELLETS
 * ==================
 */

cv::Mat cutOnePelletInImage(const cv::Mat &img, const Circle &circle,
                            bool clone) {
    /* @Brief extract a pellet sub-images from an AST image

    Given an image of an AST and the circle that localizes one antibiotic pellet
    cut the sub-image of the pellet and return it.

    if (clone == false) the image data is not copied in memory (faster), but
    only the ROI of the cv::Mat is changed
    */
    int delta;
    cv::Point2i p;
    // calculate ROI from circle
    p.x = round(circle.center.x - circle.radius);
    p.y = round(circle.center.y - circle.radius);

    if (p.x < 0 || p.y < 0) {
        throw astimp::Exception::generic(
            "An error while cutting the pellets in the image, check image crop "
            "(probably the dish is too close to the borders of the image).",
            __FILE__, __LINE__);
    }
    delta = round(2 * (circle.radius + 1));
    cv::Mat temp;
    if (clone) {
        temp = img(cv::Rect2i(p.x, p.y, delta, delta)).clone();
    } else {
        temp = img(cv::Rect2i(p.x, p.y, delta, delta));
    }
    return temp;
}

vector<cv::Mat> cutPelletsInImage(const cv::Mat &img, vector<Circle> &circles) {
    /* @Brief Return the pellet sub-images in an AST image

        The location of the pellets in the image are specified in the circles
       vector.
     */

    // log("cutPelletInImage START");
    vector<cv::Mat> out;

    for (auto this_circle : circles) {
        out.push_back(cutOnePelletInImage(img, this_circle));
    }
    return out;
}

/* -------------------------------------------------------------------------- */
/*                                  DIAMETERS                                 */
/* -------------------------------------------------------------------------- */
double squareError(vector<float>::iterator first, vector<float>::iterator last,
                   float m) {
    /* @brief calculate the SE of a vector from first to last. Errors are
    distances from m. first, last Input iterators to the initial and final
    positions in a sequence. The range used is [first,last), which contains all
    the elements between first and last, including the element pointed by first
    but not the element pointed by last
    m
      the value with which the error is calculated: error[i] = (x[i] - m)
    */
    double se = accumulate(
        first, last, 0, [m](float x, float y) { return x + pow((y - m), 2); });
    return se;
}

double meanSquareError(vector<float>::iterator first,
                       vector<float>::iterator last, float m) {
    /* @brief calculate the MSE of a vector from first to last. Errors are
    distances from m. first, last Input iterators to the initial and final
    positions in a sequence. The range used is [first,last), which contains all
    the elements between first and last, including the element pointed by first
    but not the element pointed by last
    m
      the value with which the error is calculated: error[i] = (x[i] - m)
    */
    return squareError(first, last, m) / (last - first);
}

float calcDiameterReadingSensibility(InhibDiamPreprocResult preproc,
                                     int pellet_idx) {
    // calc diameter reading sensibility based on inhibition/bacteria contrast

    // image contrast
    auto kcl = preproc.km_centers_local[pellet_idx];
    float drs;  // diameterReadingSensibility

    // The reading sensibility is defined in config by the user
    // It has a value between 0 and 1
    // Zero is a special value that means sensibility is auto-calculated
    drs = astimp::getConfig()->Inhibition.diameterReadingSensibility;
    if (drs > 0) {
        return drs;
    }

    // CALCULATE THE SENSIBILITY AUTOMATICALLY
    // because drs == 0
    int minContrast =
        astimp::getConfig()->Inhibition.minInhibToBacteriaIntensityDiff;

    // In a case of no inhibition (contact between bacteria nad pellet), both of
    // the following are often true:
    // 1. There is very little contrast between detected bacteria and inhibition
    // for this pellet
    // 2. The local inhibition intensity is closer to the global bacteria
    // intensity than the global inhibition intensity (indicating that there is
    // no bacteria)
    int local_inhib = kcl[0];
    int local_bacteria = kcl[1];
    int global_inhib = preproc.km_centers[0];
    int global_bacteria = preproc.km_centers[1];

    float contrast_local = local_bacteria - local_inhib;
    int contrast_global = global_bacteria - global_inhib;

    float relative_contrast_diff =
        ((float)contrast_global - contrast_local) / contrast_global;
    bool is_big_contrast_difference = relative_contrast_diff >= 0.65;

    bool is_contact =
        is_big_contrast_difference && (abs(local_bacteria - global_inhib) >
                                       abs(local_bacteria - global_bacteria));

    // log(std::to_string(pellet_idx), is_contact);
    // log("---", abs((float) contrast-global_contrast)/global_contrast);

    if (is_contact) {
        // contact = no inhibition zone ==> use high drs
        drs = 0.7;
    } else {
        if (contrast_local <= minContrast) {
            // reduce sensib. for very low contrast
            drs = 0.05;
        } else {
            /**
             * 3 scenarios possible:
             * 1. local contrast == global contrast => drs=0.5.
             *    example: global == local == 30, drs=(0^3 + 1)/2 = 0.5
             *
             * 2. local contrast < global conrast => drs < 0.5
             *   Usually this happens because of a large inhibition zone.
             *   The sensibility should be increased
             *   example: global=90,local=30, drs=(0.67^3 + 1) / 2 = 0.64
             *
             * 3. local contrast > global contrast => drs > 0.5
             *   probably due to bright pixels associated with light reflection.
             *   Sensibility should be decreased.
             *   example: local=80, global=40, drs = (-.5^3 + 1) / 2 = 0.44
             *
             * About the way the drs is calculated:
             * let r = relative_contrast_diff
             * then
             *
             * (r+1)*0.5 is a linear function which decreases from 1 to zero
             when
             * local contrast goes from 0 to 2g and r=0.5 iif l=g
             *
             * The result is constrained by min and max because drs must be in
             [0,1]
             * the upper limit is 0.8 because 1 really means "no sensibility"
             and never
             * results in finding a breakpoint, which is not safe.
             *
             * by using r^3 (the third power of r) the function is still
             asymmetrical with respect to l=g, but
             * r deviates much from 0.5 only for large contrast differencies.
             *
             * Here is a plot of the drs function vs l/g
                  1 +-------------------------------------------------+
                    |*           +           +            +           |
                    | *                                               |
                0.8 |-+**                                           +-|
                    |    *                                            |
                    |     **                                          |
                0.6 |-+     ***                                     +-|
                    |          ****                                   |
                    |              *********************              |
                    |                                   ****          |
                0.4 |-+                                     ***     +-|
                    |                                          **     |
                    |                                            *    |
                0.2 |-+                                           **+-|
                    |                                               * |
                    |            +           +            +          *|
                    0 +-------------------------------------------------+
                    0           0.5          1           1.5          2

             */

            drs = (pow((relative_contrast_diff), 3) + 1) * 0.5;
            drs = min((float)0.8, drs);
            drs = max((float)0.0, drs);
        }
    }
    return drs;
}

InhibDisk measureOneInscribedDiameter(InhibDiamPreprocResult preproc,
                                      int pellet_idx) {
    vector<int> kmcl = preproc.km_centers_local[pellet_idx];

    // calculate diameter reading sensibility
    float drs = calcDiameterReadingSensibility(preproc, pellet_idx);

    // if the contrast is low, use global kmeans instead
    int minContrast =
        astimp::getConfig()->Inhibition.minInhibToBacteriaIntensityDiff;

    float contrast_local = abs(kmcl[1] - kmcl[0]);
    float contrast_global = preproc.km_centers[1] - preproc.km_centers[0];
    bool is_large_inhib = kmcl[1] < preproc.km_centers[0];
    float relative_contrast_diff =
        ((float)contrast_global - contrast_local) / contrast_global;
    bool is_big_contrast_difference = relative_contrast_diff >= 0.65;

    if (is_large_inhib ||
        (is_big_contrast_difference && (contrast_global > minContrast))) {
        // sometimes, even if the the global contrast is good
        // if the inhibition zone is very large, the local contrast
        // might be low: in this case use global k-means instead of local
        // k-means
        kmcl = preproc.km_centers;
    }

    // Threshold between inhibition and non-inhibition.
    // kmcl[0] and kmcl[1] are the intensities of inhibition
    // and bacteria.
    float inhib_intensity_threshold =
        (kmcl[0] + (kmcl[1] - kmcl[0]) * (1 - drs)) / UCHAR_MAX;

    vector<float> y_count = astimp::radial_profile(
        preproc, pellet_idx, astimp::PROFILE_SWITCH, inhib_intensity_threshold);

    auto minmaxresult = std::minmax_element(y_count.begin(), y_count.end());
    float lv = *minmaxresult.first;   // low value
    float hv = *minmaxresult.second;  // high value

    // log("\tlv", lv);
    // log("\thv", hv);

    size_t pellet_r_px = (size_t)ceil(getConfig()->Pellets.DiamInMillimeters *
                                      preproc.px_per_mm / 2.0);
    // log("\tpellet_radius_px", pellet_radius_px);

    double mse = 0;  // mean square error

    size_t inhib_r_px = 0;
    float inhib_diam_mm = -1;

    if (all_of(y_count.begin(), y_count.end(),
               [lv](float x) { return x > lv; })) {
        // evident case of no inhibition
        // log("\tno inhibition","");
    } else {
        if (y_count.size() <= pellet_r_px) {
            /* the intensity profile of a pellet is shorter than the pellet size
            in pixels, this can happen only if something is wrong with the
            image, for example an object was erroneously interpreted as a
            pellet. The pellet for which this error happens should be not
            measured.
            */
            throw astimp::Exception::generic(
                "The intensity profile is too short", __FILE__, __LINE__);
        }
        vector<float>::iterator data_start = y_count.begin() + pellet_r_px;
        if (all_of(data_start, y_count.end(), [](float x) { return x == 0; })) {
            // evident case of full inhibition
            inhib_r_px = y_count.size();
            // log("\tfull inhibition","");
            // log("\tdata_start",*data_start);
        } else {
            mse = meanSquareError(data_start, y_count.end(), hv);
            size_t non_pellet_px = y_count.size() - pellet_r_px;
            for (size_t px_i = 1; px_i < y_count.size() - pellet_r_px; px_i++) {
                double inhib_err =
                    squareError(data_start, data_start + px_i, lv);
                double bacteria_err =
                    squareError(data_start + px_i, y_count.end(), hv);
                double curr_mse = (inhib_err + bacteria_err) / non_pellet_px;
                if (curr_mse < mse) {
                    // This is the lowest error we've found so far.
                    mse = curr_mse;
                    inhib_r_px = px_i;
                }
            }
            inhib_r_px += pellet_r_px;
        }
        inhib_diam_mm = float(inhib_r_px * 2) / preproc.px_per_mm;
    }

    double confidence = 1.0;
    if (mse != 0) {
        // the confidence is caluclated as a ratio max_error/mse
        // max_error is the estimation of the mse due to a glitch of size
        // allowed_error_mm in the profile data.
        float allowed_error_size_in_px = preproc.px_per_mm * 1.5;
        float max_error_per_px = hv;
        float max_error = allowed_error_size_in_px * max_error_per_px;
        confidence = min(max_error / mse, 1.0);
        // log("confidence", confidence);
    }

    // diameter can not be larger than max diameter
    float max_diameter = getConfig()->Inhibition.maxInhibitionDiameter;
    inhib_diam_mm = min(inhib_diam_mm, max_diameter);

    return InhibDisk{inhib_diam_mm, (float)confidence};
}

InhibDisk measureOneCircumscribedDiameter(InhibDiamPreprocResult ip,
                                          int pellet_idx) {
    /* @brief Measures the inhibition diameter of one pellet.
     *
     * pellet_idx specifies the pellet index. The order of the pellet is the
     * same of "circles" used to generate InhibDiamPreprocResult.
     *
     */

    float max_diameter =
        getConfig()
            ->Inhibition.maxInhibitionDiameter;  // max inhibition diameter
    uint pr =
        (uint)ceil((getConfig()->Pellets.DiamInMillimeters * ip.px_per_mm) /
                   2);  // pellet radius in pixel

    // WARNING : REMEMBER THAT THERE ARE -1 VALUES IN THE IMAGE so do not
    // reconvert the image here!!!
    vector<float> profile = astimp::radial_profile(
        ip, pellet_idx, astimp::PROFILE_MEAN);  // get the radial profile

    // eliminate from the profile the part corresponding to the pellet
    vector<float> y(profile.size() - pr);
    copy(profile.begin() + pr, profile.end(), y.begin());

    // skip residual pellet high values at the beginning of the profile
    float y_old = y[0];
    uint pellet_end_idx = 0;
    for (size_t i = 1; i < y.size(); i++) {
        if (round(y[i] - y_old) < 0) {
            // if the profile is decreasing
            y_old = y[i];
        } else {
            pellet_end_idx = i;
            break;
        }
    }

    // find the intensity threshold corresponding to the breakpoint
    float min_y = *min_element(y.begin() + pellet_end_idx,
                               y.end());  // max intensity value in this profile
    float max_y = *max_element(y.begin() + pellet_end_idx,
                               y.end());  // min value in this profile
    float half = (max_y + min_y) / 2 * 1.1;

    // float half = ip.km_thresholds_local[pellet_idx];

    int breakpoint;
    float diameter;

    // check if there is no inhibition in the first millimeter from the pellet
    // end
    uint delta_start = pellet_end_idx + ceil(ip.px_per_mm * 0.5);
    uint delta_end = pellet_end_idx + ceil(ip.px_per_mm * 1);

    float old_y;

    if (y.begin() + delta_end < y.end() && delta_end - delta_start != 0) {
        float y_miap =
            std::accumulate(y.begin() + delta_start, y.begin() + delta_end,
                            0) /
            (delta_end -
             delta_start);  // mean of y immediatly afyter the pellet

        if (y_miap > ip.km_thresholds_local[pellet_idx]) {
            // the intensity value just after the pellet is above the threshold
            diameter = getConfig()->Pellets.DiamInMillimeters;
            goto end;
        }
    }

    // find the breackpoint
    old_y = y[pellet_end_idx];
    for (size_t i = pellet_end_idx + 1; i < y.size(); i++) {
        if ((half - y[i] <= 1) && (y[i] - old_y > 0)) {
            // the intensity value is close enough to the threshold value
            // and the intensity is growing
            breakpoint = i + pr;  // adding the pellet radius originally
                                  // eliminated in the prfile
            diameter = min((float)breakpoint / ip.px_per_mm * 2, max_diameter);
            goto end;
        }
    }

    // nothing worked
    diameter = max_diameter;

end:

    // Calculate min RootMeanSquare error against a step function that changes
    // at breakpoint
    float rms = 0;
    // float low_mean = accumulate( y.begin(), y.begin() + breakpoint-pr,
    // 0.0)/y.size(); float hig_mean = accumulate( y.begin() + breakpoint-pr,
    // y.end(), 0.0)/y.size(); for (size_t i = pellet_end_idx; i < y.size();
    // i++)
    // {
    //   if (i < breakpoint-pr) {
    //     rms = rms + pow(y[i]-low_mean,2);
    //   } else {
    //     rms = rms + pow(y[i]-hig_mean,2);
    //   }
    // }
    // rms = sqrt(rms/y.size());

    // DEBUG
    // log("pellet end idx", pellet_end_idx);
    // log("half", half);
    // log("breackpoint", breackpoint);
    // log("diameter", diameter);
    // vector_to_csv("tests/profile.csv", profile);
    // int err = system("gnuplot --persist 'tests/script.plt'");
    // log("gnuplot err:", err);

    return InhibDisk{diameter, rms};
}

InhibDisk measureOneDiameter(InhibDiamPreprocResult preproc, int pellet_idx,
                             InhibMeasureMode mode) {
    switch (mode) {
        case CIRCUMSCRIBED:
            return measureOneCircumscribedDiameter(preproc, pellet_idx);
        case INSCRIBED:
        default:
            return measureOneInscribedDiameter(preproc, pellet_idx);
    }
}

InhibDiamPreprocResult inhib_diam_preprocessing(cv::Mat cropped_plate_img,
                                                bool isRound,
                                                vector<Circle> &circles) {
    /* preprocessing without PetriDish object */
    PetriDish petri(
        cropped_plate_img,
        cv::Rect{0, 0, cropped_plate_img.cols, cropped_plate_img.rows},
        isRound);
    return inhib_diam_preprocessing(petri, circles);
}

InhibDiamPreprocResult inhib_diam_preprocessing(PetriDish petri,
                                                vector<Circle> &circles) {
    // TODO(Marco): Split this method into multiple, perhaps using a class.
    // TODO: refactoring could probably improve the performances

    /* @brief create an image that is optimal for inhibition diameter
     * measurement
     *
     * the input image must be BGR cropped image of a Petri dish.
     *
     * This image is cropped rescaled and resized in so that it can be used for
     * the measurement of the inibition diameters.
     */

    cv::Mat img;
    if (astimp::getConfig()->PetriDish.growthMedium == MEDIUM_BLOOD) {
        // copy green channel into red channel
        int fromto[] = {0, 0, 1, 1, 1, 2};
        img = petri.img.clone();
        cv::mixChannels(&petri.img, 1, &img, 1, fromto, 3);
    } else {
        // copy blue channel into red channel
        img = petri.img;
    }
    // cv::imshow("display", img); cv::waitKey(0);

    cv::Mat crop, std, temp, crop_color, debug;
    auto inhibConfig = getConfig()->Inhibition;
    float original_img_px_per_mm = 1.0 / get_mm_per_px(circles);

    // convert the input image to a one-channel image
    cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);

    vector<cv::Point2f> centers(circles.size(), cv::Point2f(0, 0));
    vector<cv::Point2f> new_centers(circles.size(), cv::Point2f(0, 0));
    vector<float> radii(circles.size(), 0);
    vector<Circle> new_circles(circles.size(), Circle{cv::Point2f(0, 0), 0});

    // Normalize
    cv::normalize(img, img, UCHAR_MAX, 0, cv::NORM_MINMAX);

    // erase label text
    for (size_t i = 0; i < circles.size(); i++) {
        cv::circle(img, circles[i].center, circles[i].radius,
                   cv::Scalar(UCHAR_MAX), cv::FILLED, 0, 0);
    }
    // cv::imshow("display", img); cv::waitKey(0);

    // get a list of the centers of the pellets
    for (size_t i = 0; i < centers.size(); i++) {
        centers[i] = circles[i].center;
        radii[i] = circles[i].radius;
    }

    // find the ROI that includes all the pellets centers
    cv::Rect bbox, bbox_strict;
    float max_r = vector_min(radii);
    float total_roi_border =
        max_r + original_img_px_per_mm *
                    astimp::getConfig()->PetriDish.borderPelletDistance_in_mm;
    if (!petri.isRound) {
        bbox_strict = cv::boundingRect(centers);
        bbox_strict.x -= max_r;
        bbox_strict.y -= max_r;
        bbox_strict.height += round(2 * max_r);
        bbox_strict.width += round(2 * max_r);
        bbox = cv::boundingRect(centers);
        // enlarge the ROI so that it includes the whole images of the pellets

        bbox.x = bbox.x - total_roi_border;
        bbox.y = bbox.y - total_roi_border;
        if (bbox.x < 0 || bbox.y < 0) {
            throw astimp::Exception::generic(
                "Error while preprocessing. bbox.x < 0 || bbox.y < 0.",
                __FILE__, __LINE__);
        }
        bbox.height += round(total_roi_border * 2);
        bbox.width += round(total_roi_border * 2);
    } else {
        float mec_radius;
        cv::Point2f mec_center;

        // NOTE: The following code calculates the bouding-box
        //       as the smallest circle including all the pellets.
        // cv::minEnclosingCircle(centers, mec_center, mec_radius);
        // mec_radius += total_roi_border;

        // Calculate the bouding box the smallest circle centered in
        // the image center, which includes all the pellets
        float image_cx = img.cols / 2;
        float image_cy = img.rows / 2;
        mec_radius = -1;
        mec_center = cv::Point2f(image_cx, image_cy);
        for (size_t i = 0; i < centers.size(); i++) {
            float curr_dist = pow((centers[i].x - image_cx), 2) +
                              pow((centers[i].y - image_cy), 2);
            if (curr_dist > mec_radius) {
                mec_radius = curr_dist;
            }
        }
        mec_radius = sqrt(mec_radius);
        mec_radius += total_roi_border;
        bbox = getBoundingRectFromCircle(mec_center, mec_radius);
        mec_radius += max_r;
        bbox_strict = getBoundingRectFromCircle(mec_center, mec_radius);
        // log("mec_radius",max_dist_from_center);
    }

    cv::Mat crop_strict;
    img(bbox).copyTo(crop);
    img(bbox_strict).copyTo(crop_strict);  // for k-means only
    // cv::imshow("display",crop_strict); cv::waitKey(0);
    // cv::imshow("display",crop); cv::waitKey(0);

    //* Calculate rescaling factor
    float px_per_mm = astimp::getConfig()->Inhibition.preprocImg_px_per_mm;
    float resize_f;
    if (px_per_mm < original_img_px_per_mm) {
        resize_f = ((float)px_per_mm) / original_img_px_per_mm;
    } else {
        px_per_mm = original_img_px_per_mm;
        resize_f = 1;
    }
    // float rescaled_size = inhibConfig.stdImageSize;
    // float resize_f = 1;
    // float px_per_mm = original_img_px_per_mm;
    // if (inhibConfig.resizeStdImage) {
    //     resize_f = rescaled_size / max(crop.rows, crop.cols);
    //     px_per_mm *= resize_f;
    // }

    //* BLUR (for noise reduction)
    // median blur kernel size
    // it should be equivalent to half millimeter in image scale
    int mbKernelSize = (int)floor(max(((double)px_per_mm) / 2, 3.0));
    if (mbKernelSize % 2 == 0) {
        mbKernelSize = mbKernelSize + 1;
    }
    cv::Mat tempBlurImg;
    cv::medianBlur(crop, tempBlurImg, mbKernelSize);
    tempBlurImg.copyTo(crop);
    cv::medianBlur(crop_strict, tempBlurImg, mbKernelSize);
    tempBlurImg.copyTo(crop_strict);
    // cv::imshow("display", tempBlurImg); cv::waitKey(0);

    // MASK DISH BODERS WITH UCHAR_MAX for global k-means
    crop_strict.copyTo(temp);
    if (petri.isRound) {
        // remove data outside the minimum enclosing circle
        cv::Mat mask = cv::Mat::zeros(crop_strict.size(), CV_8U);
        cv::circle(mask,
                   cv::Point2f(crop_strict.cols / 2, crop_strict.rows / 2),
                   crop_strict.rows / 2, cv::Scalar(255), cv::FILLED);
        // cv::imshow("display",mask); cv::waitKey(0);
        temp = cv::Mat::ones(crop.size(), CV_8U) * UCHAR_MAX;
        cv::bitwise_and(crop_strict, crop_strict, temp, mask);
    }
    // crop_strict.copyTo(temp);
    // cv::imshow("display",crop); cv::waitKey(0);
    // cv::imshow("display",temp); cv::waitKey(0);

    //* Calculate padding
    float max_inib_r =
        inhibConfig.maxInhibitionDiameter / 2 * original_img_px_per_mm;

    //* Map levels in [0,1]
    cv::normalize(crop, temp, 1, 0, cv::NORM_MINMAX, CV_32F);

    // MASK plastic borders
    cv::Mat mask;
    if (petri.isRound) {
        // mask data outside the minimum enclosing circle
        mask = cv::Mat::zeros(temp.size(), CV_8U);
        cv::circle(mask, cv::Point2f(temp.cols / 2, temp.rows / 2),
                   temp.rows / 2, cv::Scalar(255), cv::FILLED);
        // cv::imshow("display",mask); cv::waitKey(0);
        std = cv::Mat::ones(crop.size(), CV_32F) * -1;
        cv::bitwise_and(temp, temp, std, mask);
    } else {
        temp.copyTo(std);
    }
    // cv::imshow("display",std); cv::waitKey(0);

    //* MASK PELLETS WITH -1
    // make pellet pixels value -1
    for (int y = 0; y < std.rows; y++) {
        for (int x = 0; x < std.cols; x++) {
            int minPelletIntensity =
                astimp::getConfig()->Inhibition.minPelletIntensity;
            if (std.at<float>(y, x) >=
                ((float)minPelletIntensity) / UCHAR_MAX) {
                std.at<float>(y, x) = -1;
            }
        }
    }
    // cv::imshow("display", std); cv::waitKey(0);

    //* Rescale the image (before padding)
    cv::resize(std, std, cv::Size(0, 0), resize_f, resize_f);

    //* Apply PADDING:
    // pad the ROI with the size of the maximum inhibition radius
    int pad = (int)round((max_inib_r + 1 * px_per_mm) *
                         resize_f);  // add one millimeter for safety
    cv::copyMakeBorder(std, std, pad, pad, pad, pad, cv::BORDER_CONSTANT, -1);
    // cv::imshow("display", std); cv::waitKey(0);

    //* Calculate new circles in std image
    for (size_t i = 0; i < centers.size(); i++) {
        new_centers[i].x = (centers[i].x - bbox.x) * resize_f + pad;
        new_centers[i].y = (centers[i].y - bbox.y) * resize_f + pad;
        new_circles[i].center = new_centers[i];
        new_circles[i].radius = ((radii[i]) * resize_f);
    }

    /* Mask pixels which are closer to the border of the plate
     * the std image includes an extra border specified in config
     * (borderPelletDistance_in_mm) this border may include the plastic borders
     * of the plate, which may affect the value of the local k-means estimation.
     *
     * Here a std_stric image is produced, where the exernal pixels are masked.
     * this image is used only in the local k-means calculation.
     */
    max_r *= resize_f;
    mask = cv::Mat::zeros(std.size(), CV_8U);
    cv::Mat std_strict = cv::Mat::ones(std.size(), CV_32F) * -1;
    if (petri.isRound) {
        float mec_radius;
        cv::Point2f mec_center;
        // remove data outside the minimum enclosing circle
        cv::minEnclosingCircle(new_centers, mec_center, mec_radius);
        mec_radius += max_r;
        cv::circle(mask, mec_center, mec_radius, cv::Scalar(255), cv::FILLED);
        // cv::imshow("display",mask); cv::waitKey(0);
        cv::bitwise_and(std, std, std_strict, mask);
    } else {
        bbox_strict = cv::boundingRect(new_centers);
        bbox_strict.x -= max_r;
        bbox_strict.y -= max_r;
        bbox_strict.height += round(2 * max_r);
        bbox_strict.width += round(2 * max_r);
        cv::rectangle(mask, bbox_strict, cv::Scalar(255), cv::FILLED);
        // cv::imshow("display",mask); cv::waitKey(0);
        cv::bitwise_and(std, std, std_strict, mask);
    }
    // cv::imshow("display", std); cv::waitKey(0);
    // cv::imshow("display", std_strict); cv::waitKey(0);

    //* GLOBAL K-MEANS
    //* get the intensity thresholds of bacteria and inhibition by k-means
    float km_resize_f = 150.0 / max(temp.rows, temp.cols);
    vector<int> km_centers;
    std_strict.copyTo(temp);
    cv::resize(temp, temp, cv::Size(0, 0), km_resize_f, km_resize_f);
    km_centers = masked_k_means(temp, 2);

    //* get the inhibition ROIs centered on each pellet (added 2 mm for better
    // reading)
    vector<cv::Rect> inhib_ROIs = inhibition_disks_ROIs(
        new_circles, std, (max_inib_r + 2 * px_per_mm) * resize_f);

    //* local k-means: Cluster pellets vs. inhibition vs. bacteria for
    //* each individual antibiotic.
    vector<vector<int>> km_centers_local =
        vector<vector<int>>(inhib_ROIs.size(), vector<int>(2, 0));
    vector<int> km_thresholds_local = vector<int>(inhib_ROIs.size(), 0);

    cv::Mat selection;
    //* calculate k-means local values
    for (size_t i = 0; i < inhib_ROIs.size(); i++) {
        auto roi = inhib_ROIs[i];
        //* select the roi pixel, remove border (value < 0)
        std_strict(roi).copyTo(temp);

        // resize for speed
        if (max(temp.rows, temp.cols) > 150) {
            km_resize_f = 150.0 / max(temp.rows, temp.cols);
            cv::resize(temp, temp, cv::Size(0, 0), km_resize_f, km_resize_f);
        }

        // apply k-means to positive valued pixels
        vector<int> this_km_centers = masked_k_means(temp, 2);

        // log(">>> km[0]", this_km_centers[0]);
        // log(">>> km[1]", this_km_centers[1]);
        // log(">>> km[2]", this_km_centers[2]);

        // store the result
        km_centers_local[i] = this_km_centers;
        float drs = inhibConfig.diameterReadingSensibility;
        km_thresholds_local[i] =
            this_km_centers[0] +
            (this_km_centers[1] - this_km_centers[0]) * (1 - drs);
    }

    // DEBUG display labels image
    // rows is the number of tows of temp before reshaping it (uncomment the
    // dclaration) labels = labels.reshape(0, rows);
    // cv::normalize(labels,labels,UCHAR_MAX,0,cv::NORM_MINMAX, CV_8U);
    // cv::imshow("display", labels);
    // cv::waitKey(0);

    // make pellets all white in the new preprocessed image
    cv::Mat std_white_pellets(std);
    for (size_t i = 0; i < new_circles.size(); i++) {
        cv::circle(std_white_pellets, new_circles[i].center,
                   new_circles[i].radius * 1, cv::Scalar(1), cv::FILLED, 0, 0);
    }
    // cv::imshow("display", std_white_pellets); cv::waitKey(0);

    // extract the radial profiles
    return InhibDiamPreprocResult(
        std_white_pellets, new_circles, inhib_ROIs, km_centers,
        (km_centers[1] + km_centers[0]) / 2, km_centers_local,
        km_thresholds_local, resize_f, pad, px_per_mm, original_img_px_per_mm);
}

vector<float> radial_profile(const InhibDiamPreprocResult &preproc,
                             unsigned int num, PROFILE_TYPE type,
                             float th_value) {
    /* @Brief return the radial intensity profile of the num-th pellet */
    if (num >= preproc.ROIs.size()) {
        // check that num is a valid index
        throw astimp::Exception::generic("num of pellet out of range", __FILE__,
                                         __LINE__);
    }
    return astimp::radial_profile(preproc.img(preproc.ROIs[num]), type,
                                  preproc.px_per_mm, th_value);
}

vector<float> radial_profile(const cv::Mat &img, PROFILE_TYPE type,
                             int px_per_mm, float th_value) {
    /* @Brief return the radial intensity profile of an image,
     *
     * The center is considered in the middle of the image
     *
     * The input image is expected to have be CV_32F
     * Pixels of value -1 are excluded from the calculation of the radial
     * profile
     *
     * profile types:
     *  - PROFILE_MEAN: average of the intensities at same radius
     *  - PROFILE_MAX: maximum intensity per radius
     *  - PROFILE_MAXAVERAGE: average of the highest intensity values per radius
     *  - PROFILE_SWITCH: if at a given radius the intensity is higher than th_v
     * for a sufficent number of pixels the profile value is 255, otherwise 0.
     * */

    // check that the image is of type CV_32F
    if (img.channels() != 1 || img.depth() != CV_32F) {
        throw astimp::Exception::generic(
            "wrong image type. Expected 1 channel CV_32F", __FILE__, __LINE__);
    }

    if (type < 0 || type > PROFILE_MAXAVERAGE) {
        throw astimp::Exception::generic("wrong type for radial profile",
                                         __FILE__, __LINE__);
    }

    uint n = img.rows;
    vector<vector<uint>> R = r_matrix(n);

    // profile is the output variable, the radial profile.
    // The index is interpreted as radius in pixel.
    // To each radius value, profile associates a y value which meaning depends
    // on the chosen profile_type
    vector<float> profile(n / 2 + 1, 0);

    // Intensity values of all the pixels at a given radius (in pixel)
    vector<vector<float>> intensities_by_r(n / 2 + 1, vector<float>());

    // the number of pixels at a given radius (~ 2*PI*r)
    vector<float> counts(n / 2 + 1, 0);

    float val;  // current pixel value
    size_t r;   // current radius

    // iteration over all the pixels
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            r = R[i][j];
            if (r > n / 2)
                continue;  // radius must not be > than half image size
            val = img.at<float>(i, j);
            if (val < 0) continue;  // skip negative values
            switch (type) {
                case PROFILE_MEAN:
                    profile[r] = profile[r] + val;
                    break;
                case PROFILE_MAX:
                    if (val > profile[r]) profile[r] = val;
                    break;
                case PROFILE_MAXAVERAGE:
                    intensities_by_r[r].push_back(val);
                    break;
                case PROFILE_SWITCH:
                    if (val > th_value) profile[r] = profile[r] + 1;
                    break;
            }
            counts[r] = counts[r] + 1;
        }
    }

    // calculation of the profile
    size_t px_per_mm_int = (size_t)round(px_per_mm);
    switch (type) {
        case PROFILE_MEAN:
            for (size_t i = 0; i < profile.size(); i++) {
                profile[i] = profile[i] / counts[i];
            }
            break;
        case PROFILE_MAX:
            break;
        case PROFILE_MAXAVERAGE:
            //* average the largest values
            profile[0] = intensities_by_r[0][0];  // r=0 there is only one pixel
            for (size_t r = 1; r < profile.size(); r++) {
                // number of elements to average
                uint average_size =
                    (uint)min((int)intensities_by_r[r].size(), 2 * px_per_mm);
                average_size = max(average_size, (uint)1);  // avoid sz == 0
                // sort descending (high to low)
                sort(intensities_by_r[r].begin(), intensities_by_r[r].end());
                // average over the sz first values
                profile[r] =
                    accumulate(intensities_by_r[r].rbegin(),
                               intensities_by_r[r].rbegin() + average_size,
                               0.0) /
                    average_size;
            }
            break;
        case PROFILE_SWITCH:
            for (size_t i = 0; i < profile.size(); i++) {
                if (i < px_per_mm_int) {
                    profile[i] = 1;
                } else if (profile[i] > 2 * px_per_mm) {
                    // if the bacteria pixels in this circle occupy more than
                    // the distance specified in the right member of the
                    // inequality.
                    profile[i] = 1;
                } else if (profile[i] > px_per_mm) {
                    profile[i] = profile[i] / (2 * px_per_mm);
                } else {
                    profile[i] = 0;
                }
            }
            break;
    }

    // rescale to uint8 to be coherent among all profile types
    for (size_t i = 0; i < profile.size(); i++) {
        profile[i] = profile[i] * UCHAR_MAX;
    }

    // return vector<float>(3,0);
    return profile;
}
vector<cv::Rect> inhibition_disks_ROIs(const vector<Circle> &circles,
                                       const cv::Mat &img, float max_diam) {
    int nrows = img.rows;
    int ncols = img.cols;
    return inhibition_disks_ROIs(circles, nrows, ncols, max_diam);
}

vector<cv::Rect> inhibition_disks_ROIs(const vector<Circle> &circles, int nrows,
                                       int ncols, float max_diam) {
    /* @brief return a list of ROIs centered on each pellet
     *
     * Each ROI is centered on one pellet and is as big as possible without
     * including other pellets.
     *
     * max_diam is the maximum diameter that can be measured (in pixels)
     */

    vector<cv::Rect> out(circles.size(), cv::Rect(0, 0, 0, 0));

    // For each pellet, the 1st neibourgh distance determines the max inhibirion
    // radius.
    vector<float> fnd = first_neighbour_distance(circles);

    int cx, cy, r;

    // calculate ROI for each pellet
    for (size_t i = 0; i < fnd.size(); i++) {
        cx = (int)round(circles[i].center.x);
        cy = (int)round(circles[i].center.y);
        // avoid overlap with other pellets by removing some pixels
        r = (int)round(min(fnd[i] - circles[i].radius, max_diam));

        // If there is just one pellet, take the max diameter.
        if (fnd.size() == 1) {
            r = (int)round(max_diam);
        }

        int roi_width = r * 2 + 1;
        if (roi_width < 0) {
            throw astimp::Exception::generic(
                "Invalid pellet position found (maybe 2 pellets are too "
                "close?).",
                __FILE__, __LINE__);
        }

        // calculate ROI (safe)
        int left = max(0, cx - r);
        int top = max(0, cy - r);
        int right = min(cx - r + roi_width, ncols - 1);
        int bottom = min(cy - r + roi_width, nrows - 1);

        if (right < left || bottom < top) {
            throw astimp::Exception::generic(
                "Error with inhibition ROIs. ROI has negative values", __FILE__,
                __LINE__);
        }

        out[i].x = left;
        out[i].y = top;
        out[i].width = right - left;
        out[i].height = bottom - top;
    }

    return out;
}

vector<InhibDisk> measureDiameters(InhibDiamPreprocResult inhib_preproc) {
    /*
        @brief Measure the inhibition disks on a cropped image of a petri dish.

        The result diameters are in pixels (not in millimeters).

        return:
            a list of diameters of the measured inhibition disks diameters and
       the confidence values.
      */

    vector<InhibDisk> disks(inhib_preproc.circles.size());
    cv::Mat crop;

    for (size_t i = 0; i < disks.size(); i++) {
        disks[i] = measureOneInscribedDiameter(inhib_preproc, i);
    }

    return disks;
}

//// OTHERS
float get_mm_per_px(const vector<astimp::Circle> &circles) {
    /* @brief Returns how many millimeters are there per pixel.
     *
     * The scale is calculated on the average pellet diameter in pixel
     * The reference diameter in mm is taken from the configuration file
     */

    float sum = 0;
    float diam_in_px = 0;  // mean diameter in pixel
    float diam_in_mm = getConfig()->Pellets.DiamInMillimeters;

    for (Circle circle : circles) {
        sum = sum + circle.radius;
    }

    // mean of the diameters in pixel
    diam_in_px = (sum / circles.size()) * 2;

    return diam_in_mm / diam_in_px;
}

astimp::Circle searchOnePellet(const cv::Mat &img, int center_x, int center_y,
                               float mm_per_px) {
    /* @Brief locate a pellet in the neighborhood of a given point in the image

    Params:
        - img : cropped AST image
        - center_x, center_y : coordinates of the point in pixels (x = image
    columns)
        - mm_per_px : scale of img

    Return:
        - astimp::Circle containing the center and the radius of the pellet
    */

    cv::Mat roi_img, gray, blur, threshold_img;

    int pellet_diam_in_mm = astimp::getConfig()->Pellets.DiamInMillimeters;
    int pellet_diam_in_px = pellet_diam_in_mm / mm_per_px;
    int roi_width = (int)round(2 * pellet_diam_in_px);

    // Calculate (safe) roi boundaries
    int left = max(0, center_x - pellet_diam_in_px);
    int top = max(0, center_y - pellet_diam_in_px);
    int right = min(left + roi_width, img.cols - 1);
    int bottom = min(top + roi_width, img.rows - 1);

    //* SELECT ROI around the given point
    cv::Rect roi_rect =
        cv::Rect(cv::Point2i(left, top), cv::Point2i(right, bottom));
    roi_img = img(roi_rect);

    //* GRAYSCALE CONVERT
    if (roi_img.channels() == 3) {
        // image is color
        cv::cvtColor(roi_img, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = roi_img;
    }

    //* BLUR
    // kernel size is 1/4 of one millimiter. This is 4 times smaller
    // than the wanted global precision (1 mm) to avoid interference
    // with the measurements.
    int mblurKernelSize = (int)max(ceil(1.0 / 4 / mm_per_px), 3.0);
    if (mblurKernelSize % 2 == 0) {
        mblurKernelSize = mblurKernelSize - 1;
    }
    cv::medianBlur(gray, blur, mblurKernelSize);

    //? FOR DEBUG CONVENIENCE
    // log("mblurKernelSize", mblurKernelSize); // print value
    // cv::imshow("display",blur); cv::waitKey(0); // display image

    //* THRESHOLD
    double threshold_value = cv::threshold(blur, threshold_img, 0, 255,
                                           cv::THRESH_OTSU + cv::THRESH_BINARY);
    vector<cv::Vec3f> circles;
    cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT, 1, roi_width,
                     threshold_value, 1, (1.0 * pellet_diam_in_px) / 2 * 0.8,
                     (1.0 * pellet_diam_in_px) / 2 * 1.2);

    if (circles.empty()) {
        // no circle found
        throw astimp::Exception::generic("Pellet not found", __FILE__,
                                         __LINE__);
    }

    cv::Vec3f c = circles[0];
    return astimp::Circle(cv::Point2f(c[0] + left, c[1] + top), c[2]);
}

}  // namespace astimp