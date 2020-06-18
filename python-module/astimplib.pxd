# Copyright 2019 Fondation Medecins Sans Frontières https://fondation.msf.fr/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This file is part of the ASTapp image processing library
# Author: Marco Pascucci

from opencv_mat cimport *
from libcpp.vector cimport vector
from libcpp.set cimport set
from libcpp cimport bool
from libcpp.string cimport string
from cython.operator import dereference

cdef extern from "core/types.hpp" namespace "cv":
  cdef cppclass Rect:
    Rect() except +
    Rect(double x, double y, double width, double height) except +
    double width
    double height
    double x
    double y
  cdef cppclass Point2f:
    Point2f() except +
    Point2f(float x, float y)
    float x
    float y


cdef extern from "astimp.hpp" namespace "astimp":
    cdef cppclass PetriDish:
      Mat img
      Rect boundingBox
      bool isRound

    cdef cppclass Circle:
      Circle() except +
      # Circle(Point2f center, float radius) except +
      Point2f center
      float radius

    cdef cppclass InhibDiamPreprocResult:
      Mat img
      vector[Circle] circles
      vector[Rect] ROIs
      vector[int] km_centers
      int km_threshold
      vector[vector[int]] km_centers_local
      vector[int] km_thresholds_local
      float scale_factor
      int pad
      float px_per_mm
      InhibDiamPreprocResult() except +
      float original_img_px_per_mm
      
    cdef cppclass InhibDisk:
      float diameter
      float confidence

    cdef enum PROFILE_TYPE:
      PROFILE_MEAN
      PROFILE_MAX
      PROFILE_SWITCH
      PROFILE_MAXAVERAGE

    cdef enum MEDIUM_TYPE:
      MEDIUM_HM
      MEDIUM_BLOOD

    cdef enum InhibMeasureMode:
      INSCRIBED
      CIRCUMSCRIBED

    PetriDish getPetriDish(const Mat &img) except +
    PetriDish getPetriDishWithRoi(const Mat &ast_picture, const Rect roi) except +

    vector[Circle] find_atb_pellets(const Mat &img) except +
    Mat cutOnePelletInImage(const Mat &img, const Circle &circle, bool clone) except +
    float get_mm_per_px(const vector[Circle] &circles) except +
    vector[Rect] inhibition_disks_ROIs(const vector[Circle] &circles,
                                             int nrows, int ncols,
                                             float max_diam) except +

    InhibDiamPreprocResult inhib_diam_preprocessing(PetriDish petri, vector[Circle] &circles) except +
    vector[InhibDisk] measureDiameters(InhibDiamPreprocResult inhib_preproc) except +
    InhibDisk measureOneDiameter(InhibDiamPreprocResult inhib_preproc, int pellet_idx, InhibMeasureMode mode) except +
    ImprocConfig * getConfigWritable() except +
    vector[float] radial_profile(InhibDiamPreprocResult preproc, unsigned int num, PROFILE_TYPE pr_type, float th_value) except +
    void throw_custom_exception(const string &s) except +
    Circle searchOnePellet(const Mat &img, int center_x, int center_y, float mm_per_px)  except +
    float calcDiameterReadingSensibility(InhibDiamPreprocResult preproc, int pellet_idx) except +
    void calcDominantColor(const Mat &img, int* hsv) except +
    bool isGrowthMediumBlood(const Mat &ast_crop) except +
    
cdef extern from "pellet_label_recognition.hpp" namespace "astimp":
    cdef struct LabelAndConfidence:
      string label
      float confidence
    cdef cppclass Label_match:
      Label_match() except +
      set[LabelAndConfidence] labelsAndConfidence
    const Label_match getOnePelletText(const Mat &pelletImg) except +
    # const vector[Label_match] getPelletsText(const vector[Mat] &pelletImages)

cdef struct PetriDishSettings:
    int MaxSize 
    float gcBorder
    int gcIters
    int SideInMillimeters
    int DiameterInMillimeters
    int borderPelletDistance_in_mm
    MEDIUM_TYPE growthMedium

cdef struct PelletsSettings:
    float PelletIntensityPercent
    float MaxThinness
    float DiamInMillimeters
    int minPictureToPelletRatio

cdef struct InhibitionSettings:
    int maxInhibitionDiameter
    int preprocImg_px_per_mm
    int maxInhibToBacteriaIntensityDiff
    int minInhibToBacteriaIntensityDiff
    float diameterReadingSensibility
    int minPelletIntensity

cdef extern from "astimp.hpp" namespace "astimp":
  cdef cppclass ImprocConfig:
    PetriDishSettings PetriDish
    PelletsSettings Pellets
    InhibitionSettings Inhibition

