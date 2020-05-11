# coding: utf8
# benchmark_tools.amman

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
# Author: Ellen Sebastian

from ...interfaces import *
from collections import Mapping
from os import path
import pandas as pd


class AST_annotation_Amman(AST_annotation):
    """Class to access one line (an AST) of the annotation files of amman.
    This class behaves like a dictionary which keys are extended antibiotic names.
    """

    def __init__(self, guiID, df_slice):

        self.ast_id = guiID
        self.species = df_slice['Organism']
        self.expert_system_status = []
        self.sample_date = df_slice['Specimen date']
        self.atb_names = df_slice.index[4:].tolist()
        self.sir_values = None
        self.raw_sir_values = None
        self.diameters = [float(i) for i in df_slice[4:].tolist()]
        self.sample_type = df_slice['Specimen type']


class Annotations_set_Amman(Annotations_set):
    """Utility class for reading and accessing AST annotations from Amman dataset.
    It behaves like a dictionary which keys are the guiID (id of the pictures)"""

    def __init__(self, annotations_file):
        self.file = annotations_file
        self.diam_df = Annotations_set_Amman.read_annotation_file(
            self.file)
        self.atb_names = self.diam_df.keys()[4:]
        self.ast_ids = list(self.diam_df.index)

    @staticmethod
    def read_annotation_file(path):
        df = pd.read_csv(path, sep=',', index_col="AST Image")
        return df

    def get_ast(self, guiID):
        try:
            df_slice = self.diam_df.loc[guiID]
        except:
            raise KeyError("ID not found")

        return AST_annotation_Amman(guiID, df_slice)

    def get_ast_slice(self, slice_instance):
        out = []
        start = slice_instance.start
        stop = slice_instance.stop
        step = slice_instance.step if (slice_instance.step is not None) else 1
        for i in range(start, stop, step):
            if i in self.ast_ids:
                out.append(self.get_ast(i))
        return out
