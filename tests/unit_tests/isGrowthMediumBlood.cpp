#include <gtest/gtest.h>
#include <test_config.h>

#include "astimp.hpp"

TEST(isGrowthMediumBlood, blood_0) {
    string path = test_img_path + string("test_blood_agar_0.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());
    img = astimp::getPetriDish(img).img;
    ASSERT_TRUE(astimp::isGrowthMediumBlood(img));
}


TEST(isGrowthMediumBlood, blood_1) {
    string path = test_img_path + string("test_blood_agar_1.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());
    img = astimp::getPetriDish(img).img;
    ASSERT_TRUE(astimp::isGrowthMediumBlood(img));
}

TEST(isGrowthMediumBlood, HM) {
    string path = test_img_path + string("test0.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());
    img = astimp::getPetriDish(img).img;
    ASSERT_FALSE(astimp::isGrowthMediumBlood(img));
}