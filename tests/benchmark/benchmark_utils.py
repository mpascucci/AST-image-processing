# Lint as: python3
"""
Utility functions and methods to help with benchmark.py.
"""


import numpy as np
import os
import warnings
import yaml
from collections import defaultdict
import matplotlib.pyplot as plt


class ImprocAtbResult:
    """
    Utility class to encapsulate information about a single antibiotic disk.
    """

    def __init__(self, pellet, label_result, inhib):
        self.pellet = pellet
        self.label = label_result
        self.inhib_diam = inhib.diameter
        self.expected_diam = None


class BenchmarkResult:
    """
    Utility class to encapsulate summary statistics about an AST benchmark run.
    """

    def __init__(self):
        self.num_atb_diffs = []
        self.diam_diffs = []
        self.total_expected_atb = 0
        self.total_improc_atb = 0
        self.n_files_processed = 0
        self.n_exception = 0
        self.n_no_pellet = 0
        self.n_label_match = 0
        self.label_match_denom = 0

    def record_exception(self):
        self.n_files_processed += 1
        self.n_exception += 1

    def record_ast_result(self, improc_results, expected_ast):
        self.n_files_processed += 1
        if len(improc_results) == 0:
            self.n_no_pellet += 1

        expected_num_atb = len(flatten(expected_ast.values()))
        actual_num_atb = len(improc_results)
        self.num_atb_diffs.append(actual_num_atb - expected_num_atb)
        self.total_expected_atb += expected_num_atb
        self.total_improc_atb += min(expected_num_atb, actual_num_atb)
        # Count ATB label matches: e.g. if improc returned AUG30, AUG30, P1 and 
        # golden expected AUG30, C30, P1, P1,
        # then n_label_match=2, label_match_denom=4.
        improc_labels = [result.label for result in improc_results]
        shared_labels = set(improc_labels).intersection(expected_ast.keys())
        label_matches = sum([min(improc_labels.count(label), len(expected_ast[label])) for label in shared_labels])
        self.n_label_match += label_matches
        # Maximum possible number of matches if atb-matching was perfect.
        self.label_match_denom += min(len(flatten(expected_ast.values())), len(improc_results))

    def record_diam_diffs(self, diam_diffs):
        self.diam_diffs += diam_diffs

    def show_results(self):
        print("------------ Summary Results -----------")
        perc_disks_found = self.total_improc_atb / self.total_expected_atb
        abs_diam_diffs = [abs(i) for i in self.diam_diffs]
        n_matched_pellets = len(self.diam_diffs)
        diffs_lt_1mm = sum([diff < 1.0 for diff in abs_diam_diffs])
        diffs_lt_2mm = sum([diff < 2.0 for diff in abs_diam_diffs])
        diffs_lt_3mm = sum([diff < 3.0 for diff in abs_diam_diffs])
        diffs_lt_6mm = sum([diff < 6.0 for diff in abs_diam_diffs])
        diffs_more_mm = sum([diff >= 6.0 for diff in abs_diam_diffs])
        perc_1mm_diffs = diffs_lt_1mm / n_matched_pellets
        perc_2mm_diffs = diffs_lt_2mm / n_matched_pellets
        perc_3mm_diffs = diffs_lt_3mm / n_matched_pellets
        perc_6mm_diffs = diffs_lt_6mm / n_matched_pellets
        perc_more_diffs = diffs_more_mm / n_matched_pellets
        error_classes = {
            "< 1mm" : perc_1mm_diffs,
            "1 to 2mm" : perc_2mm_diffs-perc_1mm_diffs,
            "2 to 3mm" : perc_3mm_diffs - perc_2mm_diffs,
            "3 to 6mm" : perc_6mm_diffs - perc_3mm_diffs,
            "> 6mm" : perc_more_diffs
        }

        perc_name_match = n_matched_pellets / self.total_improc_atb
        perc_exception = self.n_exception / self.n_files_processed
        print("% of disks found:                {0:.2%} ({1} / {2})"
              .format(perc_disks_found, self.total_improc_atb, self.total_expected_atb))
        print("% of diameter diffs <1mm:        {0:.2%} ({1} / {2})"
              .format(perc_1mm_diffs, diffs_lt_1mm, n_matched_pellets))
        print("% of diameter diffs <2mm:        {0:.2%} ({1} / {2})"
              .format(perc_2mm_diffs, diffs_lt_2mm, n_matched_pellets))
        print("% of diameter diffs <3mm:        {0:.2%} ({1} / {2})"
              .format(perc_3mm_diffs, diffs_lt_3mm, n_matched_pellets))
        print("% of diameter diffs <6mm:        {0:.2%} ({1} / {2})"
              .format(perc_6mm_diffs, diffs_lt_6mm, n_matched_pellets))
        print("% of diameter diffs >=6mm:        {0:.2%} ({1} / {2})"
              .format(perc_more_diffs, diffs_more_mm, n_matched_pellets))
        print("% of antibiotic name matches:    {0:.2%} ({1} / {2})"
              .format(self.n_label_match / self.label_match_denom, self.n_label_match, self.label_match_denom))
        print("% of exceptions:                 {0:.2%} ({1} / {2})"
              .format(perc_exception, self.n_exception, self.n_files_processed))
        print("Diameter diff percentiles:")
        for percentile in [.25, .5, .75, .9, .95]:
            print("  {0:.0%}ile: {1:.2f}mm".format(
                percentile, np.quantile(abs_diam_diffs, percentile)))

        # Plot 4 pie charts repeating the same data as above
        fig, axs = plt.subplots(2, 2)
        patches, _ = axs[0, 0].pie(
            [self.n_exception,
             self.n_no_pellet,
             self.n_files_processed - self.n_no_pellet - self.n_exception])
        axs[0, 0].legend(patches,
                         ["Exception", "No pellet found", "Pellets found"],
                         loc="lower left")
        axs[0, 0].set_title("result categorization")
        axs[0, 0].axis('equal')

        # patches, _ = axs[0, 1].pie([
        #     sum([diff < 1.0 for diff in abs_diam_diffs]),
        #     sum([diff >= 1.0 for diff in abs_diam_diffs])])
        # axs[0, 1].legend(patches, ["<1mm", ">=1mm"], loc="lower left")
        # axs[0, 1].set_title("diameter diffs; median= {0:.2}mm".format(
        #     np.median(abs_diam_diffs)))
        # axs[0, 1].axis('equal')

        keys = error_classes.keys()
        values = tuple(error_classes[key] for key in keys) 
        patches, _ = axs[0, 1].pie(values,colors=["#0f9e0b","#60b334","#f9ae52","#db3f34",'gray'])
        pieBox = axs[0, 1].get_position()
        axs[0, 1].set_position([pieBox.x0, pieBox.y0, pieBox.width*0.6, pieBox.height])
        axs[0, 1].legend(patches, keys, bbox_to_anchor=(1, 0), loc="lower left")
        axs[0, 1].set_title("diameter diffs; median= {0:.2}mm".format(
            np.median(abs_diam_diffs)))
        axs[0, 1].axis('equal')

        patches, _ = axs[1, 0].pie(
            [n_matched_pellets, self.total_improc_atb - n_matched_pellets])
        axs[1, 0].legend(patches,
                         ["atb name matches", "name mismatches"],
                         loc="lower left")
        axs[1, 0].set_title("Antibiotic name matches")
        axs[1, 0].axis('equal')

        patches, _ = axs[1, 1].pie(
            [self.total_improc_atb,
             self.total_expected_atb - self.total_improc_atb])
        axs[1, 1].legend(
            patches, ["Disk found", "Disk missing"], loc="lower left")
        axs[1, 1].set_title("Antibiotic disks found")
        axs[1, 1].axis('equal')

        plt.show()
        fig.savefig("last_benchmark.jpg")

