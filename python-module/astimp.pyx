# distutils: language=c++

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

from libcpp.vector cimport vector
from opencv_mat cimport *
from opencv_mat import PyMat
cimport astimplib
from libcpp.string cimport string
from collections import namedtuple
from cython.operator import dereference

from astimp_tools.datamodels import AST, Antibiotic
# from astimplib.image import subimage_by_roi

# cdef pyobj2preproc( namedtuple):
#     cdef astimplib.InhibDiamPreprocResult preproc
#     preproc = astimplib.InhibDiamPreprocResult()
#     preproc.img = np2Mat

#     return InhibDiamPreprocResult(
#         Mat2np(idpr.img),
#         [PyCircle(c.center.x, c.center.y, c.radius) for c in idpr.circles],
#         [PyRect(r.x, r.y, r.height, r.width) for r in idpr.ROIs],
#         idpr.km_centers,
#         idpr.km_threshold,
#         idpr.scale_factor,
#         idpr.pad,
#         idpr.px_per_mm,
#     )



 ######################
 ## ENUMS
 ######################

class MEDIUM_TYPE:
    HM = astimplib.MEDIUM_HM
    BLOOD = astimplib.MEDIUM_BLOOD

class PROFILE_ALGO:
    MEAN = astimplib.PROFILE_MEAN
    MAX  = astimplib.PROFILE_MAX
    MAXAVERAGE = astimplib.PROFILE_MAXAVERAGE
    SWITCH = astimplib.PROFILE_SWITCH

class INHIB_MEASURE_MODE:
    INSCRIBED = astimplib.INSCRIBED
    CIRCUMSCRIBED = astimplib.CIRCUMSCRIBED

 ######################
 ## astimp wrappers
 ######################
    
def find_atb_pellets(nparray):
    cdef vector[astimplib.Circle] v
    cdef Mat m
    cdef astimplib.Circle c
    m = np2Mat(nparray)
    v = astimplib.find_atb_pellets(m)
    out = [Circle((c.center.x, c.center.y), c.radius) for c in v]
    return out

def cutOnePelletInImage(nparray, circle):
    cdef Mat m = np2Mat(nparray)
    cdef astimplib.Circle c = py2circle(circle)
    return Mat2np(astimplib.cutOnePelletInImage(m, c, clone=True))

def identity(nparray):
    cdef Mat m = np2Mat(nparray)
    return Mat2np(m)

def get_mm_per_px(circles):
    cdef vector[astimplib.Circle] ccircles

    for pc in circles:
        ccircles.push_back(py2circle(pc))

    return astimplib.get_mm_per_px(ccircles)

def getOnePelletText(nparray):
    cdef Mat m = np2Mat(nparray)
    cdef astimplib.Label_match lm
    lm = astimplib.getOnePelletText(m)
    top_label_and_confidence = dereference(lm.labelsAndConfidence.begin())
    return Pellet_match(top_label_and_confidence.label.decode('UTF-8'),
        top_label_and_confidence.confidence)

def inhib_diam_preprocessing(petri_dish, circles):
    cdef astimplib.PetriDish petri_c = petriDish_to_c(petri_dish)
    cdef vector[astimplib.Circle] c_circles
    cdef vector[astimplib.InhibDisk] inhib 
    cdef astimplib.InhibDiamPreprocResult preproc
    for pc in circles:
        c_circles.push_back(py2circle(pc))
    preproc = astimplib.inhib_diam_preprocessing(petri_c,c_circles)
    return preproc2pyobj(preproc)

def measureDiameters(preproc):
    """measures the diameter of all the inhibition zones (with the default method)"""
    inhib = astimplib.measureDiameters(pyobj2prepoc(preproc))
    return [InhibDisk(x.diameter,x.confidence) for x in inhib]

def measureOneDiameter(preproc, int pellet_idx, mode=INHIB_MEASURE_MODE.INSCRIBED):
    """Measure the diameter of one zone of inhibition, the method can be specified.
    Use:
    - astimp.INHIB_MEASURE_MODE.INSCRIBED in order to measure the diameter of the
    largest circle centered on the antibiotic disk center and not including any bacteria

    - astimp.INHIB_MEASURE_MODE.CIRCUMSCRIBED in order to measure the diameter of 
    the circle that circumscribes the zone of inhibition (used for D-Zones).
    """
    x = astimplib.measureOneDiameter(pyobj2prepoc(preproc), pellet_idx, mode)
    return InhibDisk(x.diameter,x.confidence)

