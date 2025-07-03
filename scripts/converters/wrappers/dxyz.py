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

class LSADelta(LSAWrapper):
    def __init__(self, from_point, to_point, dx, dy, dz, units, cxx, cxy, cxz, cyy, cyz, czz):
        """Encapsulate a LSADelta object

        Required Arguments:
        fromPoint(str)   -- the from point of the dist measurement
        toPoint  (str)   -- the to point of the dist measurement
        distance (float) -- the distance between the points
        sigma    (float) -- sigma (uncertainty) of the distance measurement
        linUnits (str)   -- the units of distance and sigma
        """

        # Validate parameters
        assert isinstance(from_point, str), "fromPoint must be a str"
        assert from_point != "",            "fromPoint cannot be empty"
        assert isinstance(to_point, str),   "toPoint must be a str"
        assert to_point != "",              "toPoint cannot be empty"
        assert isinstance(dx, float),       "dx must be a float"
        assert isinstance(dy, float),       "dy must be a float"
        assert isinstance(dz, float),       "dz must be a float"
        assert isinstance(units, str),   "toPoint must be a str"
        assert units != "",              "toPoint cannot be empty"

        
        assert isinstance(cxx, float),      "dx must be a float"
        assert isinstance(cxy, float),      "cxy must be a float"
        assert isinstance(cxz, float),      "cxz must be a float"
        assert isinstance(cyy, float),      "cyy must be a float"
        assert isinstance(cyz, float),      "cyz must be a float"
        assert isinstance(czz, float),      "czz must be a float"


        from_point = from_point.encode('utf-8')
        to_point = to_point.encode('utf-8')
        dx = c.c_double(dx)
        dy = c.c_double(dy)
        dz = c.c_double(dz)
        units = units.encode('utf-8')
        cxx = c.c_double(cxx)
        cxy = c.c_double(cxy)
        cxz = c.c_double(cxz)
        cyy = c.c_double(cyy)
        cyz = c.c_double(cyz)
        czz = c.c_double(czz)

        self.lsawrappers.new_LSADelta.argtypes = [c.c_char_p, c.c_char_p, c.c_double, c.c_double, c.c_double,
                                                  c.c_char_p, c.c_double, c.c_double, c.c_double,
                                                  c.c_double, c.c_double, c.c_double]
        self.lsawrappers.new_LSADelta.restype = c.c_void_p

        self.lsawrappers.delete_LSADelta.argtypes = [c.c_void_p]
        self.lsawrappers.delete_LSADelta.restype = None

        self.lsawrappers.setDeltaHeightFromLabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setDeltaHeightFromLabel.restype = None

        self.lsawrappers.setDeltaHeightFromValue.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setDeltaHeightFromValue.restype = None

        self.lsawrappers.setDeltaHeightFromSigma.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setDeltaHeightFromSigma.restype = None

        self.lsawrappers.setDeltaHeightToLabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setDeltaHeightToLabel.restype = None

        self.lsawrappers.setDeltaHeightToValue.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setDeltaHeightToValue.restype = None

        self.lsawrappers.setDeltaHeightToSigma.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setDeltaHeightToSigma.restype = None

        self.lsawrappers.setDeltaUNCRLabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setDeltaUNCRLabel.restype = None

        self.lsawrappers.setDeltaVSCALabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setDeltaVSCALabel.restype = None

        self.lsawrappers.setDeltaVSCAValue.argtypes = [c.c_void_p, c.c_double]
        self.lsawrappers.setDeltaVSCAValue.restype = None

        self.lsawrappers.getLSADeltaString.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.getLSADeltaString.restype = None
        
        self.lsawrappers.setConversionNotes.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setConversionNotes.restype = None

        self.obj = c.c_void_p(self.lsawrappers.new_LSADelta(from_point, to_point, dx, dy, dz, units,
                                                 cxx, cxy, cxz, cyy, cyz, czz))

    def __del__(self):
        self.lsawrappers.delete_LSADelta(self.obj)

    def set_from_height_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setDeltaHeightFromLabel(self.obj, label.encode('utf-8'))

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
        self.lsawrappers.setDeltaHeightFromValue(self.obj, value, units)
        self.lsawrappers.setDeltaHeightFromSigma(self.obj, heightSigma, heightSigmaUnits)

    def set_to_height_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setDeltaHeightToLabel(self.obj, label.encode('utf-8'))

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
        self.lsawrappers.setDeltaHeightToValue(self.obj, value, units)
        self.lsawrappers.setDeltaHeightToSigma(self.obj, heightSigma, heightSigmaUnits)

    def set_uncr_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setDeltaUNCRLabel(self.obj, label.encode('utf-8'))

    def set_vsca_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setDeltaVSCALabel(self.obj, label.encode('utf-8'))

    def set_vsca_value(self, value):
        assert isinstance(value, float), "value must be a float"
        value = c.c_double(value)
        self.lsawrappers.setDeltaVSCAValue(self.obj, value)

    def get_lsa_string(self):
        buffer = c.create_string_buffer(400)
        self.lsawrappers.getLSADeltaString(self.obj, buffer)
        return buffer.value.decode('utf-8')
    
    def set_date_time(self, start_date, end_date, start_time, end_time):
        assert isinstance(start_date, str)
        assert isinstance(start_time, str)
        assert isinstance(end_date, str)
        assert isinstance(end_time, str)
        time_info = "Collection Window: {sd} {st} - {ed} {et} (UTC)\r".format(sd=start_date.replace(" ", "-"), st=start_time.replace(" ", ":"), ed=end_date.replace(" ", "-"), et=end_time.replace(" ", ":"))
        self.lsawrappers.setConversionNotes(self.obj, time_info.encode('utf-8'))
