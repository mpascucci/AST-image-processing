# Lint as: python3
"""
Benchmarks improc performance against a golden set of known results.
See benchmark/README.md for more documentation.
"""

import astimp
from imageio import imread
import matplotlib.pyplot as plt
import numpy as np
from argparse import ArgumentParser
from benchmark_utils import *
from preprocess_img import *
from tqdm import tqdm

from multiprocessing import Pool, cpu_count
from functools import partial

STOP_DISPLAY = False

def run_improc_analysis(img_dir, img_filename, pickle_preproc, method):
    """
    Runs improc analysis:
      1. Preprocesses the image using 1 of 2 methods:
         Fast option: If pickle_preproc is provided, reads preproccessing data 
         from pickle file.
         Slow option: If pickle_preproc is not provided, reads the image and
         runs preprocessing.
      2. Finds inhibition zone diameters
      3.
        1. list of ImprocAtbResult. This represents all the antibiotics that
            improc found in the image.
        2. Cropped AST image.
    Args:
      img_filename: Name of the AST image. This will be used to read either
        the original image or $preproc_dir/
      pickle_preproc: ndarray. pre-cropped AST image.
    """
    if pickle_preproc:
        img_preproc = pickle_preproc[img_filename]
    else:
        img_path = os.path.join(img_dir, img_filename)
        img_preproc = preprocess_one_image(img_path)
    circles = img_preproc['circles']
    labels = img_preproc['labels']
    preproc = img_preproc['preproc']

    if not circles or not preproc:
        return [], img_preproc

    #* VOTE-COUNT
    if method == "vote-count":
        inhibs = astimp.measureDiameters(preproc)
        # uncomment this to switch back to the python version of the measuring function
        # inhibs = [inhib_diameter_modes.count.measureOneDiameter(preproc, i) for i in range(len(circles))]

    #* MIN-MAX
    elif method == "minmax":
        inhibs = [astimp.measureOneDiameterMinMax(preproc, i) for i in range(len(circles))]

    #* STUDENT
    elif method == "student":
        from inhib_diameter_modes.sudent import measureOneDiameter
        inhibs = []
        for idx in range(len(circles)):
            inhibs.append(measureOneDiameter(preproc,idx))        
    else:
        raise ValueError("{} is not a valid method".format(method))

    results = [ImprocAtbResult(c, l, i)
               for c, l, i in zip(circles, labels, inhibs)]

    return results, img_preproc['crop']


def find_diam_diffs_for_atb(atb_results, expected_diams):
    """
    For an single antibiotic within an AST analysis, finds the  differences
    between expected inhibition zone diameters and the inhibition zone
    diameters returned by improc.
    This method returns empty if the number of pellets found for the antibiotic
    differs between improc's results and the annotation (which indicates that
    a pellet was mislabeled.)

    For example, if:
        atb_results = [ImprocAtbResult(diam=4.0), ImprocAtbResult(diam=5.6)]
        expected_diams = [4.5, 6.0]
    then:
        returns: [0.5, 0.4]
        atb_results' expected_diams updated to 4.5 and 6.0
    Returns:
      List of diffs between actual and expected diameters. (actual - expected)"""
    if len(atb_results) != len(expected_diams):
        # If the number of pellets found for the antibiotic is wrong, then some
        # of the pellets must be mislabeled. We don't know which diameters to
        # compare, so just return empty. This antibiotic will not factor into
        # diameter measurement.
        return []

    diam_diffs = []
    expected_diams = sorted(expected_diams)
    sorted_results = sorted(atb_results, key=lambda result: result.inhib_diam)
    for result, expected in zip(sorted_results, expected_diams):
        result.expected_diam = expected
        diam_diffs.append(result.inhib_diam - expected)

    return diam_diffs


