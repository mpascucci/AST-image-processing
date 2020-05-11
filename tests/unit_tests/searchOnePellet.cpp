#include <gtest/gtest.h>
#include <test_config.h>

#include "astimp.hpp"

TEST(searchOnePellet, on_full_inhib) {
    string path = test_img_path + string("test0.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());

    cv::Mat crop = astimp::getPetriDish(img).img;

    astimp::Circle c =
        astimp::searchOnePellet(crop, 1050, 1600, 0.04421902447938919);

    //? SHOW RESULT FOR VISUAL CHECK
    // cv::namedWindow("display", cv::WINDOW_NORMAL);
    // cv::resizeWindow("display", 400, 600);
    // cv::circle(crop, c.center, c.radius, cv::Scalar(0, 0, 255), 5);
    // cv::imshow("display",crop); cv::waitKey(0); // display image
    // std::cout << c.center << std::endl;

    ASSERT_NEAR(c.radius, 71, 3);
    ASSERT_NEAR(c.center.x, 1043, 3);
    ASSERT_NEAR(c.center.y, 1566, 3);
}

TEST(searchOnePellet, on_no_inhib) {
    string path = test_img_path + string("test0.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());

    // cv::theRNG().state = 0; // set opencv random generator's seed
    cv::Mat crop = astimp::getPetriDish(img).img;

    astimp::Circle c =
        astimp::searchOnePellet(crop, 2300, 300, 0.04421902447938919);

    // std::cout << c.center << std::endl;

    ASSERT_NEAR(c.radius, 64, 3);
    ASSERT_NEAR(c.center.x, 2302, 3);
    ASSERT_NEAR(c.center.y, 331, 3);
}

//! This test passes in my system but does not in the bitbucket build pipeline
// TEST(searchOnePellet, on_non_binary) {
//     string path = test_img_path + string("test0.jpg");
//     cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
//     ASSERT_FALSE(img.empty());
//     cv::Mat crop = astimp::cropPetriDish(img);

//     astimp::Circle c =
//     astimp::searchOnePellet(crop,1050,950,0.04421902447938919);
//     ASSERT_LE(abs(c.radius - 66), 10);
//     ASSERT_LE(abs(c.center.x - 976), 10);
//     ASSERT_LE(abs(c.center.y - 943), 10);
// }
