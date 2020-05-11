# Training antibiotic recognition model

On the petri dish, several antibiotic pellets are positioned for test. Each of
them is marked with a code that identifies the molecule and concentration. The
goal of this model is to identify which pellet corresponds to which antibiotic.

## Dependencies

To run scripts locally, you can install the python dependencies with the
following command

> `pip3 install -r requirements.txt`

## Run locally

Expected way to run the script is through the following command

> `python3 -m trainer.task --job-dir ./models --train-files ./data/i2a_atb_data.zip ./data/amman_atb_data.zip --num-epochs 120 --weights 1 3`

## Arguments

### Training and validation data folder

The "--train-files" arguments are the zip files where the training data is
located. You can list several folders. Each zip file should contain a top level
folder named after the zip file,then one train and one valid folder each
containing one folder per class (folder named after the class), itself
containing all the samples for that class.

Classes should match the PELLET_LIST in pellet_list.py

### Number of epochs

Introduced with '--num-epochs', defines the number of epochs to train the model
for.

### Output location

Introduced with '--job-dir', defines the folder for the model as well as
tensorboard files. Model is a .h5 (Keras format).

### Sample weights

Weights can be assigned to each training set. They're introduced with
'--weights'. There should be one weight value per training data set.

## Run on gcloud

Using gcloud will require installing the Google cloud SDK and login into a
Google account with the proper API access.
[More details](https://cloud.google.com/ml-engine/docs/tensorflow/getting-started-keras)

### Testing gcloud setup locally

Use the following command

> `gcloud ai-platform local train --package-path trainer --module-name trainer.task --job-dir models -- --train-files ./data/test_data.zip --num-epochs=1`

### Running on gcloud proper

Use the following command after population the variables JOB_NAME, JOB_DIR and
REGION (preferrably europe-west1). The command assume a specific location and
project (pellet-labels-260211) that are set up for the MSF Google Cloud account.
Note that JOB_NAME needs to be unique for every run.

> `JOB_NAME=train_run REGION=europe-west1 JOB_DIR=gs://pellet_labels/${JOB_NAME}/pellet_labels_model && gcloud ai-platform jobs submit training $JOB_NAME --package-path trainer/ --module-name trainer.task --region $REGION --scale-tier BASIC_GPU --python-version 3.5 --runtime-version 1.13 --job-dir $JOB_DIR --stream-logs -- --train-files gs://pellet_labels/amman_atb_data.zip gs://pellet_labels/i2a_atb_data.zip --weights 3 1`

## Infer

You can use the trained TF Lite model to infer the label on a pellet image using
the infer.py script:

> `python3 infer.py img.jpg`

Or you can test the Keras model by adding `--model keras`.

## Test the model

After training it on the cloud, check everything looks good by evaluating it
against the valid data set with:

> `python3 test_model.py --data-files ./data/amman_atb_data.zip ./data/i2a_atb_data.zip`

## Convert model to tflite

You can convert a keras model (.h5) to tflite with the following command

> `python3 tflite_converter.py --source models/ab_recognition_i2a_and_amman_data.h5 --destination models/ab_recognition_i2a_and_amman_data.tflite`

## Run tests

> `python3 -m unittest`
