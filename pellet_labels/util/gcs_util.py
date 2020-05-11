from tensorflow.python.lib.io import file_io
import os
import subprocess


def download_file_from_gcs(source, destination):
	if not os.path.exists(destination):
		subprocess.check_call([
				'gsutil',
				'cp',
				source, destination])
	else:
		print('File %s already present locally, not downloading' % destination)

# h5py workaround: copy local models over to GCS if the job_dir is GCS.
def copy_file_to_gcs(local_path, gcs_path):
	with file_io.FileIO(local_path, mode='rb') as input_f:
		with file_io.FileIO(gcs_path, mode='w+') as output_f:
			output_f.write(input_f.read())

def load_models_from_gcs(
	job_dir, model_folder, model_name, working_dir, n_ensemble):
	model_paths = []
	for i in range(1, n_ensemble + 1):
		gcs_path = os.path.join(job_dir, model_folder + str(i), model_name)
		local_path = os.path.join(working_dir,
			model_folder + str(i), model_name)
		download_file_from_gcs(gcs_path, local_path)
		model_paths.append(local_path)
	return model_paths