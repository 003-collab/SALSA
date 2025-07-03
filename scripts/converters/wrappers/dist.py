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

class LSADist(LSAWrapper):
    def __init__(self, from_point, to_point, distance, sigma, lin_units):
        """Encapsulate a LSADist object

        Required Arguments:
        fromPoint(str)   -- the from point of the dist measurement
        toPoint  (str)   -- the to point of the dist measurement
        distance (float) -- the distance between the points
        sigma    (float) -- sigma (uncertainty) of the distance measurement
        linUnits (str)   -- the units of distance and sigma
        """

        # Validate parameters
        assert isinstance(from_point, str), "DIST fromPoint must be a str"
        assert from_point != "", "DIST fromPoint cannot be empty"
        assert isinstance(to_point, str), "DIST toPoint must be a str"
        assert to_point != "", "DIST toPoint cannot be empty"
        assert isinstance(distance, float), "DIST distance must be a float"
        assert isinstance(sigma, float),    "DIST sigma must be a float"
        assert isinstance(lin_units, str), "Dist linUnits must be a str"
        assert lin_units != "", "Dist linUnits cannot be empty"

        from_point = from_point.encode('utf-8')
        to_point = to_point.encode('utf-8')
        distance = c.c_double(distance)
        sigma = c.c_double(sigma)
        lin_units = lin_units.encode('utf-8')

        self.lsawrappers.new_LSADist.argtypes = [c.c_char_p, c.c_char_p, c.c_double, c.c_double, c.c_char_p]
        self.lsawrappers.new_LSADist.restype = c.c_void_p

        self.lsawrappers.delete_LSADist.argtypes = [c.c_void_p]
        self.lsawrappers.delete_LSADist.restype = None

        self.lsawrappers.setDistHeightFromLabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setDistHeightFromLabel.restype = None

        self.lsawrappers.setDistHeightFromValue.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setDistHeightFromValue.restype = None

        self.lsawrappers.setDistHeightFromSigma.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setDistHeightFromSigma.restype = None

        self.lsawrappers.setDistHeightToLabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setDistHeightToLabel.restype = None

        self.lsawrappers.setDistHeightToValue.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setDistHeightToValue.restype = None

        self.lsawrappers.setDistHeightToSigma.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setDistHeightToSigma.restype = None

        self.lsawrappers.setDistUNCRLabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setDistUNCRLabel.restype = None

        self.lsawrappers.setDistVSCALabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setDistVSCALabel.restype = None

        self.lsawrappers.setDistVSCAValue.argtypes = [c.c_void_p, c.c_double]
        self.lsawrappers.setDistVSCAValue.restype = None

        self.lsawrappers.getLSADistString.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.getLSADistString.restype = None

        self.obj = c.c_void_p(self.lsawrappers.new_LSADist(from_point, to_point, distance, sigma, lin_units))

    def __del__(self):
        self.lsawrappers.delete_LSADist(self.obj)

    def set_from_height_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setDistHeightFromLabel(self.obj, label.encode('utf-8'))

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
        self.lsawrappers.setDistHeightFromValue(self.obj, value, units)
        self.lsawrappers.setDistHeightFromSigma(self.obj, heightSigma, heightSigmaUnits)


    def set_to_height_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setDistHeightToLabel(self.obj, label.encode('utf-8'))

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
        self.lsawrappers.setDistHeightToValue(self.obj, value, units)
        self.lsawrappers.setDistHeightToSigma(self.obj, heightSigma, heightSigmaUnits)

    def set_uncr_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setDistUNCRLabel(self.obj, label.encode('utf-8'))

    def set_vsca_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setDistVSCALabel(self.obj, label.encode('utf-8'))

    def set_vsca_value(self, value):
        assert isinstance(value, float), "value must be a float"
        value = c.c_double(value)
        self.lsawrappers.setDistVSCAValue(self.obj, value)

    def get_lsa_string(self):
        buffer = c.create_string_buffer(200)
        self.lsawrappers.getLSADistString(self.obj, buffer)
        return buffer.value.decode('utf-8')
