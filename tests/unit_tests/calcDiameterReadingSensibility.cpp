#include <gtest/gtest.h>

#include "astimp.hpp"

using namespace astimp;

TEST(calcDiameterReadingSensibility, standard) {
    astimp::InhibDiamPreprocResult preproc;
    cv::Mat img = cv::Mat();
    vector<Circle> circles = {};
    vector<cv::Rect> ROIs = {};
    float scale_factor = 1;
    int pad = 0;
    float px_per_mm = 0;
    float original_img_px_per_mm = 0;
    int km_threshold = 0;
    vector<int> km_thresholds_local = {};

    vector<int> km_centers = {};
    vector<vector<int>> km_centers_local = {};

    preproc = InhibDiamPreprocResult(img, circles, ROIs, km_centers,
                                     km_threshold, km_centers_local,
                                     km_thresholds_local, scale_factor, pad,
                                     px_per_mm, original_img_px_per_mm);

    float drs;

    // get minContrast from config
    int minContrast =
        astimp::getConfig()->Inhibition.minInhibToBacteriaIntensityDiff;

    /**********************************
     * CONTACT
     ***********************************/
    int inhib = 50;
    int bact = inhib + minContrast;

    preproc.km_centers = {{inhib, bact}};
    preproc.km_centers_local = {{bact - 1, bact}};
    drs = calcDiameterReadingSensibility(preproc, 0);
    ASSERT_FLOAT_EQ(drs, 0.7);

    /**********************************
     * local contrast <= minContrast
     ***********************************/
    inhib = 50;
    bact = inhib + minContrast;

    preproc.km_centers = {{inhib, bact}};
    preproc.km_centers_local = {{inhib, bact}};
    drs = calcDiameterReadingSensibility(preproc, 0);
    ASSERT_FLOAT_EQ(drs, 0.05);

    /**********************************
     * local contrast > minContrast
     ***********************************/

    inhib = 50;
    bact = inhib + 2 * minContrast;
    // contrast : local == global
    preproc.km_centers = {{inhib, bact}};
    preproc.km_centers_local = {{inhib, bact}};
    drs = calcDiameterReadingSensibility(preproc, 0);
    ASSERT_FLOAT_EQ(drs, 0.5);

    // contrast : local > global
    preproc.km_centers = {{inhib, bact}};
    preproc.km_centers_local = {{inhib, bact + 10}};
    drs = calcDiameterReadingSensibility(preproc, 0);
    ASSERT_LT(drs, 0.5);

    // contrast : local < global
    preproc.km_centers = {{inhib, bact}};
    preproc.km_centers_local = {{inhib, bact - minContrast / 2}};
    ;
    drs = calcDiameterReadingSensibility(preproc, 0);
    ASSERT_GT(drs, 0.5);
}