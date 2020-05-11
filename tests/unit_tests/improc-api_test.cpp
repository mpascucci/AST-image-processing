#include "improc-api.hpp"

#include <gtest/gtest.h>
#include <test_config.h>

#include <chrono>

using namespace astimp;
using namespace std::chrono;

TEST(findPetriDish, happyDay) {
    string path = test_img_path + string("test0.jpg");
    cv::Rect petriDish = findPetriDish(path);

    ASSERT_EQ(petriDish.x, 138);
    ASSERT_EQ(petriDish.y, 595);
    ASSERT_EQ(petriDish.height, 2599);
    ASSERT_EQ(petriDish.width, 2698);
}

TEST(findPellets, happyDay) {
    string path = test_img_path + string("test0_crop.jpg");
    PelletsAndPxPerMm pelletsAndPxPerMm = findPellets(path);
    ASSERT_NEAR(pelletsAndPxPerMm.original_img_px_per_mm, 22.3, 0.1);

    vector<Pellet> &pellets = pelletsAndPxPerMm.pellets;
    ASSERT_EQ(16, pellets.size());
    ASSERT_NEAR(2303, pellets[0].circle.center.x, 1);
    ASSERT_NEAR(2224, pellets[0].circle.center.y, 1);
    ASSERT_NEAR(67, pellets[0].circle.radius, 1);

    ASSERT_EQ("CAZ10",
              pellets[1].labelMatch.labelsAndConfidence.begin()->label);
    ASSERT_NEAR(0.98,
                pellets[1].labelMatch.labelsAndConfidence.begin()->confidence,
                0.02);

    // acceptable diameter difference is 0.5 mm
    ASSERT_NEAR(11, pellets[10].disk.diameter, 0.5);
    ASSERT_EQ(1, pellets[10].disk.confidence);
}

TEST(findPellet, knownPosition) {
    string path = test_img_path + string("test0.jpg");
    PelletsAndPxPerMm pelletsAndPxPerMm = findPellets(path);

    // the circle of the last found pellet
    Pellet lastPellet = pelletsAndPxPerMm.pellets.back();

    // all other circles (without the last one)
    vector<astimp::Circle> otherCircles;
    for (size_t i = 0; i < pelletsAndPxPerMm.pellets.size() - 1; i++) {
        otherCircles.push_back(pelletsAndPxPerMm.pellets[i].circle);
    }
    Pellet foundPellet = findPellet(lastPellet.circle, otherCircles, path);

    ASSERT_NEAR(lastPellet.disk.diameter, foundPellet.disk.diameter, 2);
}

TEST(findPellet, randomPosition) {
    string path = test_img_path + string("test0.jpg");
    Pellet pellet = findPellet({{200.0, 200.0}, 68.0}, {}, path);

    ASSERT_NEAR(200, pellet.circle.center.x, 0.1);
    ASSERT_NEAR(200, pellet.circle.center.y, 0.1);
    ASSERT_NEAR(68, pellet.circle.radius, 0.1);

    ASSERT_EQ("AUG30", pellet.labelMatch.labelsAndConfidence.begin()->label);
    ASSERT_NEAR(0.03, pellet.labelMatch.labelsAndConfidence.begin()->confidence,
                0.02);

    //! WARNING: Since findPellet() is given an empty otherCircles vector,
    //! the diameter measurement will probably be wrong. Don't test it!
}

