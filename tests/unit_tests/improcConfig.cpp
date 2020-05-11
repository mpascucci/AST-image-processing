#include <gtest/gtest.h>
#include <test_config.h>

#include "astimp.hpp"

TEST(ImprocConfig, set_get_param) {
    /* the config object can be written and read correctly */

    const int x = 8;
    auto config = astimp::getConfigWritable();
    auto old_pellet_diameter =
        config->Pellets.DiamInMillimeters;  // store original value

    config->Pellets.DiamInMillimeters = x;  // assign new value

    ASSERT_EQ(x, config->Pellets.DiamInMillimeters);
    ASSERT_EQ(x, astimp::getConfig()->Pellets.DiamInMillimeters);

    config->Pellets.DiamInMillimeters =
        old_pellet_diameter;  // restore original value
}

TEST(ImprocConfig, measureDiameters) {
    /* Modifying the config object has an effect on improc functions that use
     it. Scale pellet diameter will scale inhibition diameters the same factor
   */

    string path = test_img_path + string("phantom_picture_25.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());

    float scale_factor = 2;
    auto config = astimp::getConfigWritable();
    auto old_pellet_diameter =
        config->Pellets.DiamInMillimeters;  // store old value

    config->Pellets.DiamInMillimeters =
        config->Pellets.DiamInMillimeters / scale_factor;  // change pellet diam

    astimp::PetriDish petri = astimp::getPetriDish(img);
    vector<astimp::Circle> circles = astimp::find_atb_pellets(petri.img);
    astimp::InhibDiamPreprocResult inhib =
        inhib_diam_preprocessing(petri, circles);
    vector<astimp::InhibDisk> disks = astimp::measureDiameters(inhib);

    for (auto disk : disks) {
        ASSERT_LE(round(disk.diameter - 25.0 / scale_factor), 1);
    }
    config->Pellets.DiamInMillimeters =
        old_pellet_diameter;  // restore old value
}

TEST(ImprocConfig, findAtbPelletsRespectsMaxRatio) {
    string path = test_img_path + string("big_pellet.png");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());

    auto config = astimp::getConfigWritable();
    // Pellets are smaller than picture_size / 2, so pellets are found when
    // maxPictureToPelletRatio = 2.
    config->Pellets.maxPictureToPelletRatio = 2;
    ASSERT_EQ(astimp::find_atb_pellets(img).size(), 2);

    // Pellets are larger than picture_size / 10, so pellets are found when
    // maxPictureToPelletRatio = 10.
    config->Pellets.maxPictureToPelletRatio = 10;
    ASSERT_EQ(astimp::find_atb_pellets(img).size(), 0);
}