# ---------------- Plot number of antibiotics found per-plate ---------------- #
        plt.hist(self.num_atb_diffs, bins=20)
        ## In the image, suptitle actually looks like title,
        ## title looks like subtitle.
        plt.suptitle("Golden vs. improc number of pellets found per-AST")
        plt.title("Left = too few pellets, Right = too many pellets",
                  fontsize=10)
        plt.xlabel(
            "Number of pellets found by improc - golden number of pellets")
        plt.ylabel("Frequency of ASTs")
        plt.show()

# -------------------- Plot diameter diffs per-antibiotic -------------------- #
        plt.hist(self.diam_diffs, bins=30)
        plt.suptitle("Diameter diffs per-antibiotic")
        plt.title("Among pellets with matched antibiotic names\n" +
                  "Left = improc too small, right = improc too large",
                  fontsize=8)
        plt.xlabel("Diameter found by improc - golden diameter")
        plt.ylabel("Frequency of ASTs")
        plt.show()


def parse_and_validate_config(config_path):
    """
    Parses expected_results.yml (present in the same directory as benchmark.py)
    Skips and logs a warning for any image files that don't exist.
    yaml library will raise errors if the required label and diameter fields
    are not present.

    Returns a map of the form:
    {
        "path/to/filename1.jpg": {
            # antibiotic name -> diameters of its inhibition zones.
            # multiple diameters means the antibiotic is present multiple times
            # on the AST plate.
            "ATB1": [25.0, 23.0],
            "ATB2": [6.0],
            ...
        }
        "path/to/filename2.jpg": {
            "ATB1": [27.0],
            "ATB3": [10.0],            
        }
    }
    """
    f = open(config_path)
    config = yaml.safe_load(f)
    f.close()
    config_map = {}
    for filename, expected_result in config.items():
        expected_atbs_in_file = defaultdict(list)
        for entry in expected_result:
            expected_label = entry['label']
            expected_atbs_in_file[expected_label].append(entry['diameter'])
        config_map[filename] = expected_atbs_in_file
    return config_map


def flatten(list_of_lists):
    """
    Returns a list consisting of the elements in the list of lists.
    e.g. [[1,2,3],[3,4],[5,6]] -> [1,2,3,3,4,5,6]
    """
    return [result for sublist in list_of_lists for result in sublist]


def find_nearest(array, value):
    """Finds the entry in array which is closest to value. """
    return array[find_nearest_index(array, value)]


def find_nearest_index(array, value):
    """Finds the index of the entry in array which is closest to value. """
    return (np.abs(np.asarray(array) - value)).argmin()
