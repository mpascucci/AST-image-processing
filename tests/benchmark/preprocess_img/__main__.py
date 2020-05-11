import os, argparse
import astimp
from .preproc import preprocess, Dataset

PICKLE_PATH = "pickled_preprocs"

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("input_folder", help="The images folder")

    parser.add_argument('-o', '--output-folder', help="The folder where the "
                        "pickles will be stored. Defaults to {}".format(
                            PICKLE_PATH),
                        required=False)

    parser.add_argument('-s', '--diamter-sensibility', help="The sensibility in diameter reading in (0,1) "
                        "pickles will be stored. Defaults to {}".format(
                            astimp.config.Inhibition_diameterReadingSensibility),
                        required=False)

    args = parser.parse_args()
    print()

    ds = Dataset(args.input_folder)

    if args.output_folder is not None:
        PICKLE_PATH = args.output_folder

    if args.diamter_sensibility is not None:
        astimp.config.Inhibition_diameterReadingSensibility = float(args.diamter_sensibility)

    print("Preprocessing results will be stored in {}".format(PICKLE_PATH))
    
    errors = preprocess(ds.paths, PICKLE_PATH, parallel=True)

    if errors:
        print("Impossible to process {} files".format(len(errors)))

        # save filenames of error images
        with open(os.path.join(PICKLE_PATH, 'error_log.txt'), 'w') as f:
            f.write('\n'.join(errors))
            print("log saved in in {}/error_log.txt".format(PICKLE_PATH))
