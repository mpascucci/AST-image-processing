// Copyright 2019 Copyright 2019 The ASTapp Consortium
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    <http://www.apache.org/licenses/LICENSE-2.0>
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Marco Pascucci <marpas.paris@gmail.com>.

#include <tensorflow/lite/c/c_api.h>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <opencv2/core/mat.hpp>
#include <utility>

#include "astExceptions.hpp"
#include "pellet_label_recognition.hpp"
#include "pellet_label_tflite_model.hpp"

using namespace std;

namespace astimp {

// Runs the model inference. Returns error string on error. Uses the C language,
// not C++.
extern "C" const char *runInference(float *inputs, size_t length,
                                    float *outputs, size_t outputs_length) {
    TfLiteModel *model = TfLiteModelCreate(PELLET_LABEL_TFLITE_MODEL,
                                           PELLET_LABEL_TFLITE_MODEL_SIZE);

    TfLiteInterpreterOptions *options = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(options, 2);
    TfLiteInterpreter *interpreter = TfLiteInterpreterCreate(model, options);

    if (TfLiteInterpreterAllocateTensors(interpreter) == kTfLiteError) {
        return "TfLiteInterpreterAllocateTensors";
    }

    TfLiteTensor *input_tensor =
        TfLiteInterpreterGetInputTensor(interpreter, 0);
    if (TfLiteTensorCopyFromBuffer(input_tensor, inputs,
                                   length * sizeof(float)) == kTfLiteError) {
        return "TfLiteTensorCopyFromBuffer";
    }
    if (TfLiteInterpreterInvoke(interpreter) == kTfLiteError) {
        return "TfLiteInterpreterInvoke";
    }
    const TfLiteTensor *output_tensor =
        TfLiteInterpreterGetOutputTensor(interpreter, 0);
    if (TfLiteTensorCopyToBuffer(output_tensor, outputs,
                                 outputs_length * sizeof(float)) ==
        kTfLiteError) {
        return "TfLiteTensorCopyToBuffer";
    }

    TfLiteInterpreterDelete(interpreter);
    TfLiteInterpreterOptionsDelete(options);
    TfLiteModelDelete(model);
    return nullptr;
}

// Returns the label and confidence score based on the model output.
Label_match getMatch(const vector<float> &outputs) {
    set<LabelAndConfidence, ConfidenceComparator> labels_and_confidence;

    for (size_t i = 0; i < outputs.size(); i++) {
        LabelAndConfidence label_and_confidence{PELLET_LABELS[i], outputs[i]};
        labels_and_confidence.insert(label_and_confidence);
    };

    return Label_match(labels_and_confidence);
}

// TODO(jakuba): Run recognition on all inputs at once, not one by one.
vector<Label_match> PelletLabelRecognitionUsingML::getPelletsText(
    const vector<cv::Mat> &pelletImages) {
    vector<Label_match> results;
    vector<float> outputs(PELLET_LABELS.size());
    for (vector<float> inputs : getPelletModelInputs(pelletImages)) {
        const char *error = runInference(&inputs.front(), inputs.size(),
                                         &outputs.front(), outputs.size());
        if (error != nullptr) {
            throw astimp::Exception::generic(error, __FILE__, __LINE__);
        }
        results.emplace_back(getMatch(outputs));
    }
    return results;
}

Label_match PelletLabelRecognitionUsingML::getOnePelletText(
    const cv::Mat &pelletImage) {
    return getPelletsText({std::move(pelletImage)})[0];
}
}  // namespace astimp