def radial_profile(preproc, num, profile_type, th_value=0):
    return astimplib.radial_profile(pyobj2prepoc(preproc), num, profile_type, th_value)

def calcDiameterReadingSensibility(preproc, pellet_idx):
    return astimplib.calcDiameterReadingSensibility(pyobj2prepoc(preproc), pellet_idx)

# def test(preproc):
#     return Mat2np(pyobj2prepoc(preproc).img)

def throw_custom_exception(message):
    cdef string s = message.encode('UTF-8')
    astimplib.throw_custom_exception(s)

def searchOnePellet(nparray, center_x, center_y, mm_per_px):
    """search one pellet in image npadday, around point center.
    center is a list of 2 int [x,y].
    """
    cdef int cx = round(center_x)
    cdef int cy = round(center_y)
    cdef Mat img = np2Mat(nparray)
    cdef float mm_per_px_c = mm_per_px
    cdef astimplib.Circle circ = astimplib.searchOnePellet(img, center_x, center_y, mm_per_px_c)
    return Circle((circ.center.x, circ.center.y), circ.radius)

def inhibition_disks_ROIs(circles, nparray, max_diam):
    """get the region of interest for each circle"""
    cdef vector[astimplib.Circle] c_circles
    for pc in circles:
        c_circles.push_back(py2circle(pc))

    nrows = nparray.shape[0]
    ncols = nparray.shape[1]

    rois = astimplib.inhibition_disks_ROIs(c_circles,nrows,ncols, max_diam)
    p_rois = []
    for roi in rois:
        p_rois.append(Roi(roi.x,roi.y,roi.width,roi.height))
    return p_rois

def getPetriDish(nparray):
    cdef astimplib.PetriDish pd = astimplib.getPetriDish(
        astimplib.np2Mat(nparray)
    )
    
    return PetriDish(
        img=astimplib.Mat2np(pd.img),
        boundingBox=Roi(pd.boundingBox.x,
            pd.boundingBox.y,
            pd.boundingBox.width,
            pd.boundingBox.height
            ),
        isRound=pd.isRound
    )

def getPetriDishWithRoi(nparray, roi):
    """roi is a rectangle (x,y,width,height)"""
    cdef astimplib.PetriDish pd = astimplib.getPetriDishWithRoi(
        astimplib.np2Mat(nparray), astimplib.Rect(roi[0],roi[1],roi[2],roi[3])
    )
    
    return PetriDish(
        img=astimplib.Mat2np(pd.img),
        boundingBox=Roi(pd.boundingBox.x,
            pd.boundingBox.y,
            pd.boundingBox.width,
            pd.boundingBox.height
            ),
        isRound=pd.isRound
    )

def calc_dominant_color(nparray):
    cdef int hsv[3]
    astimplib.calcDominantColor(astimplib.np2Mat(nparray),hsv)
    return (hsv[0],hsv[1],hsv[2])


def is_growth_medium_blood(nparray):
    return astimplib.isGrowthMediumBlood(astimplib.np2Mat(nparray))


# --------------------------------- CLASSES --------------------------------

class Roi():
  def __init__(self,x,y,width,height):
    self.x = x
    self.y = y
    self.height = height
    self.width = width
    self.abs_sub_ROIs = [self]
  @property
  def left(self):
    return self.x
  @property
  def top(self):
    return self.y
  @property
  def right(self):
    return self.x + self.width
  @property
  def bottom(self):
    return self.y + self.height
  @property
  def center(self):
    return (self.x + (self.right - self.left)/2,
            self.y + (self.bottom - self.top)/2)
  @property
  def sub_ROIs(self):
    relative = []
    for roi in self.abs_sub_ROIs:
        new_roi = roi.clone()
        new_roi.x -= self.left
        new_roi.y -= self.top
        relative.append(new_roi)
    return relative

  def clone(self):
    return Roi(
        self.x,
        self.y,
        self.width,
        self.height
    )

  def __repr__(self):
    return "(x={},y={}), w={}, h={}".format(
        self.x, self.y, self.width, self.height
    )

  def __eq__(self, other):
    return (self.x == other.x
            and self.y == other.y
            and self.width == other.width
            and self.height == other.height)

  def __add__(self, other):
    left = min(self.left, other.left)
    top = min(self.top, other.top)
    right = max(self.right, other.right)
    bottom = max(self.bottom, other.bottom)

    r = Roi(left,top,right-left,bottom-top)

    r.abs_sub_ROIs = self.abs_sub_ROIs

    for roi in self.abs_sub_ROIs:
        if other == roi:
            break
    else:
        r.abs_sub_ROIs += [other]
    
    return r


