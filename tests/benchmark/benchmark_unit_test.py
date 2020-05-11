# Lint as: python3
"""
Unit tests for some internal helper methods of benchmark.py and benchmark_utils.py.
This test does not run actual benchmarking. Use benchmark.py for that.
"""

import unittest
import warnings
import astimp
from benchmark import find_diam_diffs_for_ast
from benchmark_utils import *
from collections import defaultdict

"""
Constants & helpers
"""


def buildAtbResult(label, inhib_diam):
    # pellet and confidence do not matter for these tests.
    pellet = astimp.Circle((1, 2), 6.0)
    confidence = 1.0
    return ImprocAtbResult(pellet,
                           astimp.Pellet_match(label, confidence),
                           astimp.InhibDisk(inhib_diam, confidence))


atb1 = "atb1"
atb2 = "atb2"
atb1_small_improc_diam = 10.0
atb1_large_improc_diam = 15.0
atb2_small_improc_diam = 10.0
atb2_large_improc_diam = 20.0
atb1_small_expected_diam = 12.0
atb1_large_expected_diam = 12.6
atb2_small_expected_diam = 11.0
atb2_large_expected_diam = 19.0


class BenchmarkUnitTest(unittest.TestCase):

    def test_parse_valid_config(self):
        config = parse_and_validate_config("unit_test_golden_results.yaml")

        img_1_atbs = {'ATB1': [10.1, 10.1, 15.0], 'ATB2': [4.5]}
        img_2_atbs = {'ATB1': [1.1], 'ATB2': [2.2], 'ATB3': [3.3]}
        expected_config = {
            'IMG_20180107_184023.jpg': defaultdict(list, img_1_atbs),
            'IMG_20180107_184052.jpg': defaultdict(list, img_2_atbs),
        }
        self.assertEqual(config, expected_config)

    def test_find_nearest_value_larger_than_all_options(self):
        array = [1.0, 3.0, 2.0]
        value = 3.3
        self.assertEqual(find_nearest(array, value), 3.0)
        self.assertEqual(find_nearest_index(array, value), 1)

    def test_find_nearest_value_in_middle_of_options(self):
        array = [100.3, 200.5, 150, 120]
        value = 170
        self.assertEqual(find_nearest(array, value), 150)
        self.assertEqual(find_nearest_index(array, value), 2)

    def test_flatten(self):
        list_of_lists = [[], [1, 2], ['a'], [3, 4, 5]]
        expected_flattened = [1, 2, 'a', 3, 4, 5]
        self.assertEqual(flatten(list_of_lists), expected_flattened)

    def test_find_diams_all_atb_matched(self):
        improc_result = [
            buildAtbResult(atb1, atb1_small_improc_diam),
            buildAtbResult(atb1, atb1_large_improc_diam),
            buildAtbResult(atb2, atb2_large_improc_diam),
            buildAtbResult(atb2, atb2_small_improc_diam)
        ]
        expected_diams = {
            atb1: [atb1_small_expected_diam, atb1_large_expected_diam],
            atb2: [atb2_small_expected_diam, atb2_large_expected_diam]
        }

        diam_diffs = find_diam_diffs_for_ast(improc_result, expected_diams)

        expected_diffs = [
            atb1_small_improc_diam - atb1_small_expected_diam,
            atb1_large_improc_diam - atb1_large_expected_diam,
            atb2_small_improc_diam - atb2_small_expected_diam,
            atb2_large_improc_diam - atb2_large_expected_diam,
        ]
        # order does not matter
        self.assertListEqual(sorted(diam_diffs), sorted(expected_diffs))
        # all improc diameters are matched to their closest expected diameter.
        self.assertEqual(
            improc_result[0].expected_diam, atb1_small_expected_diam)
        self.assertEqual(
            improc_result[1].expected_diam, atb1_large_expected_diam)
        self.assertEqual(
            improc_result[2].expected_diam, atb2_large_expected_diam)
        self.assertEqual(
            improc_result[3].expected_diam, atb2_small_expected_diam)

    def test_find_diam_diffs_for_name_mismatch(self):
        improc_result = [buildAtbResult(atb1, atb1_small_improc_diam)]
        expected_diams = {
            atb1: [atb1_small_expected_diam, atb1_large_expected_diam]
        }
        diam_diffs = find_diam_diffs_for_ast(improc_result, expected_diams)

        # No diffs found because there are a different number of ATB1 in the
        # golden data vs. improc result.
        self.assertListEqual(diam_diffs, [])
        self.assertEqual(improc_result[0].expected_diam, None)

    def test_record_results_label_match(self):
        all_results = BenchmarkResult()
        annotation = defaultdict(list, {'ATB1': [1,2,3], 'ATB2': [4]} )
        improc_results = [
            buildAtbResult('ATB1', 2),
            buildAtbResult('ATB1', 3),
            buildAtbResult('ATB3', 4),
        ]
        all_results.record_ast_result(improc_results, annotation)
        self.assertEqual(all_results.n_label_match, 2)
        self.assertEqual(all_results.label_match_denom, 3)

        annotation = defaultdict(list, {'ATB1': [10.1], 'ATB3': [2.0], 'ATB4': [6.0]} )
        improc_results = [
            buildAtbResult('ATB1', 9.0),
            buildAtbResult('ATB2', 11.0),
            buildAtbResult('ATB3', 5.0),
        ]
        all_results.record_ast_result(improc_results, annotation)
        self.assertEqual(all_results.n_label_match, 4)
        self.assertEqual(all_results.label_match_denom, 6)

if __name__ == '__main__':
    unittest.main()
