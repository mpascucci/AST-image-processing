#include "pellet_label_recognition.hpp"

#include <gtest/gtest.h>
#include <test_config.h>

#include "astimp.hpp"

using namespace astimp;

static const string jpgs[4] = {"pellet_i2a_AK30.jpg", "pellet_i2a_CIP5.jpg",
                               "pellet_i2a_L15.jpg", "pellet_i2a_CIP5_1.jpg"};
static const string labels[4] = {"AK30", "CIP5", "L15", "CIP5"};

TEST(PelletLabelRecognitionMLTest, getOnePelletText) {
    for (size_t i = 0; i < 4; i++) {
        string path = test_img_path + string(jpgs[i]);
        cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
        if (img.empty()) {
            FAIL() << "test image not found: " << path;
        }
        Label_match m = getOnePelletText(img);
        ASSERT_EQ(labels[i], m.labelsAndConfidence.begin()->label);
    }
}

TEST(PelletLabelRecognitionMLTest, getPelletsText) {
    vector<cv::Mat> images;
    for (const auto &jpg : jpgs) {
        string path = test_img_path + jpg;
        cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
        if (img.empty()) {
            FAIL() << "test image not found: " << path;
        }
        images.emplace_back(img);
    }
    vector<Label_match> matches = getPelletsText(images);
    for (size_t i = 0; i < 4; i++) {
        ASSERT_EQ(labels[i], matches[i].labelsAndConfidence.begin()->label);
    }
}

TEST(PelletLabelRecognitionMLTest, getPelletModelInputs) {
    vector<cv::Mat> images;
    for (const auto &jpg : jpgs) {
        string path = test_img_path + jpg;
        cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
        if (img.empty()) {
            FAIL() << "test image not found: " << path;
        }
        images.emplace_back(img);
    }
    vector<vector<float>> modelInputs =
        PelletLabelRecognitionUsingML::getPelletModelInputs(images);
    // Test a few values in the model inputs.
    EXPECT_NEAR(modelInputs[0][0], -1.54896, 0.000001);
    EXPECT_NEAR(modelInputs[1][100], 0.442385, 0.000001);
    EXPECT_NEAR(modelInputs[2][64 * 64 - 1], 34, 0.000001);
}
