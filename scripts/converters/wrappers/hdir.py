'''
	Copyright (c) 2015-2024 Applied Research Laboratories, The University of Texas
	at Austin (ARL:UT).
	
	SALSA is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License version 3 (GPL-3.0-only) as published
	by the Free Software Foundation.
	
	SALSA is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License along with
	SALSA.  If not, see https://www.gnu.org/licenses/.
'''
import ctypes as c
from wrappers.lsawrapper import LSAWrapper


class LSAHDir(LSAWrapper):
    def __init__(self):
        self.lsawrappers.new_LSAHDir.argtypes = []
        self.lsawrappers.new_LSAHDir.restype = c.c_void_p

        self.lsawrappers.delete_LSAHDir.argtypes = [c.c_void_p]
        self.lsawrappers.delete_LSAHDir.restype = None

        self.lsawrappers.setHDirRequiredValues_DecDeg.argtypes = [c.c_void_p, c.c_char_p, c.c_char_p, c.c_double,
                                                                  c.c_double, c.c_char_p]
        self.lsawrappers.setHDirRequiredValues_DecDeg.restype = None

        self.lsawrappers.setHDirRequiredValues_DMS.argtypes = [c.c_void_p, c.c_char_p, c.c_char_p, c.c_int,
                                                                  c.c_int, c.c_double, c.c_double, c.c_char_p]
        self.lsawrappers.setHDirRequiredValues_DMS.restype = None

        self.lsawrappers.setHDirHeightToLabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setHDirHeightToLabel.restype = None

        self.lsawrappers.setHDirHeightToValue.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setHDirHeightToValue.restype = None

        self.lsawrappers.setHDirHeightToSigma.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setHDirHeightToSigma.restype = None

        self.lsawrappers.getHDirString.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.getHDirString.restype = None

        self.obj = c.c_void_p(self.lsawrappers.new_LSAHDir())
        self.required_values_are_set = False

    def __del__(self):
        self.lsawrappers.delete_LSAHDir(self.obj)

    def set_required_values_decdeg(self, dgrp_label, to_point, angle, sigma, sigma_units):
        """
        :type dgrp_label: str
        :type to_point: str
        :type angle: float
        :type sigma: float
        :type sigma_units: str

        :param dgrp_label: label for DGRP record
        :param to_point: label for the to point
        :param angle: measured value in decimal degrees
        :param sigma: measurement uncertainty
        :param sigma_units: uncertainty units
        """

        assert isinstance(dgrp_label, str),  "HDir from_point must be a str"
        assert dgrp_label != "",             "HDir from_point cannot be empty"
        assert isinstance(to_point, str),    "HDir to_point must be a str"
        assert to_point != "",               "HDir to_point cannot be empty"
        assert isinstance(angle, float),     "HDir DecDeg angle must be a float"
        assert isinstance(sigma, float),     "HDir sigma must be a float"
        assert isinstance(sigma_units, str), "HDir sigma_units must be a str"
        assert sigma_units != "",            "HDir sigma_units cannot be empty"

        dgrp_label = dgrp_label.encode('utf-8')
        to_point = to_point.encode('utf-8')
        angle = c.c_double(angle)
        sigma = c.c_double(sigma)
        sigma_units = sigma_units.encode('utf-8')

        self.required_values_are_set = True
        self.lsawrappers.setHDirRequiredValues_DecDeg(self.obj, dgrp_label, to_point, angle, sigma, sigma_units)

    def set_required_values_dms(self, dgrp_label, to_point, degrees, minutes, seconds, sigma, sigma_units):
        assert isinstance(dgrp_label, str),  "HDir dgrp_label must be a str"
        assert dgrp_label != "",             "HDir dgrp_label cannot be empty"
        assert isinstance(to_point, str),    "HDir to_point must be a str"
        assert to_point != "",               "HDir to_point cannot be empty"
        assert isinstance(degrees, int),     "HDir DMS degrees must be an integer"
        assert isinstance(minutes, int),     "HDir DMS minutes must be an integer"
        assert isinstance(seconds, float),   "HDir DMS minutes must be a float"
        assert isinstance(sigma, float),     "HDir sigma must be a float"
        assert isinstance(sigma_units, str), "HDir sigma_units must be a str"
        assert sigma_units != "",            "HDir sigma_units cannot be empty"

        dgrp_label = dgrp_label.encode('utf-8')
        to_point = to_point.encode('utf-8')
        degrees =  c.c_int(degrees)
        minutes =  c.c_int(minutes)
        seconds =  c.c_double(seconds)
        sigma = c.c_double(sigma)
        sigma_units = sigma_units.encode('utf-8')

        self.required_values_are_set = True
        self.lsawrappers.setHDirRequiredValues_DMS(self.obj, dgrp_label, to_point,
                                                   degrees, minutes, seconds, sigma, sigma_units)

    def set_to_height_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setHDirHeightToLabel(self.obj, label.encode('utf-8'))

    def set_to_height_value(self, value, units, heightSigma, heightSigmaUnits):
        assert isinstance(value, float),            "value must be a float"
        assert isinstance(units, str),              "units must be a str"
        assert units != "",                         "units cannot be empty"
        assert isinstance(heightSigma, float),      "heightSigma must be a float"
        assert isinstance(heightSigmaUnits, str),   "heightSigmaUnits must be a str"
        value = c.c_double(value)
        units = units.encode('utf-8')
        heightSigma = c.c_double(heightSigma)
        heightSigmaUnits = heightSigmaUnits.encode('utf-8')
        self.lsawrappers.setHDirHeightToValue(self.obj, value, units)
        self.lsawrappers.setHDirHeightToSigma(self.obj, heightSigma, heightSigmaUnits)

    def get_lsa_string(self):
        assert(self.required_values_are_set == True), "get_lsa_string called before required values set"
        buffer = c.create_string_buffer(200)
        self.lsawrappers.getHDirString(self.obj, buffer)
        return buffer.value.decode('utf-8')
