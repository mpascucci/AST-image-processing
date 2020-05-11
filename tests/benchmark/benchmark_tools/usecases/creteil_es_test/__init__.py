# benchmark_tools.creteil

# Copyright 2019 Fondation Medecins Sans Frontières https://fondation.msf.fr/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This file is part of the ASTapp image processing library
# Author: Marco Pascucci

from ...interfaces import *
from collections import Mapping
from os import path
import pandas as pd


class AST_annotation_Creteil_es_test(AST_annotation):
    """Class to access one line (an AST) of the annotation files of Creteil.
    This class behaves like a dictionary which keys are extended antibiotic names.
    """

    def __init__(self, guiID, diams, sirs):

        meta = []
        tests = diams[2:5]
        tests_answered = diams[6:8]
        species = sirs[1]
        diameters = diams[9:]
        sir_values = sirs[9:]
        atb_names = diams.index[9:]
        sample_date = diams[0]

        self.ast_id = guiID
        self.species = species
        self.expert_system_status = []
        for k in tests_answered.index:
            value = tests_answered[k]
            if not pd.isna(value):
                self.expert_system_status.append(
                    {'name': k, 'value': value, 'input': True})
        for k in tests.index:
            value = tests[k]
            if not pd.isna(value):
                self.expert_system_status.append(
                    {'name': k, 'value': value, 'input': False})
        self.sample_date = sample_date
        self.atb_names = atb_names.tolist()
        self.sir_values = sir_values.tolist()
        self.raw_sir_values = None
        self.diameters = diameters.tolist()
        self.sample_type = None


class Annotations_set_Creteil_es_test(Annotations_set):
    """Help accessing and combining SIR annotation results.
    It beheaves like a dictionary which keys are the guiID (id of the pictures)"""

    def __init__(self, annotation_folder):
        self.files = {
            "diam": path.join(annotation_folder, "Extraction_SIRscan_diameter_2018_2019.csv"),
            "sir": path.join(annotation_folder, "Extraction_SIRscan_SIR_2018_2019.csv")
        }
        self.diam_df = Annotations_set_Creteil_es_test.read_annotation_file(
            self.files["diam"])
        self.sir_df = Annotations_set_Creteil_es_test.read_annotation_file(
            self.files["sir"])
        assert len(self.diam_df) == len(self.sir_df)

        self.diam_df = self.diam_df[self.diam_df["Germe (libellé)"]
                                    != "Germe (libellé)"]
        self.sir_df = self.sir_df[self.sir_df["Germe (libellé)"]
                                  != "Germe (libellé)"]

        self.atb_names = self.diam_df.keys()[9:]
        self.ast_ids = list(self.diam_df.index)

    @staticmethod
    def read_annotation_file(path):
        df = pd.read_csv(
            path, sep=';', index_col="Numero_Guilhem", encoding='latin1')
        return df

    def get_ast(self, numero):
        try:
            diams = self.diam_df.loc[numero]
            sirs = self.sir_df.loc[numero]
        except:
            raise KeyError("ID not found")

        return AST_annotation_Creteil_es_test(numero, diams, sirs)

    def get_ast_slice(self, slice_instance):
        out = []
        start = slice_instance.start
        stop = slice_instance.stop
        step = slice_instance.step if (slice_instance.step is not None) else 1
        for i in range(start, stop, step):
            if i in self.ast_ids:
                out.append(self.get_ast(i))
        return out


def get_annotations_set(annotation_folder):
    return Annotations_set_Creteil_es_test(annotation_folder)
