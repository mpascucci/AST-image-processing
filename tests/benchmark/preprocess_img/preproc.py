from tqdm import tqdm
import os
import glob
import pickle
import numpy as np
from imageio import imread, imwrite
import astimp
from multiprocessing import Pool, cpu_count
from functools import partial

class ErrorInPreproc(Exception):
    pass


class Dataset():
    """Datasets consisting of several files in a given input_folder."""

    def __init__(self, base_path, glob_patterns=('*.jpg', '*.JPG', '*.png', "*.PNG")):
        """base_path : path to the folder where the files are stored
        glob_patterns : a list of patterns for selecting files (e.g. ['*.jpg'])"""
        assert os.path.exists(
            base_path), "input folder '{}' not found".format(base_path)
        self.base_path = base_path
        self.paths = []
        for pattern in glob_patterns:
            self.paths += glob.glob(os.path.join(base_path, pattern))
        self.names = [os.path.basename(path).split('.')[0]
                      for path in self.paths]


class PreprocResults():
    """Access to preprocessed pickled AST images"""

    def __init__(self, pickles_folder):
        if not os.path.exists(pickles_folder):
            raise FileNotFoundError("{} does not exit".format(pickles_folder))
        self.pf = pickles_folder
        self.ds = Dataset(self.pf, glob_patterns=("*.pickle",))
        self.names = self.ds.names

        errorlog_path = os.path.join(pickles_folder, "error_log.txt")
        if os.path.exists(errorlog_path):
            with open(errorlog_path, 'r') as f:
                lines = f.readlines()
                self.errors = {line.split(',')[0]: line.split(',')[
                    1] for line in lines}
        else:
            self.errors = []

    def get_by_name(self, name):
        """Load a pickle by name.

        Pickles have the same name than images
        example:
            234_SLR_ESBL.jpg <-> 234_SLR_ESBL.jpg.pickle"""

        if name in self.errors and self.errors[name].split(" ") != 'INFO':
            raise ErrorInPreproc(self.errors[name].strip())

        path = os.path.join(self.pf, name+'.pickle')
        if not os.path.exists(path):
            raise FileNotFoundError("Pickle {} not found.".format(path))

        with open(path, 'rb') as f:
            p = pickle.load(f)

        return p

    def __getitem__(self, name):
        return self.get_by_name(name)

    def get_all(self):
        """Load all pickles in input folder"""
        output = []
        for path in tqdm(self.ds.paths, desc="Loading pickles"):
            with open(path, 'rb') as f:
                p = pickle.load(f)
                output.append(p)
        return output


def preprocess_one_image(path):
    img = np.array(imread(path))  # load image
    ast = astimp.AST(img)
    
    crop = ast.crop
    circles = ast.circles
    pellets = ast.pellets
    labels = ast.labels_text
    # create preprocessing object
    # NOTE the preprocessing object is not created it no pellets where found.
    preproc = ast.preproc if len(circles) != 0 else None

    pobj = {"ast":ast,
            "preproc": preproc,
            "circles": circles,
            "pellets": pellets,
            "labels": labels,
            "crop": crop,
            "fname": os.path.basename(path),
            "inhibitions": ast.inhibitions}
    return pobj


def pickle_one_preproc(idx, output_path, image_paths, error_list, skip_existing=False, mute=True):
    if mute:
        log_function = lambda x : x
    else:
        log_function = tqdm.write
    path = image_paths[idx]
    try:
        # create output path
        fname = os.path.basename(path)  # file name from path
        ofpath = os.path.join(
            output_path, f"{fname}.pickle")  # output file path

        if skip_existing:
            # skip if output file exists already
            if os.path.exists(ofpath):
                return None

        # WARNING for an unknown reason the pickle call must be inside this function
        pobj = preprocess_one_image(path)

        with open(ofpath, 'wb') as f:
            pickle.dump(pobj, f)

        if len(pobj['circles']) == 0:
            # if no pellet found
            error_list[idx] = "INFO : {}, No pellets found".format(fname)
            log_function("No pellet found in {}".format(fname))


    except Exception as e:
        ex_text = ', '.join(map(lambda x: str(x), e.args))
        error_list[idx] = "{}, {}".format(fname, ex_text)
        log_function("Failed images: {} - {}".format(len(error_list), ex_text))

    return None
    

def preprocess(img_paths, output_path, skip_existing=False, parallel=True):
    """preprocess images and pickle the preproc object.

    img_paths : a list of paths of the image files."""


    if not os.path.exists(output_path):
        os.mkdir(output_path)

    errors = [""]*len(img_paths)

    if parallel:
        jobs = cpu_count()
        print("Running in parallel on {} processes".format(jobs))
        f = partial(pickle_one_preproc,
            image_paths=img_paths,
            output_path=output_path,
            error_list=errors,
            skip_existing=skip_existing
            )
        with Pool(jobs) as p:
            list(tqdm(p.imap(f,range(len(img_paths))), total=len(img_paths)))
        
        errors = [e for e in errors if e != ""]
    else:
        for idx in tqdm(range(len(img_paths)), desc="Preprocessing"):
            pickle_one_preproc(idx, output_path, img_paths, errors, skip_existing, mute=False)

    return errors
