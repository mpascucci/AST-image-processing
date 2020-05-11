from .image import subimage_by_roi
import astimp

class Antibiotic():
    "an antibiotic tested in an AST"

    def __init__(self, short_name, pellet_circle, inhibition, image, roi, px_per_mm):
        self.short_name = short_name
        self.pellet_circle = pellet_circle
        self.inhibition = inhibition
        self.img = image
        self.px_per_mm = px_per_mm
        self.roi = roi
        self._center_in_roi = None

    @property
    def center_in_roi(self):
        """center relative to the roi coordinate"""
        if self._center_in_roi is None:
            cx, cy = self.pellet_circle.center
            cx -= self.roi.left
            cy -= self.roi.top
            self._center_in_roi = (cx, cy)
        return self._center_in_roi

    def __repr__(self):
        return "ATB : {n}, inhibition diameter: {d:.1f}mm".format(n=self.short_name, d=self.inhibition.diameter)


class AST():
    """Represent an AST"""

    def __init__(self, ast_image):
        self.img = ast_image
        self._crop = None
        self._petriDish = None
        self._circles = None
        self._rois = None
        self._mm_per_px = None
        self._px_per_mm = None
        self._pellets = None
        self._labels = None
        self._labels_text = None
        self._preproc = None
        self._inhibitions = None

    @property
    def crop(self):
        """cropped image of Petri dish"""
        if self._crop is None:
            self._crop = self.petriDish.img
        return self._crop
    @crop.setter
    def crop(self, image):
        self._crop = image

    @property
    def petriDish(self):
        """Petri dish"""
        if self._petriDish is None:
            self._petriDish = astimp.getPetriDish(self.img)
        return self._petriDish

    @property
    def circles(self):
        """circles representing pellets"""
        if self._circles is None:
            self._circles = astimp.find_atb_pellets(self.crop)
        return self._circles

    @property
    def rois(self):
        if self._rois is None:
            max_diam_mm = 40  # TODO: get this from config
            self._rois = astimp.inhibition_disks_ROIs(
                self.circles, self.crop, max_diam_mm*self.px_per_mm)
        return self._rois

    @property
    def mm_per_px(self):
        """image scale"""
        if self._mm_per_px is None:
            self._mm_per_px = astimp.get_mm_per_px(self.circles)
        return self._mm_per_px

    @property
    def px_per_mm(self):
        """image scale"""
        if self._px_per_mm is None:
            self._px_per_mm = 1/astimp.get_mm_per_px(self.circles)
        return self._px_per_mm

    @property
    def pellets(self):
        """subimages of the found pellets"""
        if self._pellets is None:
            self._pellets = [astimp.cutOnePelletInImage(
                self.crop, circle) for circle in self.circles]
        return self._pellets

    @property
    def labels(self):
        """label objects"""
        if self._labels is None:
            self._labels = [astimp.getOnePelletText(
                pellet) for pellet in self.pellets]
        return self._labels

    @property
    def labels_text(self):
        """label texts"""
        if self._labels_text is None:
            self._labels_text = tuple(label.text for label in self.labels)
        return self._labels_text

    @property
    def preproc(self):
        """preporc object for inhib diameter measurement"""
        if self._preproc is None:
            self._preproc = astimp.inhib_diam_preprocessing(
                self.petriDish, self.circles)
        return self._preproc

    @property
    def inhibitions(self):
        """preporc object for inhib diameter measurement"""
        if self._inhibitions is None:
            self._inhibitions = astimp.measureDiameters(self.preproc)
        return self._inhibitions

    def get_atb_by_idx(self, idx):
        return Antibiotic(short_name=self.labels[idx].text,
                          pellet_circle=self.circles[idx],
                          roi=self.rois[idx],
                          inhibition=self.inhibitions[idx],
                          image=subimage_by_roi(self.crop, self.rois[idx]),
                          px_per_mm=self.px_per_mm)

    def get_atb_idx_by_name(self, short_name):
        return self.labels_text.index(short_name)
