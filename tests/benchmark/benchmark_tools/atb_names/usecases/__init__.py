from ..interfaces import AtbNamesTranslator
import os, csv


class CSVBasedAtbNamesTranslator(AtbNamesTranslator):
    """Use a csv based file which has two columns: "short_name; full_name"""

    def __init__(self, acronym_file_path):
        with open(acronym_file_path, 'r') as csvfile:
            csv_reader = csv.DictReader(csvfile, delimiter=';')

            full_names = []
            short_names = []
            whonet_codes = []
            concentrations = []

            for row in csv_reader:
                full_names.append(row['full_name'])
                short_names.append(row['short_name'])
                whonet_codes.append(row['whonet_code'])
                concentrations.append(row['concentration'])

        super().__init__(full_names, short_names, whonet_codes, concentrations)


# ------------------------------------ i2a ----------------------------------- #
i2a = CSVBasedAtbNamesTranslator(os.path.join("atb_names", "i2a.csv"))

# ----------------------------------- amman ---------------------------------- #
amman = CSVBasedAtbNamesTranslator(os.path.join("atb_names", "amman.csv"))

# ----------------------------------- amman ---------------------------------- #
whonet = CSVBasedAtbNamesTranslator(os.path.join("atb_names", "whonet.csv"))
