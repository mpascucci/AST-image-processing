#include <gtest/gtest.h>
#include <test_config.h>

#include "astimp.hpp"

TEST(findAtbPellets, onPhantom) {
    string path = test_img_path + string("antibiogram_phantom_all_25mm.png");
    cv::Mat img = cv::imread(path, cv::IMREAD_GRAYSCALE);
    ASSERT_FALSE(img.empty());
    ASSERT_EQ(astimp::find_atb_pellets(img).size(), 16);
}

TEST(findAtbPellets, excludesPelletsOutsideCrop) {
    string path = test_img_path + string("pellets_outside_bounds.png");
    cv::Mat img = cv::imread(path, cv::IMREAD_GRAYSCALE);
    ASSERT_FALSE(img.empty());

    // There are 16 total pellets in this image, but 12 of them are partially
    // falling off the edges of the image. Assert that those 12 are excluded,
    // and the remaining 4 are returned.
    ASSERT_EQ(astimp::find_atb_pellets(img).size(), 4);
}
