import numpy as np
import cv2
import astimp

def subimage_by_roi(img, roi):
  left = int(roi.left)
  top = int(roi.top)
  right = int(roi.right)
  bottom = int(roi.bottom)
  return img[top:bottom,left:right]


def angular_profile(image, r, d_r=2, d_angle=1, a=0, b=359, f=np.mean):
  """Extract an angular profile from image.
  
  Params:
    - image: a grayscale image
    - r : radius
    - d_r: width. All pixels within [r-d_r//2, r+d_r//2] will be selected
    - d_angle: the resolution of the profile
    - a: start angle
    - b: end angle
    - f: function applied to the selected pixels
  """
  Y,X = np.indices(image.shape)
  cx = image.shape[1]//2
  cy = image.shape[0]//2
  X -= cx
  Y -= cy

  R = np.sqrt(X**2+Y**2).astype(np.uint64)
  
  Z = X-1j*Y
  theta = np.angle(Z*np.exp(1j*np.pi)) + np.pi
  theta = theta*180/np.pi


  a = a%360
  b = b%360

  if a>b:
    points = np.where(
      np.logical_or(theta>=a, theta<=b) * np.logical_and(R>=r-d_r//2, R<r+d_r//2)
    )     
  else:
    points = np.where(
      np.logical_and(theta>=a, theta<=b) * np.logical_and(R>=r-d_r//2, R<r+d_r//2)
    )
  key = np.argsort(theta[points])
  x = theta[points][key]
  y = image[points][key]

  ux = np.arange(0,360,d_angle)
  profile = np.zeros((2,len(ux)))
  for i,r in enumerate(ux):
    # NOTE: this is approximative because when x<d_angle the selected interval is smaller
    profile[:,i] = (r,f(y[abs(x-r)<d_angle]))

  return profile


def radial_profile(image, angles, d_angle=1, r_min=0, r_max=np.infty, f=np.mean, interpolation=True,):
  """Extract a radial profile from image
  
  Params:
    - image: a grayscale image
    - angles: angles at which radial profiles are extracted (iterable)
    - d_angle: angle width. Pixels will be selected if in [angle-d_angle, angle+d_angle]
    - r_min: start radius for the profile extraction
    - r_max: end radius
    - f: function to be applied to pixels at same radius
    - interpolation: if true, the profile will be interpolated from r_min to r_max resulting
        in a constant lenght vector (r_max+1-rmin).
        
  Return: a list of profiles. Each profiles is a np.array of shape (2, d) where d may change according
    to the other paramenters. If interpolation=True d=(r_max+1-rmin)
  """
  Y,X = np.indices(image.shape)
  cx = image.shape[1]//2
  cy = image.shape[0]//2
  X -= cx
  Y -= cy

  R = np.sqrt(X**2+Y**2).astype(np.uint64)
  
  r_max = min(r_max,int(max(R[cy,0],R[0,cx])))
  r_min = int(r_min)

  Z = X-1j*Y
  theta = np.angle(Z*np.exp(1j*np.pi)) + np.pi
  theta = theta*180/np.pi

  profiles = []
  for alpha in angles:
    a = (alpha - d_angle/2)%360
    b = (alpha + d_angle/2)%360
    
    if a>b:
      points = np.where(
        np.logical_or(theta>=a, theta<=b) * np.logical_and(R>=r_min, R<r_max)
      )     
    else:
      points = np.where(
        np.logical_and(theta>=a, theta<=b) * np.logical_and(R>=r_min, R<r_max)
      )
    key = np.argsort(R[points])
    x = R[points][key]
    y = image[points][key]

    ux = np.unique(x)
    profile_temp = np.zeros((2,len(ux)))
    for i,r in enumerate(ux):
      profile_temp[:,i] = (r,f(y[x==r]))
      
    if interpolation:
      profile = np.empty((2,r_max+1-r_min))
      profile[0] = np.arange(r_min,r_max+1)
      profile[1] = np.interp(profile[0],*profile_temp)
    else:
      profile = profile_temp


    profiles.append(profile)
    

  return profiles


class InvalidPelletIndex(Exception):
  pass


def get_atb_angle(ast, pivot_atb_idx, other_atb_idx):
  """get the angle between two antibiotics"""
  atb_pivot = ast.get_atb_by_idx(pivot_atb_idx)
  atb_other = ast.get_atb_by_idx(other_atb_idx)
  x,y = atb_other.pellet_circle.center
  rx = x-atb_pivot.pellet_circle.center[0]
  ry = y-atb_pivot.pellet_circle.center[1]

  return np.arctan2(ry,rx)/np.pi*180


def rotate_image(img, center=None, angle=0, adjust_size=False):
  """Rotate image.
  if adjust_size is True, adjsut the output size so that all the image is visible at the end."""
  h, w = img.shape[:2]
  if center is not None:
    cx,cy = center
  else:
    cx = w//2
    cy = h//2
    
  rot_mat = cv2.getRotationMatrix2D((cx,cy), angle, 1)

  if adjust_size:
    cos = np.abs(rot_mat[0, 0])
    sin = np.abs(rot_mat[0, 1])

    # compute the new bounding dimensions of the image
    w = int((h * sin) + (w * cos))
    h = int((h * cos) + (w * sin))

    # adjust the rotation matrix to take into account translation
    rot_mat[0, 2] += (w / 2) - cx
    rot_mat[1, 2] += (h / 2) - cy
    
  rot_img = cv2.warpAffine(img, rot_mat, (h,w))

  return rot_img, rot_mat


def rotate_ast_and_rois(ast, angle=np.pi):
  """Rotate the ast's Petri-dish image around its center.
  Output the rotated image, the updated pellets circles and ROIs"""
  rot, M = rotate_image(ast.crop, angle=angle+180)

  h, w = ast.crop.shape[:2]
  cx = w//2
  cy = h//2

  ast1 = astimp.AST(np.array(0))
  ast1._crop = rot
  ast1._px_per_mm = ast.px_per_mm
  ast1._circles = []
  for c in ast.circles:
    px,py = c.center
    x = (px - cx)
    y = (py - cy)
    px = x*M[0,0] + y*M[0,1] + rot.shape[0]//2
    py = x*M[1,0] + y*M[1,1] + rot.shape[1]//2
    ast1._circles.append(astimp.Circle((px,py),ast.px_per_mm*3))

  return rot, ast1.circles, ast1.rois


def draw_circle(img, radius, center=None, bgr_color=(255,255,255), thickness=-1):
  """draw a circle centered in the center of the image"""
  if center is None:
    cx = int(img.shape[1]//2)
    cy = int(img.shape[0]//2)
  else:
    cx,cy = list(map(int,center))
  cv2.circle(img, (cx,cy), int(radius), bgr_color, thickness=thickness)
  return img

def mask_pellet_in_atb_image(atb, bgr_color=(255,255,255)):
  """return a copy of the atb image with the pellet masked with a
  circle of the specified color."""
  masked_image = atb.img.copy()
  pellet_r = int(atb.px_per_mm * astimp.config.Pellets_DiamInMillimeters/2)
  draw_circle(masked_image, radius=pellet_r, bgr_color=bgr_color, thickness=-1)
  return masked_image

def mask_pellets_in_ast_image(ast, pellet_indices=[], bgr_color=(255,255,255), all=False):
    """Return an image of the petri dish with masked pellets.
    The pellets are replaced with a disk of the specified color.
    
    if all=True, all pellets are masked and pellet_indices is ignored."""
    masked_image = ast.crop.copy()
    pellet_r = int(ast.px_per_mm * astimp.config.Pellets_DiamInMillimeters/2)
    if all:
      pellet_indices = range(len(ast.circles))
    for pellet_idx in pellet_indices:
      draw_circle(masked_image, radius=pellet_r,
          center=ast.circles[pellet_idx].center, bgr_color=bgr_color, thickness=-1)
    return masked_image