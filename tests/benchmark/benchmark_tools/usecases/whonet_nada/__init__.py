# benchmark_tools.whonet_nada

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

from ...interfaces import *
from ...atb_names import *
from collections import Mapping
from os import path
import pandas as pd
import datetime


class AST_annotation_WhonetNada_es_test(AST_annotation):
    """Class to access one line (an AST) of the annotation files of Creteil.
    This class behaves like a dictionary which keys are extended antibiotic names.
    """

    def __init__(self, guiID, diams, sirs):

        meta = []
        tests = []
        species = sirs[4]
        diameters = diams[6:]
        raw_sir_values = sirs[6:]
        atb_names = diams.index[6:]
        sample_date = diams[2]
        sample_type = diams[3]

        sample_date = datetime.datetime.strptime(
            str(sample_date), '%Y-%m-%d %H:%M:%S').strftime('%d/%m/%Y')

        # Organism
        species_dict = {
            "eco": "Escherichia coli",
            "sau": "Staphylococcus aureus",
            "kpn": "Klebsiella pneumoniae",
        }
        species = species_dict.get(species, species)

        # Specimen type
        sample_dict = {
            "sa": "BLOOD",
            "ur": "URINE",
            "lc": "CRM",
            "ab": None,  # abcès or pus ?
            "bl": "BLOOD",
            "sb": None,
            "bo": "BONE",
            "ti": "TISSUE",
            "ps": None,
            "fb": None
        }
        sample_type = sample_dict.get(sample_type, None)

        # Add an offset to match original excel line number (because of headers and index start at 0)
        self.ast_id = guiID + 2
        self.species = species
        self.expert_system_status = []
        self.sample_date = sample_date
        self.atb_names = list(map(
            lambda whonet_code: self.getI2Ashortname(whonet_code),
            atb_names.tolist()
        ))
        self.sir_values = None
        self.raw_sir_values = raw_sir_values.tolist()
        self.diameters = diameters.tolist()
        self.sample_type = sample_type

    def getI2Ashortname(self, whonet_code):
        try:
            return i2a.whonet_code2short(whonet_code)
        except:
            return whonet_code


class Annotations_set_WhonetNada_es_test(Annotations_set):
    """Help accessing and combining SIR annotation results.
    It beheaves like a dictionary which keys are the guiID (id of the pictures)"""

    def __init__(self, annotation_folder):
        self.files = {
            "sir_and_diam": path.join(annotation_folder, "SRI_and_diameter_amman.xlsx"),
        }

        self.xl = Annotations_set_WhonetNada_es_test.read_annotation_file(
            self.files["sir_and_diam"])

        self.diam_df = pd.read_excel(self.xl, 'diametre')
        self.sir_df = pd.read_excel(self.xl, 'SRI')
        assert len(self.diam_df) == len(self.sir_df)

        self.atb_names = self.diam_df.keys()[6:]
        self.ast_ids = list(self.diam_df.index)

    @staticmethod
    def read_annotation_file(path):
        xl = pd.ExcelFile(path)
        return xl

    def get_ast(self, numero):
        try:
            diams = self.diam_df.loc[numero]
            sirs = self.sir_df.loc[numero]
        except:
            raise KeyError("ID not found")

        return AST_annotation_WhonetNada_es_test(numero, diams, sirs)

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
    return Annotations_set_WhonetNada_es_test(annotation_folder)
