import numpy as np

def total_inhibition(data, preproc, pellet_idx):
    """Return true if estimate that there is no inhibition in this profile"""
    if all(data > preproc.km_thresholds_local[pellet_idx]):
        return True
    else:
        return False


def no_inhibition(data, preproc, pellet_idx):
    """Return true if estimate that there is no inhibition in this profile"""
    if all(data < preproc.km_thresholds_local[pellet_idx]):
        return True
    else:
        return False

def varianceHall(y):
  n = len(y)
  d = [0.1942, 0.2809, 0.3832, - 0.8582]
  sigma2 = 0
  for j in range(1,n-3):
    sigma2 += (d[0]*y[j+0] + d[1]*y[j+1] + d[2]*y[j+2] + d[3]*y[j+3])**2
  return 1/(n-3)*sigma2

def varianceHallDiff(y):
  n = len(y)
  d = [0.1942, 0.2809, 0.3832, - 0.8582]
  sigma2 = 0
  corrector = np.sqrt(d[3]**2 + (d[2]-d[3])**2 + (d[1]-d[2])**2 + (d[0]-d[1])**2 + d[0]**2)
  for j in range(1,n-4):
    sigma2 += (d[0]*(y[j+0+1]-y[j+0]) + d[1]*(y[j+1+1]-y[j+1]) + d[2]*(y[j+2+1]-y[j+2]) + d[3]*(y[j+3+1]-y[j+3]))**2
  return 1/(n-4)/corrector*sigma2