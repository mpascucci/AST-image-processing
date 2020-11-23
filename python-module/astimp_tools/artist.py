import matplotlib.pyplot as plt
import astimp
from .image import subimage_by_roi

default = {
    "artist": {
        'fill': False,
        'ls': '--'},
}


def apply_custom_defaults(d, default_key):
    for k, v in default[default_key].items():
        if k not in d:
            d[k] = v
    

def draw_petri_dish(petri, ax=None):
    bb = petri.boundingBox
    # DRAW CENTER
    # ax.plot(*bb.center, 'or')

    if ax is None:
        ax = plt.gca()

    if petri.isRound:
        art = plt.Circle(petri.center, petri.radius, fill=None, ec='r', ls='--')
        ax.add_artist(art)
    else:
        draw_roi(bb, ax)


def draw_roi(roi, ax, text="", ec='r', text_color='w', text_args={}, rect_args={}):
    apply_custom_defaults(rect_args, "artist")

    rect_args["ec"] = ec
    text_args["color"] = text_color

    rect = plt.Rectangle((roi.x, roi.y), roi.width, roi.height, **rect_args)
    ax.add_artist(rect)

    if text != "":
        text = plt.text(roi.x, roi.bottom, text, **text_args)
        ax.add_artist(text)


def draw_ast(ast, ax, **kwargs):
    draw_numbers = kwargs.get("draw_numbers", True)
    ax.imshow(ast.crop)
    for j in range(len(ast.circles)):
        center = ast.circles[j].center
        pellet_r = astimp.config.Pellets_DiamInMillimeters/2
        temp = (center[0]-pellet_r*ast.px_per_mm,
                center[1]-pellet_r*ast.px_per_mm)
        s = f"{j}"
        if draw_numbers:
            bbox = dict(boxstyle="square", ec=(0, 1, 0.5), fc=(0.2, 0.6, 0.2, 0.7))
            text = plt.Text(*temp, s, color='w', bbox=bbox)
            ax.add_artist(text)

        center = ast.circles[j].center
        inhib_disk = astimp.measureOneDiameter(ast.preproc, j)
        diam = inhib_disk.diameter
        conf = inhib_disk.confidence

        if conf < 1:
            color = 'r'
        else:
            color = 'c'
        circle = plt.Circle(center, diam/2*ast.px_per_mm,
                            ec=color, fc='none', ls='--', alpha=1)
        ax.add_artist(circle)


def draw_antibiotic(atb, ax):
    ax.imshow(atb.img)
    diam = atb.inhibition.diameter
    r = diam/2*atb.px_per_mm
    cx, cy = atb.center_in_roi
    circle = plt.Circle((cx, cy), r, fill=False, ec='c', ls='--')
    ax.add_artist(circle)


def draw(obj, ax=None, **kwargs):
    if ax is None:
        ax = plt.gca()

    if isinstance(obj, astimp.Roi):
        draw_roi(obj, ax, **kwargs)
    elif isinstance(obj, astimp.AST):
        draw_ast(obj, ax, **kwargs)
    elif isinstance(obj, astimp.Antibiotic):
        draw_antibiotic(obj, ax, **kwargs)
    elif isinstance(obj, astimp.PetriDish):
        draw_petri_dish(obj, ax)
    else:
        raise AttributeError("Don't know how to draw: {}.".format(str(obj)))
