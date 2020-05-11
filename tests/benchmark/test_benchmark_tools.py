# coding=utf8
import unittest
from benchmark_tools import atb_names
import benchmark_tools as bt
import os
from pandas import DataFrame
import numpy as np

creteil_set = bt.creteil.Annotations_set_Creteil("annotations/creteil")
amman_set = bt.amman.Annotations_set_Amman('annotations/amman/amman_test.csv')


class BenchmarkToolsUnitTest(unittest.TestCase):

    def test_amman_annotations(self):
        self.assertEqual(len(amman_set.ast_ids), 2)

        amman_atb = amman_set.get_ast(amman_set.ast_ids[0])
        self.assertEqual(amman_atb.ast_id, "20181024-SAU-20.jpg")
        self.assertEqual(amman_atb.species, "sau")
        self.assertEqual(amman_atb.sample_date, "7/25/2019 12:00:00 AM")
        self.assertEqual(amman_atb.sir_values, None)
        self.assertEqual(amman_atb.sample_type, "sb")

        tested_atb_names = [row.name for i,
                            row in amman_atb.tested_atbs.iterrows()]
        self.assertEqual(tested_atb_names, ['AK30', 'FOX30', 'C30'])
        tested_atb = amman_atb.tested_atbs.loc['AK30']
        self.assertEqual(tested_atb.name, "AK30")
        self.assertEqual(tested_atb.diam, 7.0)
        self.assertEqual(tested_atb.sir, None)

        not_tested_atb = amman_atb['AUG30']
        self.assertEqual(not_tested_atb.name, 'AUG30')
        self.assertTrue(np.isnan(not_tested_atb.diam))
        self.assertEqual(tested_atb.sir, None)

    def test_create_yaml_amman(self):
        expected_yaml = {'20181024-SAU-20.jpg':
                         [{'label': 'AK30', 'diameter': 7.0, 'sir': None},
                          {'label': 'FOX30', 'diameter': 6.0, 'sir': None},
                          {'label': 'C30', 'diameter': 25.0, 'sir': None}],
                         '20190102-SAU-11.jpg':
                         [{'label': 'AK30', 'diameter': 6.0, 'sir': None},
                          {'label': 'FOX30', 'diameter': 6.0, 'sir': None},
                          {'label': 'C30', 'diameter': 25.0, 'sir': None}]}
        self.assertEqual(amman_set.create_yaml(), expected_yaml)

    def test_creteil_annotations(self):
        self.assertEqual(
            creteil_set[545]["PIPERACILLINE 30µg"].diam, 24)
        self.assertEqual(
            creteil_set[545]["PIPERACILLINE 30µg"].sir, 'S')
        self.assertEqual(creteil_set[500].species, "Escherichia coli")

    def test_atb_names_translation(self):
        self.assertEqual(atb_names.i2a.full2short(
            'CIPROFLOXACINE 5µg'), "CIP5")
        self.assertEqual(atb_names.i2a.short2full(
            "CIP5"), 'CIPROFLOXACINE 5µg')

    def test_ast_script(self):
        expected_ast_script = """# ASTscript automatically generated from Creteil annotations
# script creation date : 11/11/2019
# sample date : 12/04/2017

SPECIES : Streptococcus constellatus

ATB : PENICILLINE G 1U, 20, NA, NA, S
ATB : OXACILLINE 1µg, 6, NA, NA, PASVAL
ATB : AMPICILLINE 2µg, 16, NA, NA, R
ATB : AMOXICILLINE 20µg, NA, NA, NA, S
ATB : CEFOTAXIME 5µg, 6, NA, NA, S
ATB : STREPTOMYCINE 300µg, 24, NA, NA, S
ATB : GENTAMICINE 30µg, NA, NA, NA, S
ATB : GENTAMICINE 500µg, 23, NA, NA, S
ATB : NORFLOXACINE 10µg, 19, NA, NA, PASVAL
ATB : LEVOFLOXACINE 5µg, 25, NA, NA, S
ATB : MOXIFLOXACINE 5µg, 25, NA, NA, S
ATB : TRIMETHOPRIME + SULFAMIDES 1.25-23.75µg, 24, NA, NA, S
ATB : LINCOMYCINE 15µg, 23, NA, NA, S
ATB : PRISTINAMYCINE 15µg, 22, NA, NA, S
ATB : LINEZOLIDE 10µg, 26, NA, NA, S
ATB : RIFAMPICINE 5µg, 25, NA, NA, S
ATB : TETRACYCLINE 30µg, 24, NA, NA, S
ATB : TIGECYCLINE 15µg, 24, NA, NA, PASVAL
ATB : VANCOMYCINE 5µg, 19, NA, NA, S
ATB : TEICOPLANINE 30µg, 22, NA, NA, S"""

        actual_ast_script = bt.astscript.annotation_to_ASTscript(
            creteil_set[2])
        self.assertEqual(actual_ast_script.split(
            '\n')[2:], expected_ast_script.split('\n')[2:])


if __name__ == '__main__':
    unittest.main()
