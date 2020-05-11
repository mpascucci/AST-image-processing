from abc import ABC
from collections import namedtuple

WhonetAndConcentration = namedtuple("WHOnet_atb", ["code", "concentration"])


class AtbNamesTranslator(ABC):
    def __init__(self, full_names, short_names, whonet_codes, concentrations):
        assert len(short_names) == len(
            full_names), "names list must have same length"
        assert all([name not in full_names for name in short_names]) and all(
            [name not in short_names for name in full_names]), "the parameter lists should have void interception"
        self.full_names = full_names
        self.short_names = short_names
        self.whonet_codes = whonet_codes
        self.concentrations = concentrations

    def full2short(self, full_name):
        """Translate a full antibiotic name into its short version"""
        return self.short_names[self.full_names.index(full_name)]

    def short2full(self, short_name):
        """Translate a short antibiotic name into its long version"""
        return self.full_names[self.short_names.index(short_name)]

    def short2whonet_code(self, short_name):
        idx = self.short_names.index(short_name)
        return self.whonet_codes[idx]

    def short2whonet(self, short_name):
        idx = self.short_names.index(short_name)
        return "{}{}".format(self.whonet_codes[idx], self.concentrations[idx])

    def short2concentration(self, short_name):
        idx = self.short_names.index(short_name)
        return self.concentrations.index(short_name)

    def short2whonet_tuple(self, short_name):
        idx = self.short_names.index(short_name)
        return WhonetAndConcentration(self.whonet_codes[idx], self.concentrations[idx])

    def whonet_code2short(self, whonet_code):
        """Translate a WHOnet antibiotic code its i2a short version"""
        idx = self.whonet_codes.index(whonet_code)
        return self.short_names[idx]