TEST(findPelletFromApproxCoordinates, standard) {
    string path = test_img_path + string("test0.jpg");
    PelletsAndPxPerMm pelletsAndPxPerMm = findPellets(path);

    // all pellet circles
    size_t index_removed = 5;

    Pellet removedPellet = pelletsAndPxPerMm.pellets[index_removed];
    vector<astimp::Circle> otherCircles;

    for (size_t i = 0; i < pelletsAndPxPerMm.pellets.size(); i++) {
        if (i == index_removed) {
            continue;
        } else {
            otherCircles.push_back(pelletsAndPxPerMm.pellets[i].circle);
        }
    }

    // shift the pellet center to get approximative coordinates
    float cX = removedPellet.circle.center.x + 25;
    float cY = removedPellet.circle.center.y - 42;

    Pellet foundPellet = findPelletFromApproxCoordinates(
        cX, cY, otherCircles, path,
        1 / pelletsAndPxPerMm.original_img_px_per_mm);

    ASSERT_NEAR(removedPellet.circle.center.x, foundPellet.circle.center.x, 2);
    ASSERT_NEAR(removedPellet.circle.center.y, foundPellet.circle.center.y, 2);
    ASSERT_EQ(removedPellet.labelMatch.labelsAndConfidence.begin()->label,
              foundPellet.labelMatch.labelsAndConfidence.begin()->label);
    ASSERT_NEAR(removedPellet.disk.diameter, foundPellet.disk.diameter, 2);
}

TEST(findPellet, removedPellet) {
    string path = test_img_path + string("test0_crop.jpg");
    PelletsAndPxPerMm pelletsAndPxPerMm = findPellets(path);
    vector<Pellet> &pellets = pelletsAndPxPerMm.pellets;
    Pellet lastPellet = pellets.back();
    pellets.pop_back();
    vector<Circle> circles;
    for (Pellet pellet : pellets) {
        circles.push_back(pellet.circle);
    }
    Pellet pellet = findPellet(lastPellet.circle, circles, path);

    ASSERT_EQ("CFR30", pellet.labelMatch.labelsAndConfidence.begin()->label);
    ASSERT_NEAR(1, pellet.labelMatch.labelsAndConfidence.begin()->confidence,
                0.1);

    ASSERT_NEAR(6, pellet.disk.diameter, 0.1);
    ASSERT_EQ(1, pellet.disk.confidence);
}

TEST(getPelletDiamInMm, happyDay) { ASSERT_EQ(getPelletDiamInMm(), 6); }

TEST(findPellets, noticesWhenImageChanges) {
    PelletsAndPxPerMm pellets1 =
        findPellets(test_img_path + string("test0.jpg"));
    PelletsAndPxPerMm pellets2 =
        findPellets(test_img_path + string("test-antibio-full.jpg"));
    ASSERT_NE(pellets1.pellets, pellets2.pellets);

    PelletsAndPxPerMm pellets3 =
        findPellets(test_img_path + string("test0.jpg"));
    ASSERT_EQ(pellets1.pellets, pellets3.pellets);
}

TEST(findPellets, throwsWhenImageNotFound) {
    ASSERT_THROW(findPellets(test_img_path + string("foo.jpg")),
                 astimp::Exception::generic);
}

TEST(measureOnePelletDiameter, measuresDZoneDiameter) {
    string path = test_img_path + string("dzone.jpg");
    vector<Pellet> pellets = findPellets(path).pellets;

    vector<astimp::Circle> allCircles;
    for (Pellet pellet : pellets) {
        allCircles.push_back(pellet.circle);
    }
    bool clindaFound = false;
    for (size_t i = 0; i < pellets.size(); i++) {
        Pellet pellet = pellets[i];
        if (pellet.labelMatch.labelsAndConfidence.begin()->label != "CD2") {
            continue;
        }
        clindaFound = true;

        // Clindamycin has a D-shaped inhibition zone.
        // its INSCRIBED diameter should be smaller than its
        // CIRCUMSCRIBED diameter.
        InhibDisk circumscribedDisk =
            measureOnePelletDiameter(path, allCircles, i, CIRCUMSCRIBED);
        ASSERT_NEAR(27.4, circumscribedDisk.diameter, 0.5);

        InhibDisk inscribedDisk =
            measureOnePelletDiameter(path, allCircles, i, INSCRIBED);
        ASSERT_NEAR(19.6, inscribedDisk.diameter, 0.5);
    }
    ASSERT_TRUE(clindaFound);
}