def find_diam_diffs_for_ast(improc_results, expected_atbs):
    """
    For an entire AST analysis, finds the minimal differences between expected
    inhibition zone diameters and the inhibition zone diameters returned by improc.
    This method sets each improc_result.expected_diam if it is matched with an expected
    diameter.

    For example, if:
        expected_atbs = {'ATB1': [13.1, 9.7], 'ATB2':[5.0], 'ATB3': [6.0]}
        improc_results = [
            ImprocAtbResult(label=ATB1, inhib_diam=10.0, expected_diam=None),
            ImprocAtbResult(label=ATB1, inhib_diam=12.5, expected_diam=None),
            ImprocAtbResult(label=ATB2, inhib_diam=4.3, expected_diam=None),
            ImprocAtbResult(label=ATB3, inhib_diam=5.0, expected_diam=None)
        ]
    Then:
      returns: [
          0.6,  # ATB1: 13.1 - 12.5
          -0.8, # ATB1: 9.7 - 10.0
          0.7,  # ATB2: 5.0 - 4.3 
          1.0   # ATB3: 6.0 - 5.0.
        ]
      expected_atbs is updated to: {'ATB1': [13.2] }
      improc_results is updated to: 
        improc_results = [
            ImprocAtbResult(label=ATB1, inhib_diam=10.0, expected_diam=9.7),
            ImprocAtbResult(label=ATB2, inhib_diam=4.5, expected_diam=-0.5),
            ImprocAtbResult(label=ATB3, inhib_diam=4.3, expected_diam=-1.7),
            ImprocAtbResult(label=ATB3, 1inhib_diam=0.0, expected_diam=None)
        ]

    Args:
      improc_results: list(ImprocAtbResult).
      expected_atbs: bool. Whether to display an image of each AST.

    Returns:
      List of diffs between actual and expected diameters.
    """
    diam_diffs = []
    improc_results_per_atb = defaultdict(list)
    for result in improc_results:
        improc_results_per_atb[result.label].append(result)

    # For each antibiotic that improc found, attempt to match it up with
    # the closest expected antibiotic with the same label.
    for improc_label, atb_results in improc_results_per_atb.items():
        expected_diams = expected_atbs[improc_label]
        diam_diffs += find_diam_diffs_for_atb(atb_results, expected_diams)

    return diam_diffs

def run_one_benchmark(item, img_dir, results, preproc_results, method, use_multiproc=True):
    filename, annotation = item
    if not use_multiproc: tqdm.write("processing file: {}".format(filename))
    try:
        improc_results, ast_image = run_improc_analysis(
            img_dir, filename, preproc_results, method)
    except FileNotFoundError as e:
        if not use_multiproc: tqdm.write("File not found: {}".format(e))
        return (None,None,None)
    except ErrorInPreproc as e:
        if not use_multiproc: tqdm.write("Improc threw expection on file {} {}".format(filename, e))
        results.record_exception()
        return (None,None,None)
    except Exception as e:
        # raise ## UNCOMMENT FOR DEBUG
        if not use_multiproc: tqdm.write("Error {} {}".format(filename, e))
        return (None,None,None)

    if use_multiproc:
        # makes benchmark run faster in multiprocess mode
        ast_image = None

    return improc_results, annotation, ast_image


def run_benchmark(config, display, img_dir, preproc_dir, jobs=1, method="vote-count", show_result=False):
    """
    Runs AST benchmark as described at the top of this file.

    Args:
      config: Config data derived from benchmark_golden_results.yml,
      parsed by parse_and_validate_config().
      display: bool. Whether to display an image of each AST.
        If true, img_dir is required, even if preproc_dir is provided.
      Provide one of: 
        image_dir: Directory containing AST images. May be None if
            display=false and preproc_dir != None.
        preproc_dir: Directory containing preprocessing results
            generated by preproc.py.
      jobs : number of jobs for multiprocessing
    """
    # Counters to accumulate metrics across all AST files.
    results = BenchmarkResult()
    preproc_results = None
    if preproc_dir:
        preproc_results = PreprocResults(preproc_dir)

    # Process each file individually, incrementing the counters above.

    f = partial(run_one_benchmark, img_dir=img_dir,
        results=results,preproc_results=preproc_results,method=method)

    if jobs > 1 and not display:
        # use multiprocessing
        print("Running in parallel on {} processes".format(jobs))
        with Pool(jobs) as p:
            out = list(tqdm(p.imap(f,config.items()), total=len(config.items())))

        for improc_results, annotation, _ in out:
            if improc_results is None and annotation is None:
                continue
            results.record_ast_result(improc_results, annotation)
            results.record_diam_diffs(
                find_diam_diffs_for_ast(improc_results, annotation))
    else:
        # use only one process
        for item in tqdm(config.items()):
            filename, annotation = item
            improc_results, annotation, ast_image = f(item, use_multiproc=False)
            if improc_results is None and annotation is None:
                continue
            
            results.record_ast_result(improc_results, annotation)
            diffs = find_diam_diffs_for_ast(improc_results, annotation)
            results.record_diam_diffs(diffs)
            if display:
                plot_ast_result(improc_results, ast_image, filename, annotation)
                if STOP_DISPLAY:
                    break

    if show_result:
        results.show_results()

    return results.diam_diffs


