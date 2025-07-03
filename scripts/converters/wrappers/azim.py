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


class LSAAzimuth(LSAWrapper):
    def __init__(self):
        self.lsawrappers.new_LSAAzimuth.argtypes = []
        self.lsawrappers.new_LSAAzimuth.restype = c.c_void_p

        self.lsawrappers.delete_LSAAzimuth.argtypes = [c.c_void_p]
        self.lsawrappers.delete_LSAAzimuth.restype = None
        
        self.lsawrappers.setAzimRequiredValues_DecDeg.argtypes = [c.c_void_p, c.c_char_p, c.c_char_p, c.c_double, c.c_double, c.c_char_p]
        self.lsawrappers.setAzimRequiredValues_DecDeg.restype = None

        self.lsawrappers.setAzimRequiredValues_DMS.argtypes = [c.c_void_p, c.c_char_p, c.c_char_p, c.c_int, c.c_int, c.c_double, c.c_double, c.c_char_p]
        self.lsawrappers.setAzimRequiredValues_DMS.restype = None

        self.lsawrappers.setAzimHeightFromLabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setAzimHeightFromLabel.restype = None

        self.lsawrappers.setAzimHeightFromValue.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setAzimHeightFromValue.restype = None

        self.lsawrappers.setAzimHeightFromSigma.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setAzimHeightFromSigma.restype = None

        self.lsawrappers.setAzimHeightToLabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setAzimHeightToLabel.restype = None

        self.lsawrappers.setAzimHeightToValue.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setAzimHeightToValue.restype = None

        self.lsawrappers.setAzimHeightToSigma.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setAzimHeightToSigma.restype = None

        self.lsawrappers.setAzimUNCRLabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setAzimUNCRLabel.restype = None

        self.lsawrappers.setAzimIsReduced.argtypes = [c.c_void_p, c.c_bool]
        self.lsawrappers.setAzimIsReduced.restype = None

        self.lsawrappers.setAzimVSCALabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setAzimVSCALabel.restype = None

        self.lsawrappers.setAzimVSCAValue.argtypes = [c.c_void_p, c.c_double]
        self.lsawrappers.setAzimVSCAValue.restype = None

        self.lsawrappers.getAzimString.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.getAzimString.restype = None

        self.obj = c.c_void_p(self.lsawrappers.new_LSAAzimuth())
        self.required_values_are_set = False

    def __del__(self):
        self.lsawrappers.delete_LSAAzimuth(self.obj)

    def set_required_values_decdeg(self, from_point, to_point, angle, sigma, sigma_units):
        """
        :type from_point: str
        :type to_point: str
        :type angle: float
        :type sigma: float
        :type sigma_units: str

        :param from_point: label for the from point
        :param to_point: label for the to point
        :param angle: measured value in decimal degrees
        :param sigma: measurement uncertainty
        :param sigma_units: uncertainty units
        """

        assert isinstance(from_point, str),  "ZAngle from_point must be a str"
        assert from_point != "",             "ZAngle from_point cannot be empty"
        assert isinstance(to_point, str),    "ZAngle to_point must be a str"
        assert to_point != "",               "ZAngle to_point cannot be empty"
        assert isinstance(angle, float),     "ZAngle DecDeg angle must be a float"
        assert isinstance(sigma, float),     "ZAngle sigma must be a float"
        assert isinstance(sigma_units, str), "ZAngle sigma_units must be a str"
        assert sigma_units != "",            "ZAngle sigma_units cannot be empty"

        from_point = from_point.encode('utf-8')
        to_point = to_point.encode('utf-8')
        angle = c.c_double(angle)
        sigma = c.c_double(sigma)
        sigma_units = sigma_units.encode('utf-8')

        self.required_values_are_set = True
        self.lsawrappers.setAzimRequiredValues_DecDeg(self.obj, from_point, to_point, angle, sigma, sigma_units)

    def set_required_values_dms(self, from_point, to_point, degrees, minutes, seconds, sigma, sigma_units):
        assert isinstance(from_point, str),  "ZAngle from_point must be a str"
        assert from_point != "",             "ZAngle from_point cannot be empty"
        assert isinstance(to_point, str),    "ZAngle to_point must be a str"
        assert to_point != "",               "ZAngle to_point cannot be empty"
        assert isinstance(degrees, int),     "ZAngle DMS degrees must be an integer"
        assert isinstance(minutes, int),     "ZAngle DMS minutes must be an integer"
        assert isinstance(seconds, float),   "ZAngle DMS minutes must be a float"
        assert isinstance(sigma, float),     "ZAngle sigma must be a float"
        assert isinstance(sigma_units, str), "ZAngle sigma_units must be a str"
        assert sigma_units != "",            "ZAngle sigma_units cannot be empty"

        from_point = from_point.encode('utf-8')
        to_point = to_point.encode('utf-8')
        degrees =  c.c_int(degrees)
        minutes =  c.c_int(minutes)
        seconds =  c.c_double(seconds)
        sigma = c.c_double(sigma)
        sigma_units = sigma_units.encode('utf-8')

        self.required_values_are_set = True
        self.lsawrappers.setAzimRequiredValues_DMS(self.obj, from_point, to_point,
                                                   degrees, minutes, seconds, sigma, sigma_units)

    def set_from_height_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"

        self.lsawrappers.setAzimHeightFromLabel(self.obj, label.encode('utf-8'))

    def set_from_height_value(self, value, units, heightSigma, heightSigmaUnits):
        assert isinstance(value, float),            "value must be a float"
        assert isinstance(units, str),              "units must be a str"
        assert units != "",                         "units cannot be empty"
        assert isinstance(heightSigma, float),      "height sigma must be a float"
        assert isinstance(heightSigmaUnits, str),   "height sigma units must be a str"

        value = c.c_double(value)
        units = units.encode('utf-8')
        heightSigma = c.c_double(heightSigma)
        heightSigmaUnits = heightSigmaUnits.encode('utf-8')

        self.lsawrappers.setAzimHeightFromValue(self.obj, value, units)
        self.lsawrappers.setAzimHeightFromSigma(self.obj, heightSigma, heightSigmaUnits)

    def set_to_height_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setAzimHeightToLabel(self.obj, label.encode('utf-8'))

    def set_to_height_value(self, value, units, heightSigma, heightSigmaUnits):
        assert isinstance(value, float),            "value must be a float"
        assert isinstance(units, str),              "units must be a str"
        assert units != "",                         "units cannot be empty"
        assert isinstance(heightSigma, float),      "height sigma must be a float"
        assert isinstance(heightSigmaUnits, str),   "height sigma units must be a str"

        value = c.c_double(value)
        units = units.encode('utf-8')
        heightSigma = c.c_double(heightSigma)
        heightSigmaUnits = heightSigmaUnits.encode('utf-8')

        self.lsawrappers.setAzimHeightToValue(self.obj, value, units)
        self.lsawrappers.setAzimHeightToSigma(self.obj, heightSigma, heightSigmaUnits)

    def set_uncr_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setAzimUNCRLabel(self.obj, label.encode('utf-8'))

    def set_is_reduced(self, is_reduced):
        assert isinstance(is_reduced, bool), "is_reduced must be a bool"
        value = c.c_bool(is_reduced)
        self.lsawrappers.setAzimIsReduced(self.obj, is_reduced)

    def set_vsca_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setAzimVSCALabel(self.obj, label.encode('utf-8'))

    def set_vsca_value(self, value):
        assert isinstance(value, float), "value must be a float"
        value = c.c_double(value)
        self.lsawrappers.setAzimVSCAValue(self.obj, value)

    def get_lsa_string(self):
        assert(self.required_values_are_set == True), "get_lsa_string called before required values set"
        buffer = c.create_string_buffer(200)
        self.lsawrappers.getAzimString(self.obj, buffer)
        return buffer.value.decode('utf-8')
