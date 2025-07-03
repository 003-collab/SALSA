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

class LSAHDif(LSAWrapper):
    def __init__(self, from_point, to_point, height_dif, sigma, lin_units):
        """Encapsulate a LSADist object

        Required Arguments:
        from_point(str)   -- the from point of the dist measurement
        to_point  (str)   -- the to point of the dist measurement
        height_dif (float) -- the height difference between the points
        sigma    (float) -- sigma (uncertainty) of the distance measurement
        lin_units (str)   -- the units of distance and sigma
        """

        # Validate parameters
        assert isinstance(from_point, str), "DIST fromPoint must be a str"
        assert from_point != "", "DIST fromPoint cannot be empty"
        assert isinstance(to_point, str), "DIST toPoint must be a str"
        assert to_point != "", "DIST toPoint cannot be empty"
        assert isinstance(height_dif, float), "DIST distance must be a float"
        assert isinstance(sigma, float),    "DIST sigma must be a float"
        assert isinstance(lin_units, str), "Dist linUnits must be a str"
        assert lin_units != "", "Dist linUnits cannot be empty"

        self.lsawrappers.new_LSAHeightDiff.argtypes = [c.c_char_p, c.c_char_p, c.c_double, c.c_double, c.c_char_p]
        self.lsawrappers.new_LSAHeightDiff.restype = c.c_void_p

        self.lsawrappers.delete_LSAHeightDiff.argtypes = [c.c_void_p]
        self.lsawrappers.delete_LSAHeightDiff.restype = c.c_void_p

        self.lsawrappers.setHDifUNCRLabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setHDifUNCRLabel.restype = None

        self.lsawrappers.setHDifRefraction.argtypes = [c.c_void_p, c.c_double]
        self.lsawrappers.setHDifRefraction.restype = None

        self.lsawrappers.setHDifIsReduced.argtypes = [c.c_void_p, c.c_bool]
        self.lsawrappers.setHDifIsReduced.restype = None

        self.lsawrappers.setHDifVSCALabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setHDifVSCALabel.restype = None

        self.lsawrappers.setHDifVSCAValue.argtypes = [c.c_void_p, c.c_double]
        self.lsawrappers.setHDifVSCAValue.restype = None

        self.lsawrappers.setHDifCurvCorr.argtypes = [c.c_void_p, c.c_bool]
        self.lsawrappers.setHDifCurvCorr.restype = None

        self.lsawrappers.getHDifString.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.getHDifString.restype = None

        from_point = from_point.encode('utf-8')
        to_point = to_point.encode('utf-8')
        height_dif = c.c_double(height_dif)
        sigma = c.c_double(sigma)
        lin_units = lin_units.encode('utf-8')

        self.obj = c.c_void_p(self.lsawrappers.new_LSAHeightDiff(from_point, to_point, height_dif, sigma, lin_units))


    def __del__(self):
        self.lsawrappers.delete_LSAHeightDiff(self.obj)

    def set_uncr_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setHDifUNCRLabel(self.obj, label.encode('utf-8'))

    def set_refract_coeff(self, value):
        assert isinstance(value, float), "value must be a float"
        value = c.c_double(value)
        self.lsawrappers.setHDifRefraction(self.obj, value)

    def set_is_reduced(self, is_reduced):
        assert isinstance(is_reduced, bool), "is_reduced must be a bool"
        value = c.c_bool(is_reduced)
        self.lsawrappers.setHDifIsReduced(self.obj, is_reduced)

    def set_vsca_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setHDifVSCALabel(self.obj, label.encode('utf-8'))

    def set_vsca_value(self, value):
        assert isinstance(value, float), "value must be a float"
        value = c.c_double(value)
        self.lsawrappers.setHDifVSCAValue(self.obj, value)

    def set_curv_corr(self, is_curv_corr):
        assert isinstance(is_curv_corr, bool), "is_curv_corr must be a bool"
        value = c.c_bool(is_curv_corr)
        self.lsawrappers.setHDifCurvCorr(self.obj, is_curv_corr)

    def get_lsa_string(self):
        buffer = c.create_string_buffer(200)
        self.lsawrappers.getHDifString(self.obj, buffer)
        return buffer.value.decode('utf-8')