class Pellet_match:
    def __init__(self,text,confidence):
        self.text = text
        self.confidence = confidence

class InhibDisk:
    def __init__(self, diameter, confidence):
        self.diameter = diameter
        self.confidence = confidence

class Circle:
    def __init__(self,center,radius):
        self.center = center
        self.radius = radius

class PetriDish:
    def __init__(self, img, boundingBox, isRound):
      self.img = img
      self.boundingBox = boundingBox
      self.isRound = isRound

cdef astimplib.PetriDish petriDish_to_c(petri):
    cdef astimplib.PetriDish petri_c
    petri_c.img = astimplib.np2Mat(petri.img)
    bb = petri.boundingBox
    petri_c.boundingBox = astimplib.Rect(bb.x,bb.y,bb.width,bb.height)
    petri_c.isRound = petri.isRound
    return petri_c

cdef astimplib.Circle py2circle(pc):
    cdef astimplib.Circle c_circle = astimplib.Circle()
    c_circle.center = astimplib.Point2f(pc.center[0], pc.center[1])
    c_circle.radius = pc.radius
    return c_circle

class Rect:
    def __init__(self,x,y,width,height):
        self.x = x
        self.y = y
        self.height = height
        self.width = width

class PyPreproc:
    def __init__(self,img,circles,ROIs,
    km_centers,km_threshold,
    km_centers_local,km_thresholds_local,
    scale_factor,pad,px_per_mm,original_img_px_per_mm):
        self.img = img.copy()
        self.circles = circles
        self.ROIs = ROIs
        self.km_centers = km_centers
        self.km_threshold = km_threshold
        self.km_centers_local = km_centers_local
        self.km_thresholds_local = km_thresholds_local
        self.scale_factor = scale_factor
        self.pad = pad
        self.px_per_mm = px_per_mm
        self.original_img_px_per_mm = original_img_px_per_mm

cdef preproc2pyobj( astimplib.InhibDiamPreprocResult idpr):
    # transform a C object InhibDiamPreprocResult into python namedtuple
    return PyPreproc(
        Mat2np(idpr.img),
        [Circle((c.center.x, c.center.y), c.radius) for c in idpr.circles],
        [Roi(r.x, r.y, r.height, r.width) for r in idpr.ROIs],
        idpr.km_centers,
        idpr.km_threshold,
        idpr.km_centers_local,
        idpr.km_thresholds_local,
        idpr.scale_factor,
        idpr.pad,
        idpr.px_per_mm,
        idpr.original_img_px_per_mm,
    )

cdef astimplib.InhibDiamPreprocResult pyobj2prepoc(idpr):
    cdef astimplib.InhibDiamPreprocResult out
    cdef vector[astimplib.Circle] c_circles
    # cdef astimplib.Circle c_circle
    cdef vector[astimplib.Rect] c_ROIs
    
    for pc in idpr.circles:
        c_circles.push_back(py2circle(pc))

    for roi in idpr.ROIs:
        c_ROIs.push_back(astimplib.Rect(roi.x, roi.y, roi.width, roi.height))

    cdef astimplib.InhibDiamPreprocResult result = astimplib.InhibDiamPreprocResult()
    result.img = np2Mat(idpr.img)
    result.circles = c_circles
    result.ROIs = c_ROIs
    result.km_centers = idpr.km_centers
    result.km_threshold=idpr.km_threshold
    result.km_centers_local = idpr.km_centers_local
    result.km_thresholds_local=idpr.km_thresholds_local
    result.scale_factor=idpr.scale_factor
    result.pad=idpr.pad
    result.px_per_mm=idpr.px_per_mm
    result.original_img_px_per_mm=idpr.original_img_px_per_mm

    return result

 ######################
 ## CONFIG
 ######################

