// Copyright 2019 Fondation Medecins Sans Frontières https://fondation.msf.fr/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the ASTapp image processing library

#ifndef ASTIMP_PELLET_LABEL_TFLITE_MODEL_HPP
#define ASTIMP_PELLET_LABEL_TFLITE_MODEL_HPP

#include <cstddef>
#include <string>
#include <vector>

using namespace std;

// Pellet labels used as outputs while training the model. Generated from the Python list.
extern const vector<string> PELLET_LABELS;
// The trained .tflite model.
extern const char *PELLET_LABEL_TFLITE_MODEL;
// The size of PELLET_LABEL_TFLITE_MODEL.
extern size_t PELLET_LABEL_TFLITE_MODEL_SIZE;

#endif //ASTIMP_PELLET_LABEL_TFLITE_MODEL_HPP
