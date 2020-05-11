import numpy as np
import astimp

def r_matrix(preproc,idx):
  """calculate the radial matrix centered on pellet center"""
  img = preproc.img
  cx,cy = preproc.circles[idx].center
  x = np.arange(0,img.shape[1]) - cx
  y = np.arange(0,img.shape[0]) - cy
  X,Y = np.meshgrid(x,y)
  return np.sqrt(X**2+Y**2)

def measureOneDiameter(preproc, pellet_idx):
    """Return an inhibition diameter object with the measured diameter"""
    result = _measureOneDiameter(preproc, pellet_idx)
    return astimp.InhibDisk(diameter=result["radius"]*2, confidence=1)

def _measureOneDiameter(preproc, pellet_idx):
  """Measure inhibition radius with the student test method"""
  idx = pellet_idx
  img = preproc.img
  px_per_mm = preproc.px_per_mm
  R = r_matrix(preproc, idx)/px_per_mm
  Rmax = 20
  Rmin = 3

  # radii values to scan
  rs = np.arange(Rmin,Rmax,0.25) 

  # check if no inhibition
  area_in = img[(R>Rmin)&(R<rs[2])&(img>=0)]
  km_centers = preproc.km_centers_local[pellet_idx]
  th = km_centers[0] + (km_centers[1] - km_centers[0])/2
  if area_in.mean()*255 > th:
      return {"x":None, "y":None, "radius":3, "full_inhib":True}

  # calculate the T value for each radius
  t = [0]
  for r in rs[1:]:
    # NOTE: t-test can be calculated with scipy.stats.ttest_ind(area_in, area_out)
    area_in = img[(R>Rmin)&(R<r)&(img>=0)]
    area_out = img[(R>r)&(img>=0)]
    m_in = area_in.mean()
    v_in = area_in.var()
    m_out = area_out.mean()
    v_out = area_out.var()
    n_in = len(area_in)
    n_out = len(area_out)
    t.append(abs((m_in - m_out)/np.sqrt(v_in/n_in + v_out/n_out)))          

  t = np.array(t)
  t[0] = t[1]
  t = t/np.max(t)*255
  return {"x":rs, "y":t, "radius":rs[np.argmax(t)], "full_inhib":False}