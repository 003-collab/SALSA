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


class LSAPosG(LSAWrapper):
    def __init__(self, label):
        assert isinstance(label, str),  "ZAngle from_point must be a str"
        assert label != "",             "ZAngle from_point cannot be empty"

        self.lsawrappers.new_LSAPosG.argtypes = [c.c_char_p]
        self.lsawrappers.new_LSAPosG.restype = c.c_void_p

        self.lsawrappers.delete_LSAPosG.argtypes = [c.c_void_p]
        self.lsawrappers.delete_LSAPosG.restype = None

        self.lsawrappers.setPosGRequiredValues_DecDeg.argtypes = [c.c_void_p, c.c_double, c.c_char_p, c.c_double, c.c_char_p,
                                                                  c.c_double, c.c_char_p]
        self.lsawrappers.setPosGRequiredValues_DecDeg.restype = None

        self.lsawrappers.setPosGRequiredValues_DMS.argtypes = [c.c_void_p, c.c_int, c.c_int, c.c_double, c.c_char_p,
                                                                           c.c_int, c.c_int, c.c_double, c.c_char_p,
                                                                           c.c_double, c.c_char_p]
        self.lsawrappers.setPosGRequiredValues_DMS.restype = None

        self.lsawrappers.setPosGCovariance.argtypes = [c.c_void_p, c.c_double, c.c_double, c.c_double, c.c_double,
                                                       c.c_double, c.c_double]
        self.lsawrappers.setPosGCovariance.restype = None

        self.lsawrappers.setPosGVSCALabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setPosGVSCALabel.restype = None

        self.lsawrappers.setPosGVSCAValue.argtypes = [c.c_void_p, c.c_double]
        self.lsawrappers.setPosGVSCAValue.restype = None

        self.lsawrappers.getPosGString.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.getPosGString.restype = None

        label = label.encode('utf-8')
        self.obj = c.c_void_p(self.lsawrappers.new_LSAPosG(label))
        self.required_values_are_set = False

    def __del__(self):
        self.lsawrappers.delete_LSAPosG(self.obj)

    def set_required_values_decdeg(self, latDecDeg, latDir, lonDecDeg, lonDir, height, heightUnits):
        assert isinstance(latDecDeg, float), "LatDecDeg angle must be a float"
        assert isinstance(latDir, str),      "LatDir must be a str"
        assert latDir != "",                 "LatDir from_point cannot be empty"
        
        assert isinstance(lonDecDeg, float), "LonDecDeg must be a float"
        assert isinstance(lonDir, str),      "lonDir must be a str"
        assert lonDir != "",                 "lonDir to_point cannot be empty"
                
        assert isinstance(height, float),    "height must be a float"
        assert isinstance(heightUnits, str), "heightUnits sigma_units must be a str"
        assert heightUnits != "",            "heightUnits cannot be empty"

        latDecDeg = c.c_double(latDecDeg)
        latDir = latDir.encode('utf-8')
        lonDecDeg = c.c_double(lonDecDeg)
        lonDir = lonDir.encode('utf-8')
        height = c.c_double(height)
        heightUnits = heightUnits.encode('utf-8')

        self.required_values_are_set = True
        self.lsawrappers.setPosGRequiredValues_DecDeg(self.obj, latDecDeg, latDir, lonDecDeg, lonDir, height, heightUnits)

    def set_required_values_dms(self, latDeg, latMin, latSec, latDir, lonDeg, lonMin, lonSec, lonDir, height, heightUnits):
        assert isinstance(latDeg, int),   "latDeg must be an int"
        assert isinstance(latMin, int),   "latMin must be an int"
        assert isinstance(latSec, float), "latSec must be a float"
        assert isinstance(latDir, str),   "LatDir must be a str"
        assert latDir != "",              "LatDir cannot be empty"
        
        assert isinstance(lonDeg, int),   "lonDeg must be an int"
        assert isinstance(lonMin, int),   "lonMin must be an int"
        assert isinstance(lonSec, float), "lonSec must be a float"
        assert isinstance(lonDir, str),   "LatDir must be a str"
        assert lonDir != "",              "LatDir cannot be empty"
                
        assert isinstance(height, float),    "height must be a float"
        assert isinstance(heightUnits, str), "heightUnits sigma_units must be a str"
        assert heightUnits != "",            "heightUnits cannot be empty"

        latDeg = c.c_int(latDeg)
        latMin = c.c_int(latMin)
        latSec = c.c_double(latSec)
        latDir = latDir.encode('utf-8')
        
        lonDeg = c.c_int(lonDeg)
        lonMin = c.c_int(lonMin)
        lonSec = c.c_double(lonSec)
        lonDir = lonDir.encode('utf-8')
        
        height = c.c_double(height)
        heightUnits = heightUnits.encode('utf-8')

        self.required_values_are_set = True
        self.lsawrappers.setPosGRequiredValues_DMS(self.obj, latDeg, latMin, latSec, latDir,
                                                             lonDeg, lonMin, lonSec, lonDir, height, heightUnits)

    def set_covariance(self, cnn, cne, cnu, cee, ceu, cuu):
        assert isinstance(cnn, float),      "dx must be a float"
        assert isinstance(cne, float),      "cne must be a float"
        assert isinstance(cnu, float),      "cnu must be a float"
        assert isinstance(cee, float),      "cee must be a float"
        assert isinstance(ceu, float),      "ceu must be a float"
        assert isinstance(cuu, float),      "cuu must be a float"
        cnn = c.c_double(cnn)
        cne = c.c_double(cne)
        cnu = c.c_double(cnu)
        cee = c.c_double(cee)
        ceu = c.c_double(ceu)
        cuu = c.c_double(cuu)

        self.lsawrappers.setPosGCovariance(self.obj, cnn, cne, cnu, cee, ceu, cuu);

    def set_vsca_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setPosGVSCALabel(self.obj, label.encode('utf-8'))

    def set_vsca_value(self, value):
        assert isinstance(value, float), "value must be a float"
        value = c.c_double(value)
        self.lsawrappers.setPosGVSCAValue(self.obj, value)

    def set_state_constrained(self):
        self.lsawrappers.setPosGStateConstrained(self.obj)

    def set_state_fixed(self):
        self.lsawrappers.setPosGStateFixed(self.obj)

    def set_state_floating(self):
        self.lsawrappers.setPosGStateFloating(self.obj)

    def get_lsa_string(self):
        assert(self.required_values_are_set == True), "get_lsa_string called before required values set"
        buffer = c.create_string_buffer(200)
        self.lsawrappers.getPosGString(self.obj, buffer)
        return buffer.value.decode('utf-8')
