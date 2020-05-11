import argparse
import os
import pickle

import numpy as np
import tensorflow as tf
import pandas as pd

from trainer import model
from trainer.pellet_list import PELLET_LIST, REMOVED_CLASSES
from trainer import task
from util import gcs_util as util

WORKING_DIR = os.getcwd()
MODEL_FOLDER = 'uncertainty_pellet_labels_model'

def get_args():
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--job-dir',
		type=str,
		required=True,
		help='GCS location to the ensemble models')
	parser.add_argument(
		'--train-files',
		type=str,
		required=True,
		nargs='*',
		help='Dataset training file local or GCS')
	parser.add_argument(
		'--destination-file',
		type=str,
		required=True,
		default='uncertainty_data.pickle',
		help='File name to write uncertainty data to')
	parser.add_argument(
		'--n-ensemble',
		type=int,
		default=10,
		help='Number of ensemble models that were trained')
	parser.add_argument(
		'--img-size',
		type=int,
		default=64,
		help='square size to resize input images to in pixel, default=64')
	parser.add_argument(
		'--threshold',
		type=str,
		default='std_dev',
		choices=['std_dev', 'max_p', 'entropy'],
		help='which type of threshold to use to calculate uncertainty_data')
	args, _ = parser.parse_known_args()
	return args

class Evaluator():

	def __init__(self, threshold):
		self.threshold = threshold
		STRATEGIES = {
			'entropy': self._evaluate_entropy,
			'max_p': self._evaluate_max_p,
			'std_dev': self._evaluate_std_dev
		}
		self.evaluate = STRATEGIES[threshold]

	def _evaluate_entropy(self, valid_predictions, ukn_predictions):
		uncertainty_data = []
		valid_predictions = np.mean(valid_predictions, axis=0)
		ukn_predictions = np.mean(ukn_predictions, axis=0)
		valid_entropy = compute_entropies(valid_predictions)
		ukn_entropy = compute_entropies(ukn_predictions)
		for t in np.linspace(0., np.log(10), 1000, endpoint=True):
			in_set_acc = len(valid_entropy[valid_entropy <= t]) / len(valid_entropy)
			out_set_mis = len(ukn_entropy[ukn_entropy <= t]) / len(ukn_entropy)
			uncertainty_data.append([t, in_set_acc, out_set_mis])
		return uncertainty_data

	def _evaluate_max_p(self, valid_predictions, ukn_predictions):
		uncertainty_data = []
		valid_predictions = np.mean(valid_predictions, axis=0)
		ukn_predictions = np.mean(ukn_predictions, axis=0)
		valid_confidence = np.amax(valid_predictions, axis=-1)
		ukn_confidence = np.amax(ukn_predictions, axis=-1)
		for t in np.arange(0.5, 1, 0.001):
			in_set_acc = len(valid_confidence[valid_confidence >= t]) / len(valid_confidence)
			out_set_mis = len(ukn_confidence[ukn_confidence >= t]) / len(ukn_confidence)
			uncertainty_data.append([t, in_set_acc, out_set_mis])
		return uncertainty_data

	def _evaluate_std_dev(self, valid_predictions, ukn_predictions):
		uncertainty_data = []
		valid_deviations = compute_deviations(valid_predictions)
		ukn_deviations = compute_deviations(ukn_predictions)
		for t in np.arange(0.0001, 0.1, 0.0001):
			in_set_acc = len(valid_deviations[valid_deviations <= t]) / len(valid_deviations)
			out_set_mis = len(ukn_deviations[ukn_deviations <= t]) / len(ukn_deviations)
			uncertainty_data.append([t, in_set_acc, out_set_mis])
		return uncertainty_data


def compute_entropies(predictions):
	# For a reference on distribution entropy, see [1]
	# [1]: https://peltarion.com/knowledge-center/documentation/modeling-view/build-an-ai-model/loss-functions/categorical-crossentropy
	return -np.sum(np.log(predictions + 1e-10) * predictions, axis=-1)

def compute_deviations(predictions):
	# Compute the deviation between the max probability of each prediction
	return np.std(np.max(predictions, axis=-1), axis=0)

def eval_ensemble(args):
	# Take an ensemble of models trained on gcloud and evaluate their accuracy in
	# classifying in and out of distribution data. The evaluation can be done
	# using 3 types of threshold: 'max_p', 'entropy', 'std_dev'. Outputs a pandas
	# dataframe with accuracy metrics at different threshold value
	assert(args.job_dir.startswith('gs://'))

	class_list = [pellet_class for pellet_class in PELLET_LIST
		if pellet_class not in REMOVED_CLASSES]

	train_images = []
	train_labels = []
	valid_images = []
	valid_labels = []
	ukn_images = []

	for path in args.train_files:
		input_data = model.load_and_preprocess_data(
			path,
			WORKING_DIR,
			args.img_size,
			class_list,
			REMOVED_CLASSES)
		train_images.append(input_data.train_data)
		train_labels.append(input_data.train_labels)
		valid_images.append(input_data.valid_data)
		valid_labels.append(input_data.valid_labels)
		ukn_images.append(input_data.ukn_data)

	train_images = np.concatenate(train_images, axis=0)
	train_labels = np.concatenate(train_labels, axis=0)
	valid_images = np.concatenate(valid_images, axis=0)
	valid_labels = np.concatenate(valid_labels, axis=0)
	ukn_images = np.concatenate(ukn_images, axis=0)

	# Load models
	model_paths = util.load_models_from_gcs(
		args.job_dir, MODEL_FOLDER, task.MODEL_NAME, WORKING_DIR, args.n_ensemble)

	models = []
	for path in model_paths:
		models.append(tf.keras.models.load_model(path, {'sin': tf.sin}))

	# Generate predictions
	image_gen = model.get_data_generator()

	valid_flow = image_gen.flow(valid_images, valid_labels, shuffle=False)
	ukn_flow = image_gen.flow(ukn_images, shuffle=False)

	valid_predictions = []
	ukn_predictions = []
	for m in models:
		valid_predictions.append(m.predict(valid_flow))
		ukn_predictions.append(m.predict(ukn_flow))

	evaluator = Evaluator(args.threshold)

	uncertainty_data = evaluator.evaluate(valid_predictions, ukn_predictions)

	uncertainty_data = pd.DataFrame(uncertainty_data,
		columns=[args.threshold, 'in_set_acc', 'out_set_mis'])

	uncertainty_path = os.path.join(WORKING_DIR, args.destination_file)
	with open(uncertainty_path, 'wb') as file:
		pickle.dump(uncertainty_data, file)

if __name__ == '__main__':
	args = get_args()
	eval_ensemble(args)