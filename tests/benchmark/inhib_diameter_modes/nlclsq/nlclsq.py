import numpy as np
from scipy import optimize
import astimp

# from logging import getLogger
# log = getLogger(__name__)
# log.warning("regression module reloaded.")


def measureOneDiameter(preproc, pellet_idx):
    diameter, idx, fit_cost, (left, right), fit, th_v = _measureOneDiameter(preproc, pellet_idx)
    return astimp.InhibDisk(diameter=diameter, confidence=fit_cost)

def _measureOneDiameter(preproc, pellet_idx):
    """find the inhibition diameter via non linear constrained least_square."""
    px_per_mm = preproc.px_per_mm
    pellet_radius_in_px = int(np.round(px_per_mm)*3)
    tolerance = int(np.round(px_per_mm))
    y = np.array(astimp.radial_profile(
        preproc, pellet_idx, astimp.profile_type["maxaverage"]))

    kmc = preproc.km_centers
    th_v = preproc.km_thresholds_local[pellet_idx]

    # NOTE: the following line does the real fit. The importat return values are `idx` and `fit_cost`
    #       The other return values are needed only here for plotting
    idx, fit_cost, (left, right), fit = inhib_model_fit(y,
                                                        bt=tolerance,
                                                        pellet_r=pellet_radius_in_px,
                                                        th_v=th_v)

    diameter = idx/preproc.px_per_mm*2
    return diameter, idx, fit_cost, (left, right), fit, th_v


def inhib_model_fit(p, *, bt, pellet_r, th_v, pellet_v=255):
    """
    :param p: intensity profile
    :param bt: tolerance distance (int). Usually the value of 1mm in px
    :param pelletr: presumed pellet radius (int, pixels)
    :param inhib_v, bact_v : inhibition and bacteria intensty values guess
    :param pellet_v: pellet intensity value in the profile

    :return: the inhibition radius in px, the RMS of the fit
    """
    left = pellet_r

    # uncomment this lines to detect inner colonies
    # pel_end = left+bt
    # right = min(len(p)-1, pel_end + max(np.argmax(p[pel_end:]), bt) + 1)

    right = len(p)-1

    y = p[left:right]
    x = np.arange(len(y))

    vmin = p.min()
    vmax = max(th_v, 200)

    #          inhib  bact  breakpoint   slope  pellet_end  pellet_slope
    bds_min = [vmin,  vmin,   0,             0,     0,          1]
    v0 = [vmin,  vmax,   bt/2,          1,     0,          1]
    bds_max = [vmax,  vmax,   len(y),        3,     bt,         50]

    # check inconsistent boudaries
    assert all([a < b for a, b in zip(bds_min, bds_max)]
               ), "boundaries error\nmin : {}\nmax : {}".format(bds_min, bds_max)
    # check unfeasable fit
    assert all([a >= b for a, b in zip(v0, bds_min)]) and all(
        [a <= b for a, b in zip(v0, bds_max)])

    # alternatives to scipy.least_square
    # DLIB : http://dlib.net/optimization.html#find_min_box_constrained
    # ALGLIB : http://www.alglib.net/interpolation/leastsquares.php#header21
    # CERES (Google) : http://ceres-solver.org/nnls_modeling.html
    # comparison and licenses : https://en.wikipedia.org/wiki/Comparison_of_linear_algebra_libraries
    fit = optimize.least_squares(inhib_model_cost, v0,
                                 args=(x, y, pellet_v),
                                 bounds=[bds_min, bds_max],
                                 jac='3-point')
    if fit.success:
        if False: # (fit.x[0] > th_v) and ((fit.x[3] < 0.1) or (abs(fit.x[0]-fit.x[1]) < 10)):
            # no inhibition = high inhibition value && (small slope or small step hight)
            idx = 0
        elif False: #(fit.x[1] < th_v) and ((fit.x[3] < 0.1) or (abs(fit.x[0]-fit.x[1]) < 10)):
            # total bacteria = low inhibition value && (small slope or small step hight)
            idx = len(y)
        else:
            idx = fit.x[2]

        idx = int(np.round(idx))
        fit_cost = np.sqrt(inhib_model_cost(fit.x, range(
            len(p[left:])), p[left:], pellet_v).mean())
        idx += pellet_r
    else:
        raise Exception("fit failed")

    return idx, fit_cost, (left, right), fit


def inhib_model(v, x, val_pel):
    f = sigmoid
    temp = f((val_pel, v[0], v[4], v[5]), x) + \
        f((v[0], v[1], v[2], v[3]), x) - v[0]
    return temp


def inhib_model_cost(v, x, y, *args):
    err = (inhib_model(v, x, *args) - y) ** 2
    # NOTE : the value 5 hereafter is arbitrary and depends on the image size.
    err[0:5] /= 50  # penalize error on the first part (pellet)
    err += 1e5*(v[1] < v[0])  # penalize if v[1] < v[0]
    return err


def sigmoid(v, x):
    return (v[1] - v[0]) / (1 + np.exp((v[2] - x) * v[3])) + v[0]


def sigmoid_inverse(v, y, *, int_output=True):
    """
    return the x value that gives y with the given parameters vector v.
    NOTE: y must be within v[0] and v[1]
    """
    x = -np.log((v[1] - v[0]) / (y - v[0]) - 1) / v[3] + v[2]
    try:
        if int_output:
            x = int(np.round(x))
    except ValueError:
        raise astErrors.ValueError("The inverse could not be evaluated.")

    return x


def cost_sigmoid(v, x, y):
    return (sigmoid(v, x) - y) ** 2
