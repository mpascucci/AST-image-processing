# Training an ensemble model

The initial model showed good accuracy but a max probability threshold wasn't
enough to weed out out-of-distribution data. We developed an ensemble model with
an entropy threshold to achieve a better trade-off between in-distribution
accuracy and out-of-distribution misclassification.

## Training script

To train multiple models in parallel on gcloud, run

> `. ./train_ensemble.sh`

Modify the script to change the parameters you'd pass to trainer/task.py, the
destination of the resulting model or the gcloud configuration.

## Evaluate the ensemble model

eval_ensemble.py lets you load ensemble models from Google Storage and evaluate
their performance on out-of-distribution data (that assumes you trained models
with a shortened list of classes with the --remove-class parameter). You can
evaluate the ensemble with:

> `python3 eval_ensemble.py --job-dir gs://pellet_labels/uncertainty_pellet_labels7 --train-files ./data/amman_atb_data.zip ./data/i2a_atb_data.zip --threshold entropy --destination-file uncertainty_data_n10_entropy.pickle`

The script outputs a pandas dataframe with the in-distribution accuracy and
out-of-distribution misclassification data. ensemble_evaluation.ipynb is a
simple notebook to load and visualize that evaluation data (potentially
comparing different runs of evaluation)

## Package the ensemble model

package_ensemble.py lets you load ensemble models from Google Storage and
repackage them into a single graph (more performant) with an additional final
layer that implements an entropy threshold (currently the best result reached
through multiple evaluation). You can do so with:

> `python3 package_ensemble.py --job-dir gs://pellet_labels/pellet_labels_63_classes1`

## Test the ensemble model

After packaging it, check everything looks good by evaluating it against the
valid data set with:

> `python3 test_model.py --data-files ./data/amman_atb_data.zip ./data/i2a_atb_data.zip`
