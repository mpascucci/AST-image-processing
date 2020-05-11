#include <gtest/gtest.h>
#include <test_config.h>

#include "astimp.hpp"

static cv::Mat read_test_image() {
    string path = test_img_path + string("antibiogram_phantom_all_25mm.png");
    return cv::imread(path, cv::IMREAD_GRAYSCALE);
}

TEST(cutPelletsInImage, cutsPelletsAccordingToCircle) {
    float radius1 = 100.0;
    float radius2 = 153.0;
    vector<astimp::Circle> circles = {
        astimp::Circle(cv::Point2f(200, 300), radius1),
        astimp::Circle(cv::Point2f(400, 1000), radius2)};
    cv::Mat testImage = read_test_image();
    ASSERT_FALSE(testImage.empty());

    vector<cv::Mat> pellets =
        astimp::cutPelletsInImage(read_test_image(), circles);

    float expected_pellet_size_1 = 2 * radius1 + 2;
    float expected_pellet_size_2 = 2 * radius2 + 2;
    ASSERT_EQ(pellets.at(0).rows, expected_pellet_size_1);
    ASSERT_EQ(pellets.at(0).cols, expected_pellet_size_1);
    ASSERT_EQ(pellets.at(1).rows, expected_pellet_size_2);
    ASSERT_EQ(pellets.at(1).cols, expected_pellet_size_2);
}

TEST(cutPelletsInImage, pelletOutsideImageBoundsShouldThrow) {
    // Since radius is 100 and center is (0,0), the pellet would be mostly
    // outside the bounds of the image, so cutPelletsInImage should throw.
    astimp::Circle circle_outside_bounds =
        astimp::Circle(cv::Point2f(0, 0), 100.0);
    vector<astimp::Circle> circles = {circle_outside_bounds};
    cv::Mat testImage = read_test_image();
    ASSERT_FALSE(testImage.empty());

    try {
        astimp::cutPelletsInImage(testImage, circles);
    } catch (astimp::Exception::generic const& e) {
        string expected_error_message =
            "[improc Exception] <An error while cutting the pellets in the "
            "image, check image crop (probably the dish is too close to the "
            "borders of the image";
        EXPECT_PRED_FORMAT2(testing::IsSubstring, expected_error_message,
                            e.what());
    }
}
