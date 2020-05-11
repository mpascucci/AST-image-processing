import astimp
import numpy as np

PELLET_DIAM_IN_MM = 6

def measureOneDiameter(preproc, pellet_idx, th_v=None):
    diameter, cp, pellet_end, y_count, data, mse, confidence = _measureOneDiameter(preproc, pellet_idx, th_v)
    return astimp.InhibDisk(diameter=diameter, confidence=mse)

def _measureOneDiameter(preproc, pellet_idx, th_v=None):

    if th_v is None:
        # km_centers = preproc.km_centers_local[pellet_idx]
        th_v = preproc.km_thresholds_local[pellet_idx]

    th_v /= 255

    y_count = np.array(
        astimp.radial_profile(preproc, pellet_idx, astimp.profile_type["count"], th_v))

    hv = y_count.max()
    lv = y_count.min()
    pellet_radius_in_px = int(np.round(preproc.px_per_mm)*3)

    pellet_end = 0
    mse = 0
    if all(y_count > 0):
        # evident case of no inhibition
        cp = pellet_radius_in_px
        data = y_count
    else:
        # while (y_count[pellet_end] > 0 and pellet_end < len(y_count)):
        #     pellet_end += 1
        pellet_end = int(round(preproc.px_per_mm * PELLET_DIAM_IN_MM/2))
        cp = 0
        mses = []
        
        data = y_count[pellet_end:]
        if all(data == 0):
            # evident case of full inhibition
            cp = len(y_count)
        else:
            mse = np.mean((data - hv)**2)

            for i in range(1, len(data)):
                before = data[:i]
                after = data[i:]
                err_b = before - lv
                err_a = after - hv
                err = np.concatenate((err_a,err_b))
                this_mse = np.mean(err**2)
                # print(i,this_mse)
                mses.append(this_mse)
                if this_mse < mse:
                    mse = this_mse
                    cp = i

            cp += pellet_end

    diameter = (cp)/preproc.px_per_mm*2
    
    if mse != 0:
        confidence = min(preproc.px_per_mm*250*1.5/mse,1)
    else:
        confidence = 1

    return diameter, cp, pellet_end, y_count, data, mse, confidence

# An alternative way that uses linear algebra

# def mse(signal):
#   return np.mean((signal-signal.mean() )**2)

# def cost(signal, cpts):
#   cost = mse(signal[0:cpts[0]])
#   for i in range(len(cpts)-1):
#     cost += mse(signal[cpts[i]:cpts[i+1]])
#   cost += mse(signal[cpts[-1]:])

#   return cost

# def min_MSE_n_spline_piecewise_constant(x,y,cp):
#     """ Find the spline that minimizes the MSE with the signal (x,y).
#         The x coordinates of the nodes are in the cp vetor."""

#     # Build X matrix
#     X = np.empty((len(x),len(cp)+1))
#     ones = np.ones(len(y))
#     X[:,0] = ones
#     X[:,0][cp[0]:] = 0
#     for i,p in enumerate(cp):
#         xt = np.ones(len(y)) - X[:,i]
#         xt[:p] = 0
#         X[:,i+1] = xt
#     # linear regression to find vector b
#     inv = np.linalg.inv(np.dot(X.T,X))
#     b = np.dot(inv,np.dot(X.T,y))

#     # Calculate y = Xb
#     cpx = x[np.hstack([[0], cp, [len(x)-1]])]
#     cpy = np.dot(X,b)[cpx]

#     return cpx, cpy, cost(y,cp)

# cp = [57]
# signal = np.hstack([np.random.randn(56)+5, np.random.randn(74)+20])
# plt.plot(signal)

# gmse = mse(signal)
# for cp in range(5,len(y)-5):
#   try:
#     cpx, cpy, this_mse = min_MSE_n_spline_piecewise_constant(np.arange(len(signal)),signal,[cp])
#   except np.linalg.LinAlgError:
#     continue
#   if this_mse < gmse:
#     gmse = this_mse
#     final_cp = cpx

# # plot
# for cp in final_cp:
#   plt.axvline(cp, color='r')
