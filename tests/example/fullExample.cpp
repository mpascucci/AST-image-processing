// Copyright 2019 Fondation Medecins Sans Frontières https://fondation.msf.fr/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the ASTapp image processing library
// Author: Marco Pascucci <marpas.paris@gmail.com>

#include <assert.h>

#include <fstream>
#include <iostream>
#include <pellet_label_recognition.hpp>
#include <string>

#include "astimp.hpp"
#include "debug.hpp"
#include "pellet_label_recognition.hpp"
#include "test_utils.hpp"

using namespace std;

class Timer {
    // Measure execution time
   private:
    std::map<std::string, int64> starts;
    std::map<std::string, int64> ends;

   public:
    vector<string> actions;
    void start(string action) {
        // record start time for the specified action
        assert(starts.count(action) == 0);
        actions.push_back(action);
        starts.insert(std::make_pair(action, cv::getTickCount()));
    }
    void end(string action) {
        // record end time for the specified action
        assert(starts.count(action) == 1);
        ends.insert(std::make_pair(action, cv::getTickCount()));
    }

    float elapsed(string action) {
        // The elapsed time for the specified actions in seconds
        assert(ends.count(action) == 1);
        return ((float)ends[action] - starts[action]) / cv::getTickFrequency();
    }

    vector<string> report() {
        // Time report as list of strings
        vector<string> buffer;
        for (string action : actions) {
            buffer.push_back(action + ": " + to_string(elapsed(action)) + " s");
        }
        return buffer;
    }

    float total() {
        // Sum the time of all actions
        float tot = 0;
        for (string action : actions) {
            tot += elapsed(action);
        }
        return tot;
    }
};

