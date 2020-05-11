# benchmark_tools.astscript

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

import time
from pandas import isna

ASTscript_format = {
    "atb": "ATB : {name}, {diam}, {cmi}, {raw_sir}, {interp_sir}",
    "test_in": "STATUS_IN : {name}, {value}",
    "test_out": "STATUS_OUT : {name}, {value}",
    "species": "SPECIES : {name}",
    "sample_type": "SAMPLE : {sample_type}"
}


def annotation_to_ASTscript(AST_annotation):
    """Convert the annotation of an AST to an ASTscript"""

    species = {"name": AST_annotation.species}
    atbs = []
    today = time.strftime("%d/%m/%Y")

    for name, results in AST_annotation.iteritems():

        if isna(results.sir) and isna(results.diam):
            continue

        diam = "NA" if isna(results.diam) else int(results.diam)
        sir = "NA" if isna(results.sir) else results.sir
        raw_sir = "NA" if isna(results.raw_sir) else results.raw_sir

        atbs.append({'name': name,
                     'diam': diam,
                     'cmi': 'NA',
                     "raw_sir": raw_sir,
                     'interp_sir': sir})

    ASTscript = [
        "# ASTscript automatically generated from annotations"]
    ASTscript.append(f"# script creation date : {today}")
    if AST_annotation.sample_date is not None:
        ASTscript.append(f"# sample date : {AST_annotation.sample_date}")
    ASTscript.append('')
    ASTscript.append(ASTscript_format['species'].format(**species))
    if AST_annotation.sample_type is not None:
        ASTscript.append(ASTscript_format['sample_type'].format(
            sample_type=AST_annotation.sample_type))
    ASTscript.append('')
    if AST_annotation.expert_system_status:
        def inputs(status): return status['input'] is True
        def outputs(status): return status['input'] is False
        ASTscript += [ASTscript_format['test_in']
                      .format(**test) for test in filter(inputs, AST_annotation.expert_system_status)]
        ASTscript += [ASTscript_format['test_out']
                      .format(**test) for test in filter(outputs, AST_annotation.expert_system_status)]
        ASTscript.append('')
    ASTscript += [ASTscript_format['atb'].format(**atb) for atb in atbs]

    return '\n'.join(ASTscript)
