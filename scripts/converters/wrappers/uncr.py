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


class LSAUncr(LSAWrapper):
    def __init__(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        label =label.encode('utf-8')

        self.lsawrappers.new_LSAUncr.argtypes = [c.c_char_p]
        self.lsawrappers.new_LSAUncr.restype = c.c_void_p

        self.lsawrappers.delete_LSAUncr.argstypes = [c.c_void_p]
        self.lsawrappers.delete_LSAUncr.restype = None

        self.lsawrappers.setUncrSigma.argstypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setUncrSigma.restype = None

        self.lsawrappers.setUncrPPM.argtypes = [c.c_void_p, c.c_double]
        self.lsawrappers.setUncrPPM.restype = None

        self.lsawrappers.setUncrAtCenterError.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setUncrAtCenterError.restype = None

        self.lsawrappers.setUncrFromCenterError.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setUncrFromCenterError.restype = None

        self.lsawrappers.setUncrToCenterError.argtypes = [c.c_void_p, c.c_double, c.c_char_p]
        self.lsawrappers.setUncrToCenterError.restype = None

        self.lsawrappers.getLSAUncrString.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.getLSAUncrString.restype = None

        self.obj = c.c_void_p(self.lsawrappers.new_LSAUncr(label))

    def __del__(self):
        self.lsawrappers.delete_LSAUncr(self.obj)

    def set_sigma(self, value, units):
        assert isinstance(value, float), "value must be a float"
        assert isinstance(units, str),   "units must be a str"
        assert units != "",              "units cannot be empty"

        value = c.c_double(value)
        units = units.encode('utf-8')

        self.lsawrappers.setUncrSigma(self.obj, value, units)

    def set_PPM(self, value):
        assert isinstance(value, float), "value must be a float"

        value = c.c_double(value)

        self.lsawrappers.setUncrPPM(self.obj, value)

    def set_at_center(self, value, units):
        assert isinstance(value, float), "value must be a float"
        assert isinstance(units, str),   "units must be a str"
        assert units != "",              "units cannot be empty"

        value = c.c_double(value)
        units = units.encode('utf-8')

        self.lsawrappers.setUncrAtCenterError(self.obj, value, units)

    def set_from_center(self, value, units):
        assert isinstance(value, float), "value must be a float"
        assert isinstance(units, str),   "units must be a str"
        assert units != "",              "units cannot be empty"

        value = c.c_double(value)
        units = units.encode('utf-8')

        self.lsawrappers.setUncrFromCenterError(self.obj, value, units)

    def set_to_center(self, value, units):
        assert isinstance(value, float), "value must be a float"
        assert isinstance(units, str),   "units must be a str"
        assert units != "",              "units cannot be empty"

        value = c.c_double(value)
        units = units.encode('utf-8')

        self.lsawrappers.setUncrToCenterError(self.obj, value, units)

    def get_lsa_string(self):
        buffer = c.create_string_buffer(200)
        self.lsawrappers.getLSAUncrString(self.obj, buffer)
        return buffer.value.decode('utf-8')