cdef class ImprocConfig:
    cdef astimplib.ImprocConfig *config

    def __cinit__(self):
        self.config = astimplib.getConfigWritable()

    @property
    def PetriDish_gcBorder(self):
        return self.config[0].PetriDish.gcBorder
    @PetriDish_gcBorder.setter
    def PetriDish_gcBorder(self,d):
        self.config[0].PetriDish.gcBorder = d

    @property
    def PetriDish_borderPelletDistance_in_mm(self):
        return self.config[0].PetriDish.borderPelletDistance_in_mm
    @PetriDish_borderPelletDistance_in_mm.setter
    def PetriDish_borderPelletDistance_in_mm(self, value):
        self.config[0].PetriDish.borderPelletDistance_in_mm = value

    @property
    def PetriDish_growthMedium(self):
        return self.config[0].PetriDish.growthMedium
    @PetriDish_growthMedium.setter
    def PetriDish_growthMedium(self, value):
        # assert type(value) == bool
        self.config[0].PetriDish.growthMedium = value

    @property
    def PetriDish_MaxThinness(self):
        return self.config[0].Pellets.MaxThinness
    @PetriDish_MaxThinness.setter
    def PetriDish_MaxThinness(self,d):
        self.config[0].Pellets.MaxThinness = d

    @property
    def PetriDish_PelletIntensityPercent(self):
        return self.config[0].Pellets.PelletIntensityPercent
    @PetriDish_PelletIntensityPercent.setter
    def PetriDish_PelletIntensityPercent(self,d):
        self.config[0].Pellets.PelletIntensityPercent = d
    
    @property
    def Pellets_DiamInMillimeters(self):
        return self.config[0].Pellets.DiamInMillimeters
    @Pellets_DiamInMillimeters.setter
    def Pellets_DiamInMillimeters(self,d):
        self.config[0].Pellets.DiamInMillimeters = d

    @property
    def Inhibition_preprocImg_px_per_mm(self):
        return self.config[0].Inhibition.preprocImg_px_per_mm
    @Inhibition_preprocImg_px_per_mm.setter
    def Inhibition_preprocImg_px_per_mm(self,resize:bool):
        self.config[0].Inhibition.preprocImg_px_per_mm = resize



    @property
    def Inhibition_maxInhibToBacteriaIntensityDiff(self):
        return self.config[0].Inhibition.maxInhibToBacteriaIntensityDiff
    @Inhibition_maxInhibToBacteriaIntensityDiff.setter
    def Inhibition_maxInhibToBacteriaIntensityDiff(self,d:int):
        self.config[0].Inhibition.maxInhibToBacteriaIntensityDiff = d

    @property
    def Inhibition_minInhibToBacteriaIntensityDiff(self):
        return self.config[0].Inhibition.minInhibToBacteriaIntensityDiff
    @Inhibition_minInhibToBacteriaIntensityDiff.setter
    def Inhibition_minInhibToBacteriaIntensityDiff(self,d:int):
        self.config[0].Inhibition.minInhibToBacteriaIntensityDiff = d

    @property
    def Inhibition_diameterReadingSensibility(self):
        return self.config[0].Inhibition.diameterReadingSensibility
    @Inhibition_diameterReadingSensibility.setter
    def Inhibition_diameterReadingSensibility(self,d:float):
        if (d<0) or (d>1):
            raise  ValueError("sensibility value must be in [0,1].")
        self.config[0].Inhibition.diameterReadingSensibility = d

    @property
    def Inhibition_minPelletIntensity(self):
        return self.config[0].Inhibition.minPelletIntensity
    @Inhibition_minPelletIntensity.setter
    def Inhibition_minPelletIntensity(self,d:int):
        if (0<0) or (d>255):
            raise  ValueError("sensibility value must be in [0,255].")
        self.config[0].Inhibition.minPelletIntensity = d
    

config = ImprocConfig()