int main(int argc, char **argv) {
    Timer T;

    assert(argc == 2);
    const string IMG_IN_PATH = argv[1];
    const string IMG_OUT_PATH = "./output.jpg";

    cv::Mat temp2;

    log("STARTING test program");
    log("Usign opencv version", CV_VERSION);
    log("Opencv random seed", cv::theRNG().state);

    // create display window
    cv::namedWindow("display", cv::WINDOW_NORMAL);
    cv::resizeWindow("display", 400, 600);

    // =============================================================================
    // LOAD IMAGE
    // =============================================================================
    cv::Mat org = cv::imread(IMG_IN_PATH, cv::IMREAD_COLOR);
    assert(!org.empty());
    // display(org);

    // =============================================================================
    // GET and SET CONFIGURATION PARAMETERS
    // =============================================================================
    // astimp::ImprocConfig * config = astimp::get_config();
    // config->PetriDish.gcBorder = 0.1; // set new value for a parameter
    // log("main gcBorder", config->PetriDish.gcBorder);

    // ===========================================================================
    // CROP PETRI DISH
    // ===========================================================================
    // cv::Rect r = astimp::findPetriDish(org);
    // display(addRectangleToImage(org,r));

    cv::Mat cropped;

    // e_start = cv::getTickCount();
    // e1 = cv::getTickCount();
    // cropped = astimp::cropPetriDish(org);
    // // log("bounds", astimp::locatePetriDish(org)); //BOUNDING BOX
    // e2 = cv::getTickCount();
    // log("TIME - cropPetriDish", (e2 - e1) / cv::getTickFrequency());
    // display(cropped);

    T.start("getPetriDish");
    astimp::PetriDish petri = astimp::getPetriDish(org);
    // Use the following line only for test image `test0.jpg`
    // astimp::PetriDish petri = astimp::getPetriDishWithRoi(org,
    // cv::Rect(100,500,2800,2800));
    T.end("getPetriDish");
    // display(petri.img);
    cropped = petri.img;

    T.start("is_growth_medium_blood");    
    bool is_blood = astimp::isGrowthMediumBlood(cropped);
    T.end("is_growth_medium_blood");   
    
    if (is_blood) {
        astimp::getConfigWritable()->PetriDish.growthMedium = astimp::MEDIUM_BLOOD;
        log("growth medium is blood enriched HM");
    } else {
        astimp::getConfigWritable()->PetriDish.growthMedium = astimp::MEDIUM_HM;
        log("growth medium is standard HM");
    }


    // ===========================================================================
    // FIND PELLETS
    // ===========================================================================
    // petri_shape = astimp::AST_PETRI_CIRCLE;
    T.start("find_atb_pellets");
    vector<astimp::Circle> circles = astimp::find_atb_pellets(cropped);
    T.end("find_atb_pellets");
    log("number of found pellets", circles.size());

    //* Display pellets
    // cv::Mat dbg(cropped);
    // for (auto c: circles) {
    //   cout << c.center << ' ' << c.radius;
    //   cv::circle(dbg, c.center, c.radius, cv::Scalar(0,0,255), 10);
    // }
    // display(dbg);

    T.start("cutPelletsInImage");
    auto pellets = astimp::cutPelletsInImage(cropped, circles);
    T.end("cutPelletsInImage");

    float mm_per_px = get_mm_per_px(circles);
    log("image scale (mm per pixel) ", mm_per_px);

    // ===========================================================================
    // SEARCH ONE PELLET
    // ===========================================================================
    // astimp::Circle c = astimp::searchOnePellet(cropped, 1050, 1600,
    // mm_per_px); log("cx", c.center.x); log("cy", c.center.y); log("r",
    // c.radius);

    // ===========================================================================
    // READ LABELS
    // ===========================================================================
    T.start("getPelletsText");
    auto matches = astimp::getPelletsText(pellets);
    T.end("getPelletsText");

    //* PRINT PELELTS TEXT (matches)
    // for (auto m: matches) {
    //   cout << "best matching label: " << m.text
    //        << " conf: " << m.confidence << endl;
    // }

    // ===========================================================================
    // MEASURE DIAMETERS
    // ===========================================================================
    //* PREPROCESSING
    cv::Mat temp;
    astimp::InhibDiamPreprocResult inhib;
    T.start("inhib_diam_preprocessing");
    inhib = inhib_diam_preprocessing(petri, circles);
    T.end("inhib_diam_preprocessing");

    //* Test by removing or adding one pellet
    // uint temp_n = 11;
    // astimp::Circle temp_circle = circles[temp_n];
    // circles.erase(circles.begin()+temp_n);
    // circles.push_back(temp_circle);
    // inhib = inhib_diam_preprocessing(cropped, circles);
    // for (uint i=0; i<circles.size(); i++) {
    //   cout << i << ": " << inhib.ROIs[i] << endl;
    // }

    // cv::destroyAllWindows();return(0);

    //* TEST SINGLE INHIBITION DIAMETER
    // uint pellet_idx = 5;
    // cropped.copyTo(temp2);
    // astimp::InhibDisk disk = astimp::measureOneDiameter(inhib, pellet_idx);
    // cv::circle(temp2, circles[pellet_idx].center,
    //            disk.diameter/2/mm_per_px, cv::Scalar(0,0,255), 5);
    // display(temp2);

    //* Measure Diameters
    T.start("measureDiameters");
    vector<astimp::InhibDisk> disks = astimp::measureDiameters(inhib);
    T.end("measureDiameters");

    //* DISPLAY DIAMETERS
    // for (auto d :disks) {
    //   cout << d.diameter << ", " << d.confidence << endl;
    // };
    // cout << endl;

    // PRINT EXECUTION TIME
    cout << endl << ">>> Execution time:" << endl;
    for (string s : T.report()) {
        cout << s << endl;
    }
    cout << "---" << endl << "total time: " << T.total() << " s" << endl;

    // ===========================================================================
    // DISPLAY RESULTS
    // ===========================================================================
    cropped.copyTo(temp2);
    char diam_s[128];
    float rp = circles[0].radius;
    int fs = (int)round(2 * rp * mm_per_px);  // font size
    float lw = (int)round(1.5 * rp * mm_per_px);
    float il = rp * 1.5;  // text interline
    for (size_t i = 0; i < disks.size(); i++) {
        //* pellet
        cv::circle(temp2, circles[i].center, disks[i].diameter / 2 / mm_per_px,
                   cv::Scalar(0, 0, 255), 5);

        //* number
        sprintf(diam_s, "#%d", (int)i);
        cv::putText(temp2, diam_s, circles[i].center + cv::Point2f(il, 0),
                    cv::FONT_HERSHEY_PLAIN, fs, cv::Scalar(0, 255, 0), lw);
        //* text
        cv::putText(temp2, matches[i].labelsAndConfidence.begin()->label,
                    circles[i].center + cv::Point2f(il, il),
                    cv::FONT_HERSHEY_PLAIN, fs, cv::Scalar(225, 255, 0), lw);
        //* diam
        sprintf(diam_s, "d=%d", (int)round(disks[i].diameter));
        cv::putText(temp2, diam_s, circles[i].center + cv::Point2f(il, 2 * il),
                    cv::FONT_HERSHEY_PLAIN, fs, cv::Scalar(255, 0, 255), lw);
    }

    display(temp2);

#ifdef USE_CVV
    cvv::finalShow();
#else
    cv::destroyAllWindows();
#endif  // USE_CVV

    return 0;
}