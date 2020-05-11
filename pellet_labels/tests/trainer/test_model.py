import unittest
import numpy as np
import cv2
from trainer import model

class TestFormatPill(unittest.TestCase):

    def test_bigger(self):
        output = model.format_pill_for_inference(np.ones((5, 5, 3)), 4)
        self.assertEqual(output.shape, (4, 4, 3))

    def test_smaller(self):
        output = model.format_pill_for_inference(np.ones((3, 3, 3)), 4)
        self.assertEqual(output.shape, (4, 4, 3))

    def test_non_square(self):
        output = model.format_pill_for_inference(np.ones((3, 5, 3)), 4)
        self.assertEqual(output.shape, (4, 4, 3))

class TestMapToCommon(unittest.TestCase):

    def test_color(self):
        output = model.map_to_common_space(np.ones((4, 4, 3), dtype=np.uint8))
        self.assertEqual(output.shape, (4, 4))

    def test_not_color(self):
        output = model.map_to_common_space(np.ones((4, 4), dtype=np.uint8))
        self.assertEqual(output.shape, (4, 4))

class TestOversample(unittest.TestCase):

    def test_rare(self):
        train_images = np.ones((10, 5, 5, 1))
        train_labels = np.ones((10, 2))
        # Setting all the classes to label index 0
        train_labels[:,1] = 0
        # Changing the last sample label to index 1 so that we have 9 samples
        # of one class and one sample of another
        train_labels[-1] = [0, 1]
        sample_weights = np.ones(10)
        (images, labels), weights = model.oversample_rare_classes(
            train_images, train_labels, sample_weights, 3)

        # Expect the method to add 2 samples of the second class
        # Check that we have 12 samples now
        self.assertEqual(len(images), 12)
        # Check that the 2 additional samples are for the 2nd class
        self.assertEqual(labels[-2, 1], 1)
        self.assertEqual(labels[-1, 1], 1)
        # Check that the weights have properly been extended to have the same
        # shape as the images and labels
        self.assertEqual(sum(weights), 12)
        self.assertEqual(max(weights), 1)

    def test_no_rare(self):
        train_images = np.ones((10, 5, 5, 1))
        train_labels = np.ones((10, 2))
        train_labels[:,1] = 0
        train_labels[-1] = [0, 1]
        sample_weights = np.ones(10)
        (images, labels), weights = model.oversample_rare_classes(train_images, train_labels,
        sample_weights, 1)

        self.assertEqual(len(images), 10)
        self.assertEqual(labels[-2, 1], 0)
        self.assertEqual(labels[-1, 1], 1)
        self.assertEqual(sum(weights), 10)
        self.assertEqual(max(weights), 1)

if __name__ == '__main__':
    unittest.main()