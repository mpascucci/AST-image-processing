import argparse
import tensorflow as tf
from package_ensemble import EntropyThresholdLayer

def get_args():
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'--source',
		type=str,
		required=True,
		help='path to model to convert')
	parser.add_argument(
		'--destination',
		type=str,
		required=True,
		help='full path to save the converted model')
	args, _ = parser.parse_known_args()
	return args

def convert(source, destination):
	#model = tf.keras.models.load_model(
	#	args.source, {'EntropyThresholdLayer': EntropyThresholdLayer})
	converter = tf.lite.TFLiteConverter.from_keras_model_file(
		source, custom_objects={'EntropyThresholdLayer': EntropyThresholdLayer})
	tflite_model = converter.convert()
	with open(destination, 'wb') as f:
		f.write(tflite_model)

if __name__=='__main__':
	args = get_args()
	convert(args.source, args.destination)