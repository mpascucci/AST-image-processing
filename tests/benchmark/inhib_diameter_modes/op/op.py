import astimp
import optimalpartitioning
import numpy as np
from ..shared import no_inhibition, total_inhibition, varianceHall, varianceHallDiff

def measureOneDiameter(preproc, pellet_idx, dimensions, penalty=None):
    diameter, cp, opresult = _measureOneDiameter(preproc, pellet_idx, dimensions, penalty)
    return astimp.InhibDisk(diameter=diameter, confidence=0)

def _measureOneDiameter(preproc, pellet_idx, dimensions, penalty=None):
    y = np.array(astimp.radial_profile(
        preproc, pellet_idx, astimp.profile_type["maxaverage"]))
    px_per_mm = preproc.px_per_mm
    tolerance = int(np.round(px_per_mm))
    pellet_radius_in_px = int(np.round(px_per_mm)*3)
    offset = pellet_radius_in_px+tolerance

    opresult = None

    if no_inhibition(y[offset:], preproc, pellet_idx):
        cp = len(y)
    elif total_inhibition(y[offset:], preproc, pellet_idx):
        cp = pellet_radius_in_px
    else:
        op_data = y[offset:]
        if penalty is None:
            temp_max = np.max(op_data)
            y_temp = op_data/temp_max
            sigma = np.sqrt(varianceHallDiff(y_temp))
            penalty = sigma*temp_max*np.log(len(y_temp)) * 2*100

        if dimensions == 1:
            f = optimalpartitioning.op1D
        if dimensions == 2:
            f = optimalpartitioning.op2D

        opresult = f(list(range(len(op_data))), op_data, penalty)
        opresult.cp = [cp + offset for cp in opresult.cp]

        if len(opresult.cp) > 0:
            # cp = opresult.cp[0]
            cp = np.mean(opresult.cp)
        else:
            cp = len(y)

    diameter = cp/preproc.px_per_mm*2

    return diameter, cp, opresult