def plot_atb_result(result, plt, ax, scale):
    """ Plots the label and inhibition zone for a single antibiotic. """
    center = result.pellet.center
    d_inhib_mm = round(result.inhib_diam, 2)
    r_inhib_px = result.inhib_diam/scale/2
    text_offset = result.pellet.radius + 10

    text_loc = np.array(center)+[text_offset, text_offset]
    s = f"{result.label}\nd={d_inhib_mm}"
    box_color = 'green' if result.expected_diam else 'red'
    bbox = dict(boxstyle="square", ec=box_color, fc=box_color, alpha=0.4)
    text = plt.Text(*text_loc, s, color='k', bbox=bbox)
    ax.add_artist(text)
    ax.add_artist(plt.Circle(center, r_inhib_px, ec='r', fc='none', ls='--'))
    if result.expected_diam:
        expected_inhib_px = result.expected_diam/scale/2
        ax.add_artist(plt.Circle(
            center, expected_inhib_px, ec='lightgreen', fc='none', ls='--'))

def key_press_callback(event):
    # print('press', event.key)
    # sys.stdout.flush()
    global STOP_DISPLAY
    if event.key == 'escape':
        STOP_DISPLAY = True
    plt.close()


def plot_ast_result(results, image, filename, annotation):
    """
    Displays the original AST image, with improc's found antibiotic labels,
    inhibition zone diameters, and golden inhibition zone diameters if found.

    Args:
      results: List of ImprocAtbResult to be overlaid on image.
      image: ndarray, AST image to display.
      filename: String, name of the image file being processed. Used to add a
          title to the display.
      missing_expected_atbs: list of strings. Will be added to a subtitle
          informing the user of which antibiotics were expected, but improc did
          not find them.
    """
    fig = plt.figure(figsize=(7, 7))
    fig.canvas.mpl_connect('key_press_event', key_press_callback)
    plt.imshow(image)
    plt.title(filename)
    plt.suptitle("press any key to continue, ESC to stop.")
    ax = plt.gca()
    scale = astimp.get_mm_per_px([result.pellet for result in results])

    for result in results:
        plot_atb_result(result, plt, ax, scale)

    missing_expected_atbs = set(annotation.keys()) - set([r.label for r in results])
    if len(missing_expected_atbs) > 0:
        missing_atb_strs = []
        for label in missing_expected_atbs:
            diams = annotation[label]
            missing_atb_strs += ["%s:%s" % (label, diam) for diam in diams]
        subtitle = "These antibiotics were expected, but not found: %s" % \
            ", ".join(missing_atb_strs)
        plt.annotate(subtitle, (0, 0), (0, -20), xycoords='axes fraction',
                     textcoords='offset points', wrap=True)

    plt.plot()
    red_line = plt.Line2D(range(1), range(1), color="r", linewidth=0,
                          marker='o', markersize=15, markerfacecolor="none")
    green_line = plt.Line2D(range(1), range(1), color="lightgreen",
                            linewidth=0, marker='o', markersize=15,
                            markerfacecolor="none")
    plt.legend((red_line, green_line),
               ('Improc inhib diam', 'Expected inhib diam'), numpoints=1,
               framealpha=0.6, bbox_to_anchor=(0, 0, 0.1, 0.2),
               loc="lower left", borderaxespad=0, ncol=2)
    plt.axis('off')
    plt.show()


def main():
    parser = ArgumentParser()
    parser.add_argument("-d", "--display", dest="display", action='store_true',
                        help="Display each AST processing result.")
    parser.add_argument("-c", "--config_file", dest="config_file",
                        default="annotations/amman/amman_golden.yml",
                        help="Path to the file containing expected AST results.")
    parser.add_argument("-i", "--image_dir", dest="image_dir", action='store',
                        default="images/",
                        help="""Path of the directory containing AST images. 
                                At least 1 of image_dir or preproc_dir is
                                required.""")
    parser.add_argument("-p", "--preproc_dir", dest="preproc_dir", action='store',
                        default=None,
                        help="""Path of the directory containing pickle files
                                generated by preproc.py. If provided, the
                                script will not run preprocessing, and will run faster.
                                If not provided, the script will preprocess each image,
                                and will run slower. At least 1 of image_dir or
                                preproc_dir is required.""")
    parser.add_argument("-m", "--method", dest="method", action='store',
                        help="Diameter measuring method.", default="vote-count")
    parser.add_argument("-j", "--jobs-number", dest="jobs_number", action='store',
                    default=cpu_count(),
                    help="""The number of parallel processes to run for the benchmark.""")

    args = parser.parse_args()
    jobs = int(args.jobs_number)
    config = parse_and_validate_config(args.config_file)
    print("############## ASTApp benchmarking starting on %d files\n" % len(config))
    run_benchmark(config, args.display, args.image_dir, args.preproc_dir, jobs=jobs, method=args.method, show_result=True)


if __name__ == '__main__':
    main()
