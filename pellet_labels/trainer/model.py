import tensorflow as tf
from tensorflow.keras import layers
import numpy as np
import cv2
import zipfile
import os

from util import gcs_util as util

class InputData():
	def __init__(self, train_data, train_labels, valid_data, valid_labels, ukn_data):
		self.train_data = train_data
		self.train_labels = train_labels
		self.valid_data = valid_data
		self.valid_labels = valid_labels
		self.ukn_data = ukn_data

def format_pill_for_inference(pill, img_size):
	if pill.shape[0] > pill.shape[1]:
		pill = pill[0:pill.shape[1],0:pill.shape[1]]
	elif pill.shape[0] < pill.shape[1]:
		pill = pill[0:pill.shape[0],0:pill.shape[0]]
	if pill.shape[0] != img_size:
		pill = cv2.resize(pill, (img_size,img_size))
	return pill.reshape((img_size, img_size) + (pill.shape[-1],))

def map_to_common_space(pill):
	if pill.shape[-1] == 3:
		pill = cv2.cvtColor(pill, cv2.COLOR_BGR2GRAY)
	return pill

def load_and_preprocess_data(path, working_dir, img_size,
		class_list, ukn_classes=[]):
	"""Load and preprocess the data for training or infereance

	ukn_classes are classes that might have been removed from class_list
	to enable assessing the way the model handle uncertainty. The data matching
	those classes will be returned in ukn_data
	"""
	if not path:
		raise ValueError('No dataset file defined')

	# Getting the file name
	file_name = os.path.split(path)[1]

	if path.startswith('gs://'):
		util.download_file_from_gcs(path, os.path.join(working_dir, file_name))
		path = os.path.join(working_dir, file_name)

	with zipfile.ZipFile(path, 'r') as zip_ref:
		zip_ref.extractall(working_dir)

	# Expects extracted folder to have same name as zip file
	local_path = os.path.join(working_dir, file_name.split('.')[0])

	if not os.path.exists(local_path):
		raise ValueError('Unzipped folder wasn\'t named after zip file')

	if not os.path.exists(os.path.join(local_path, 'train')):
		raise ValueError('No train folder under unzipped folder')

	if not os.path.exists(os.path.join(local_path, 'valid')):
		raise ValueError('No valid folder under unzipped folder')

	(train_data, train_labels) = ([], [])
	(valid_data, valid_labels) = ([], [])
	ukn_data = []
	data = [(train_data, train_labels), (valid_data, valid_labels)]

	for (images, labels, folder) in [(train_data, train_labels, 'train'),
															(valid_data, valid_labels, 'valid')]:
		path = os.path.join(local_path, folder)
		for d in os.listdir(path):
			if d[0] == '.':
				continue
			for f in os.listdir(os.path.join(path, d)):
				if f[0] == '.':
					continue
				pill = cv2.imread(os.path.join(path, d, f))
				pill = format_pill_for_inference(pill, img_size)
				pill = map_to_common_space(pill)
				if not d in ukn_classes:
					label = class_list.index(d)
					labels.append(label)
					images.append(pill.reshape(pill.shape + (1,)))
				else:
					ukn_data.append(pill.reshape(pill.shape + (1,)))

	train_data = np.array(train_data)
	train_labels = np.array(tf.keras.utils.to_categorical(
		train_labels, len(class_list)))
	valid_data = np.array(valid_data)
	valid_labels = np.array(tf.keras.utils.to_categorical(
		valid_labels, len(class_list)))
	ukn_data = np.array(ukn_data)

	input_data = InputData(train_data, train_labels, valid_data,
		valid_labels, ukn_data)

	return input_data

def oversample_rare_classes(train_images, train_labels,
		sample_weights, min_samples_per_class):
	"""Duplicate training sample of classes with population < 
	min_samples_per_class"""
	# Find the index of the pellet label (it’s the index of the max output
	# because we have one output per each category).
	labels_idx = np.argmax(train_labels, axis=1)
	unique_labels, counts = np.unique(labels_idx, return_counts=True)
	duplicated_images = []
	duplicated_labels = []
	duplicated_weights = []
	for label, cnt in zip(unique_labels, counts):
		if cnt < min_samples_per_class:
			images = train_images[labels_idx == label]
			labels = train_labels[labels_idx == label]
			weights = sample_weights[labels_idx == label]
			pool_size = len(images)
			p = 0
		while cnt < min_samples_per_class:
			duplicated_images.append(images[p % pool_size])
			duplicated_labels.append(labels[p % pool_size])
			duplicated_weights.append(weights[p % pool_size])
			cnt += 1
			p += 1

	if len(duplicated_images):
		train_images = np.concatenate((train_images, duplicated_images), axis=0)
		train_labels = np.concatenate((train_labels, duplicated_labels), axis=0)
		sample_weights = np.concatenate((sample_weights,
			duplicated_weights), axis=0)
	return (train_images, train_labels), sample_weights

def get_model_inputs(image_path, img_size):
    pill = cv2.imread(image_path)
    pill = format_pill_for_inference(pill, img_size)
    pill = map_to_common_space(pill)
    pill = pill.reshape(pill.shape + (1,))
    return pill

def get_model_output(predictions, class_list):
    print("Predictions:", predictions)
    predictions_idx = np.argmax(predictions, axis=1)
    output = class_list[predictions_idx[0]]
    print("Prediction:", output)
    return output

def get_data_generator():
    return tf.keras.preprocessing.image.ImageDataGenerator(
        samplewise_center=True,
        samplewise_std_normalization=True,
        dtype='uint8')

def create_keras_model(input_shape, n_classes, dropout_rate, learning_rate):
	classifier = tf.keras.models.Sequential()
	classifier.add(layers.Conv2D(32, 8, activation='relu',
		input_shape=input_shape, data_format="channels_last"))
	classifier.add(layers.MaxPool2D(2))
	classifier.add(layers.BatchNormalization())
	classifier.add(layers.Conv2D(64, 4, activation='relu'))
	classifier.add(layers.MaxPool2D(2))
	classifier.add(layers.BatchNormalization())
	classifier.add(layers.Conv2D(128, 2, activation='relu'))
	classifier.add(layers.MaxPool2D(2))
	classifier.add(layers.BatchNormalization())
	classifier.add(layers.Flatten())
	classifier.add(layers.Dropout(dropout_rate))
	classifier.add(layers.Dense(n_classes, activation='softmax'))

	classifier.compile(optimizer=tf.keras.optimizers.Adam(lr=learning_rate),
					loss='categorical_crossentropy',
					metrics=['accuracy'])

	return classifier

