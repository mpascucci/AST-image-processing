#include <gtest/gtest.h>
#include <test_config.h>

#include "astimp.hpp"

TEST(calcDominantColor, blood_picture_0) {
    // picture of a blood-enriched antibiogram
    string path = test_img_path + string("test_blood_agar_0.jpg");
    cv::Mat img_blood = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img_blood.empty());
    img_blood = astimp::getPetriDish(img_blood).img;

    // picture of a standard antibiogram
    path = test_img_path + string("test0.jpg");
    cv::Mat img_standard = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img_standard.empty());
    img_standard = astimp::getPetriDish(img_standard).img;

    int hsv_blood[3];
    int hsv_standard[3];
    astimp::calcDominantColor(img_blood, hsv_blood);
    astimp::calcDominantColor(img_standard, hsv_standard);

    // hue is within 0 and 180
    ASSERT_GE(hsv_blood[0], 0);
    ASSERT_LT(hsv_blood[0], 180);

    // expected hue for a red image is close to zero (red) in a
    // shifted hue space, but not for standard images
    ASSERT_LT( abs((hsv_blood[0]+90)%180-90),  abs((hsv_standard[0]+90)%180-90));

    // mean value and saturation are encoded in 8bit unsigned int
    ASSERT_GE(hsv_blood[1], 0); 
    ASSERT_GE(hsv_blood[1], 0);
    ASSERT_LT(hsv_blood[2], UCHAR_MAX); 
    ASSERT_LT(hsv_blood[2], UCHAR_MAX); 

}



TEST(isGrowthMediumBlood, blood_agar_picture_0) {
    string path = test_img_path + string("test_blood_agar_0.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());
    img = astimp::getPetriDish(img).img;
    ASSERT_TRUE(astimp::isGrowthMediumBlood(img));
}


TEST(isGrowthMediumBlood, blood_agar_picture_1) {
    string path = test_img_path + string("test_blood_agar_1.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());
    img = astimp::getPetriDish(img).img;
    ASSERT_TRUE(astimp::isGrowthMediumBlood(img));
}

TEST(isGrowthMediumBlood, standard_MH_agar_picture) {
    // MH stands for Mueller-Hinton, is the technical name of the
    // standard growth medium used in disk diffusion AST.
    string path = test_img_path + string("test0.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());
    img = astimp::getPetriDish(img).img;
    ASSERT_FALSE(astimp::isGrowthMediumBlood(img));
}