import unittest
import numpy as np
import package_ensemble

class TestEntropyTresholdLayer(unittest.TestCase):

	def test_call(self):
		# Expects call function to flatten prediction with low conf (test_data[0])
		# and to leave intact prediction with high confidence (test_data[1])
		test_data = np.array([[0.9, 0.05, 0.05], [1, 0, 0]])
		threshold_layer = package_ensemble.EntropyThresholdLayer(
			0.23, test_data.shape[1])
		result = threshold_layer.call(test_data)
		self.assertEqual(np.all(result == [[0.475, 0.2625, 0.2625], [1, 0, 0]]), True)

if __name__ == '__main__':
		unittest.main()