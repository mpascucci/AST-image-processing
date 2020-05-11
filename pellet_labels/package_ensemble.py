import argparse
import os

import numpy as np
import tensorflow as tf
from tensorflow.keras import layers
import tensorflow.keras.backend as K

from util import gcs_util as util
from trainer import task
from trainer.pellet_list import PELLET_LIST

WORKING_DIR = os.getcwd()
MODEL_FOLDER = 'pellet_labels_model'

def get_args():
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--job-dir',
		type=str,
		required=True,
		help='GCS location to the ensemble models')
	parser.add_argument(
		'--destination',
		type=str,
		default='./models/ensemble_model.h5',
		help='full path to save the ensemble model')
	parser.add_argument(
		'--n-ensemble',
		type=int,
		default=10,
		help='Number of ensemble models that were trained')
	parser.add_argument(
		'--threshold-value',
		type=float,
		default=0.023049,
		help='threshold value to determine whether\
		an input is out of distribution')
	parser.add_argument(
		'--img-size',
		type=int,
		default=64,
		help='square size to resize input images to in pixel, default=64')
	args, _ = parser.parse_known_args()
	return args

class EntropyThresholdLayer(layers.Layer):

	def __init__(self, threshold, n_classes, **kwargs):
		self.threshold = threshold
		self.n_classes = n_classes
		super(EntropyThresholdLayer, self).__init__(**kwargs)

	def build(self, input_shape):
		super(EntropyThresholdLayer, self).build(input_shape)

	def call(self, x):
		entropy = -K.sum(K.log(x + 1e-10) * x, axis=-1)
		# The predictions that don't pass the treshold are set to 0
		mask1 = K.cast(K.less_equal(entropy, self.threshold), x.dtype)
		mask1 = K.expand_dims(mask1, 1)
		mask1 = K.tile(mask1, (1, self.n_classes))
		y = x * mask1
		# Build a flattened prediction array for items that don't pass the threshold
		# in order to reflect greater uncertainty
		mask2 = K.cast(K.greater(entropy, self.threshold), x.dtype)
		mask2 = K.expand_dims(mask2, 1)
		mask2 = K.tile(mask2, (1, self.n_classes))
		flattened_pred = (x + 1) / (self.n_classes + 1)
		flattened_pred *= mask2
		return y + flattened_pred

	def get_config(self):
		config = {
				'threshold': self.threshold,
				'n_classes': self.n_classes,
		}
		base_config = super(EntropyThresholdLayer, self).get_config()
		return dict(list(base_config.items()) + list(config.items()))

def package_model(args):
	# Load ensemble models trained on gcloud and repackage them in a single graph
	# including a final entropy threshold function to enable the model to better 
	# handle uncertainty.

	assert(args.job_dir.startswith('gs://'))

	# Load models
	model_paths = util.load_models_from_gcs(
		args.job_dir, MODEL_FOLDER, task.MODEL_NAME, WORKING_DIR, args.n_ensemble)

	models = []
	for path in model_paths:
		models.append(tf.keras.models.load_model(path, {'sin': tf.sin}))

	# Create the input for the ensemble
	input_layer = layers.Input(shape=(args.img_size, args.img_size, 1))

	prediction_layers = []
	for i, model in enumerate(models):
		# Get rid of the input of the N models
		model.layers.pop(0)
		x = input_layer
		for layer in model.layers:
			layer._name += '_' + str(i)
			# Rebuild the graph for each model starting from the same input
			x = layer(x)
		# Collect the final softmax layers
		prediction_layers.append(x)

	ensemble_prediction = layers.Average()(prediction_layers)
	output_layer = EntropyThresholdLayer(
		args.threshold_value, len(PELLET_LIST))(ensemble_prediction)

	ensemble_model = tf.keras.Model(inputs=input_layer, outputs=output_layer)

	# Need parameters to compile the model but they are meaningless outside of training
	ensemble_model.compile(optimizer=tf.keras.optimizers.Adam(lr=0.001),
								loss=('categorical_crossentropy'))

	ensemble_model.save(args.destination)


if __name__ == '__main__':
	args = get_args()
	package_model(args)