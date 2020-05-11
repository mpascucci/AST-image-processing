#include <gtest/gtest.h>
#include <test_config.h>

#include "astimp.hpp"

TEST(measureDiameters, onPhantom) {
    string path = test_img_path + string("antibiogram_phantom_all_25mm.png");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());

    astimp::PetriDish petri = astimp::getPetriDish(img);
    vector<astimp::Circle> circles = astimp::find_atb_pellets(petri.img);
    astimp::InhibDiamPreprocResult inhib =
        inhib_diam_preprocessing(petri, circles);
    vector<astimp::InhibDisk> disks = astimp::measureDiameters(inhib);

    for (auto disk : disks) {
        ASSERT_NEAR(disk.diameter, 25, 0.5);
    }
}

TEST(measureDiameters, onPhantomPicture25) {
    // use picture of printed AST where all diameters are 25mm
    string path = test_img_path + string("phantom_picture_25.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());

    astimp::PetriDish petri = astimp::getPetriDish(img);

    vector<astimp::Circle> circles = astimp::find_atb_pellets(petri.img);
    astimp::InhibDiamPreprocResult inhib =
        inhib_diam_preprocessing(petri, circles);
    vector<astimp::InhibDisk> disks = astimp::measureDiameters(inhib);

    float mean_diameter = 0;
    for (auto inhib : disks) {
        mean_diameter += inhib.diameter;
    }

    mean_diameter /= disks.size();
    ASSERT_FLOAT_EQ(round(mean_diameter), 25);
}

TEST(measureDiameters, onPhantomPicture_increasing) {
    // use picture of printed AST where diameters increase from 10 to 25mm
    string path = test_img_path + string("phantom_picture_increasing.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    ASSERT_FALSE(img.empty());

    astimp::PetriDish petri = astimp::getPetriDish(img);

    vector<astimp::Circle> circles = astimp::find_atb_pellets(petri.img);
    astimp::InhibDiamPreprocResult inhib =
        inhib_diam_preprocessing(petri, circles);
    vector<astimp::InhibDisk> disks = astimp::measureDiameters(inhib);

    int true_diameters[] = {24, 25, 23, 22, 21, 20, 19, 18,
                            17, 16, 15, 14, 13, 12, 11, 10};

    for (size_t i = 0; i < disks.size(); i++) {
        ASSERT_EQ(round(disks[i].diameter), true_diameters[i]);
    }
}