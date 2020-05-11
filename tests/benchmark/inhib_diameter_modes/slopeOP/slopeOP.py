import astimp
import slopeOP
import numpy as np
from ..shared import no_inhibition, total_inhibition, varianceHall, varianceHallDiff

def measureOneDiameter(preproc, pellet_idx, penalty=None, constraint='null'):
    diameter, cp, cpts, offset, slopeOPdata, penalty= _measureOneDiameter(preproc, pellet_idx, penalty, constraint)
    return astimp.InhibDisk(diameter=diameter, confidence=0)

def measureOneDiameter_isotonic(preproc, pellet_idx, penalty=None):
    y = np.array(astimp.radial_profile(
        preproc, pellet_idx, astimp.profile_type["maxaverage"]))
    px_per_mm = preproc.px_per_mm
    tolerance = int(np.round(px_per_mm))
    pellet_radius_in_px = int(np.round(px_per_mm)*3)
    offset = pellet_radius_in_px+tolerance

    slopeOPdata = []
    cpts = []
    if no_inhibition(y[offset:], preproc, pellet_idx):
        cp = len(y)
    elif total_inhibition(y[offset:], preproc, pellet_idx):
        cp = pellet_radius_in_px
    else:
        reduce_factor = 4
        slopeOPdata = np.array(np.round(y[offset:]/reduce_factor), dtype=int)
        if penalty is None:
            temp_max = np.max(slopeOPdata)
            y_temp = slopeOPdata
            sigma = np.sqrt(varianceHallDiff(y_temp))
            penalty = sigma*temp_max*np.log(len(y_temp)) * 2

        states = sorted(list(set(slopeOPdata)))
        omega = slopeOP.slopeOP(slopeOPdata, states,
                                penalty, constraint="isotonic")
        cpts = np.array(omega.GetChangepoints()) + offset
        cpts_filtered = cpts[cpts > offset]

        cp = np.mean(cpts_filtered[0:2])

    diameter = (cp)/preproc.px_per_mm*2

    return diameter, cp, cpts, offset, slopeOPdata, penalty


def measureOneDiameter_unimodal(preproc, pellet_idx, penalty=None):
    # radial profile data
    y = np.array(astimp.radial_profile(
        preproc, pellet_idx, astimp.profile_type["max"]))

    # image scale
    px_per_mm = preproc.px_per_mm
    tolerance = int(np.round(px_per_mm))
    pellet_radius_in_px = int(np.round(px_per_mm)*3)
    offset = pellet_radius_in_px+tolerance
    slopeOPdata = []
    cpts = []
    reduce_factor = None
    if False: # no_inhibition(y[offset:], preproc, pellet_idx):
        cp = len(y)
    elif False: #total_inhibition(y[offset:], preproc, pellet_idx):
        cp = pellet_radius_in_px
    else:
        reduce_factor = 4
        
        if penalty is None:
            # penalty estimation
            y_temp = np.array(np.round(y[offset:]/reduce_factor), dtype=int)
            temp_max = np.max(y_temp)
            y_temp = y_temp
            sigma = np.sqrt(varianceHallDiff(y_temp))
            penalty = 2*sigma*np.log(len(y_temp)) * temp_max

        # data prepared for slopeOP
        slopeOPdata = np.array(-np.round(y/reduce_factor), dtype=int)
        # finite states
        states = sorted(list(set(slopeOPdata)))
        # run slopeOP with unimodal constraint
        omega = slopeOP.slopeOP(slopeOPdata, states,
                                penalty, constraint="unimodal")
        cpts = np.array(omega.GetChangepoints())
        cpts_filtered = cpts[cpts > offset]
        # assert len(cpts_filtered) > 1, "found {} cpts < 1".format(len(cpts))
        cp = np.mean(cpts_filtered[0:2])

    diameter = (cp)/preproc.px_per_mm*2

    return diameter, cp, cpts, offset, slopeOPdata, penalty, omega, reduce_factor


def _measureOneDiameter(preproc, pellet_idx, penalty=None, constraint='null'):
    if constraint not in ["unimodal", "isotonic", 'null']:
        raise(AttributeError("constraint must be either isotonic or unimodal"))

    if constraint == "unimodal":
        result = measureOneDiameter_unimodal(preproc, pellet_idx, penalty)
    elif constraint == "isotonic":
        result = measureOneDiameter_isotonic(preproc, pellet_idx, penalty)
    else:
        raise ValueError("a constraint must be chosen")

    return result
