# benchmark_tool.interfaces

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

from abc import ABC, abstractmethod, abstractproperty
from .better_abc import ABCMeta, abstract_attribute
from collections import Mapping
from pandas import DataFrame
import yaml


class AST_annotation(ABC, Mapping, metaclass=ABCMeta):
    """Annotation of the results of one AST.
    This class behaves like a dictionary which keys are extended antibiotic names.
    """

    @abstract_attribute
    def ast_id(self): pass
    @abstract_attribute
    def species(self): pass
    @abstract_attribute
    def sample_date(self): pass
    @abstract_attribute
    def atb_names(self): pass
    @abstract_attribute
    def sir_values(self): pass
    @abstract_attribute
    def raw_sir_values(self): pass
    @abstract_attribute
    def diameters(self): pass
    @abstract_attribute
    def sample_type(self): pass

    @property
    def results(self):
        d = {
            "diam": self.diameters,
            "sir": self.sir_values,
            "raw_sir": self.raw_sir_values
        }
        return DataFrame(d, index=self.atb_names)

    @property
    def tested_atbs(self):
        data = self.results
        return data.dropna(thresh=1)

    @abstract_attribute
    def expert_system_status(self):
        """A list of expert system's status variables.
        each test is represented as a dict {'name':test-name, 'value':test-value}"""

    def iteritems(self):
        for atb in self:
            yield atb, self[atb]

    def __getitem__(self, key):
        return self.results.loc[key]

    def __iter__(self):
        for atb in self.results.index:
            yield atb

    def __len__(self):
        return len(self.results)

    def __repr__(self):
        lines = (
            "AST annotation : {}".format(self.ast_id),
            "species : {}".format(self.species),
            "sample type : {}".format(self.sample_type),
            "sample date : {}".format(self.sample_date),
            "tested atbs: {}".format(len(self.tested_atbs))
        )
        text = "\n".join(lines)
        return text


class Annotations_set(ABC, Mapping, metaclass=ABCMeta):
    """Collection of AST annotations"""

    @abstract_attribute
    def ast_ids(self): pass

    @abstractmethod
    def get_ast(self, ast_id):
        """Should return an instance of AST_annotation corresponding to ast_id by looking in self.df"""
        pass

    def get_ast_slice(self, slice_instance):
        """Should return a list of of AST_annotation instances"""
        raise(NotImplementedError("Slicing (list[a:b]) is not allowed by default. \
                                   Override get_ast_slice(self, slice_instance) if you want to use it"))

    def __getitem__(self, selection):
        if isinstance(selection, slice):
            return self.get_ast_slice(selection)
        else:
            return self.get_ast(selection)

    def __iter__(self):
        for ast_id in self.ast_ids:
            yield self[ast_id]

    def __len__(self):
        return len(self.ast_ids)

    def write_yaml(self, outfile):
        """
        Writes a YAML file representing the AST annotations.
        This YAML file can be used as input to benchmark.py.
        YAML Format is:
        ast_image_filename1.jpg:
        - {diameter: 1.0, label: atb1, sir: S}
        - {diameter: 2.0, label: atb1, sir: null}
        - {diameter: 3.0, label: atb2, sir: R}
        ast_image_filename2.jpg:
        - {diameter: 1.0, label: atb2, sir: S}
        - {diameter: 2.0, label: atb3, sir: S}


        Which corresponds to a python object like:
        {'ast_image_filename1.jpg':
        [{'label': 'atb1', 'diameter': 1.0, 'sir': 'S'},
         {'label': 'atb1', 'diameter': 2.0, 'sir': None}]
         {'label': 'atb2', 'diameter': 3.0, 'sir': 'R'}],
        'ast_image_filename2.jpg':
        [{'label': 'atb2', 'diameter': 1.0, 'sir': 'S'},
         {'label': 'atb3', 'diameter': 2.0, 'sir': 'S'}]}
         """
        with open(outfile, 'w') as file:
            yaml.dump(self.create_yaml(), file)

    def create_yaml(self):
        yaml_data = {}
        for ast_id in self.ast_ids:
            ast_yaml = []
            for _i, atb in self[ast_id].tested_atbs.iterrows():
                ast_yaml.append(
                    {'label': atb.name, 'diameter': atb.diam, 'sir': atb.sir})
            yaml_data[ast_id] = ast_yaml
        return yaml_data
