import argparse
import os
import time
import numpy as np
import tensorflow as tf
from trainer.model import get_data_generator, get_model_inputs, get_model_output
from trainer.pellet_list import PELLET_LIST
from package_ensemble import EntropyThresholdLayer

MODEL_TFLITE = "models/ab_recognition_i2a_and_amman_data.tflite"
MODEL_KERAS = "models/ab_recognition_i2a_and_amman_data.h5"

parser = argparse.ArgumentParser(
    description='Infer the label using the saved model')
parser.add_argument(
    'image', type=str,
    help='path to the image file showing one pellet with label')
parser.add_argument(
    '--img-size', type=int, default=64,
    help='square size to resize input images to in pixel, default=64')
parser.add_argument(
    '--model-type', choices=['keras', 'tflite'], default='tflite',
    help='keras or tflite')
parser.add_argument(
    '--model', default='models/ab_recognition_i2a_and_amman_data.tflite',
    help='path to model file')

def timeit(method):
    def timed(*args, **kw):
        ts = time.time()
        result = method(*args, **kw)
        te = time.time()
        print('%r  %2.2f ms' % (method.__name__, (te - ts) * 1000))
        return result
    return timed

@timeit
def infer_keras(image_path, img_size, path):
    inputs = np.array([get_model_inputs(image_path, img_size)])
    inputs_gen = get_data_generator().flow(inputs, shuffle=False)

    classifier = tf.keras.models.load_model(
        path, {'EntropyThresholdLayer': EntropyThresholdLayer})
    predictions = classifier.predict(inputs_gen)
    get_model_output(predictions, PELLET_LIST)

@timeit
def infer_tflite(image_path, img_size, path):
    inputs = np.array([get_model_inputs(image_path, img_size)], dtype=np.float32)
    inputs_gen = get_data_generator().standardize(inputs)

    interpreter = tf.lite.Interpreter(model_path=path)
    interpreter.allocate_tensors()

    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    interpreter.set_tensor(input_details[0]['index'], inputs_gen)
    interpreter.invoke()

    predictions = interpreter.get_tensor(output_details[0]['index'])
    get_model_output(predictions, PELLET_LIST)


if __name__ == '__main__':
    args = parser.parse_args()
    print("Using model", args.model)

    if args.model_type == 'keras':
        infer_keras(args.image, args.img_size, args.model)
    elif args.model_type == 'tflite':
        infer_tflite(args.image, args.img_size, args.model)
