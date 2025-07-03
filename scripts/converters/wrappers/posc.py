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

class LSAPosC(LSAWrapper):
    def __init__(self, label, x, y, z, units):
        """Encapsulate a LSAPosC object"""

        # Validate parameters
        assert isinstance(label, str), "label must be a str"
        assert label != "",            "label cannot be empty"
        assert isinstance(x, float),        "x must be a float"
        assert isinstance(y, float),        "y must be a float"
        assert isinstance(z, float),        "z must be a float"
        assert isinstance(units, str),      "units must be a str"
        assert units != "",                 "units cannot be empty"

        self.lsawrappers.new_LSAPosC.argtypes = [c.c_char_p, c.c_double, c.c_double, c.c_double, c.c_char_p]
        self.lsawrappers.new_LSAPosC.restype = c.c_void_p

        self.lsawrappers.delete_LSAPosC.argtypes = [c.c_void_p]
        self.lsawrappers.delete_LSAPosC.restype = None

        self.lsawrappers.setPosCCovariance.argtypes = [c.c_void_p, c.c_double, c.c_double, c.c_double, c.c_double,
                                                       c.c_double, c.c_double]
        self.lsawrappers.setPosCCovariance.restype = None

        self.lsawrappers.setPosCVSCALabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setPosCVSCALabel.restype = None

        self.lsawrappers.setPosCVSCAValue.argtypes = [c.c_void_p, c.c_double]
        self.lsawrappers.setPosCVSCAValue.restype = None

        self.lsawrappers.getLSAPosCString.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.getLSAPosCString.restype = None
        
        label = label.encode('utf-8')
        x = c.c_double(x)
        y = c.c_double(y)
        z = c.c_double(z)
        units = units.encode('utf-8')

        self.obj = c.c_char_p(self.lsawrappers.new_LSAPosC(label, x, y, z, units))

    def __del__(self):
        self.lsawrappers.delete_LSAPosC(self.obj)

    def set_covariance(self, cxx, cxy, cxz, cyy, cyz, czz):
        assert isinstance(cxx, float),      "dx must be a float"
        assert isinstance(cxy, float),      "cxy must be a float"
        assert isinstance(cxz, float),      "cxz must be a float"
        assert isinstance(cyy, float),      "cyy must be a float"
        assert isinstance(cyz, float),      "cyz must be a float"
        assert isinstance(czz, float),      "czz must be a float"
        cxx = c.c_double(cxx)
        cxy = c.c_double(cxy)
        cxz = c.c_double(cxz)
        cyy = c.c_double(cyy)
        cyz = c.c_double(cyz)
        czz = c.c_double(czz)

        self.lsawrappers.setPosCCovariance(self.obj, cxx, cxy, cxz, cyy, cyz, czz);

    def set_state_constrained(self):
        self.lsawrappers.setPosCStateConstrained(self.obj)

    def set_state_fixed(self):
        self.lsawrappers.setPosCStateFixed(self.obj)

    def set_state_floating(self):
        self.lsawrappers.setPosCStateFloating(self.obj)

    def set_vsca_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setPosCVSCALabel(self.obj, label.encode('utf-8'))

    def set_vsca_value(self, value):
        assert isinstance(value, float), "value must be a float"
        value = c.c_double(value)
        self.lsawrappers.setPosCVSCAValue(self.obj, value)

    def get_lsa_string(self):
        buffer = c.create_string_buffer(400)
        self.lsawrappers.getLSAPosCString(self.obj, buffer)
        return buffer.value.decode('utf-8')
