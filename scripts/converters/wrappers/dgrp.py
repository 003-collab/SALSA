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

class LSADirGroup(LSAWrapper):
    def __init__(self, label, at_point):
        """Encapsulate a LSADirGroup object

        Required Arguments:
        label(str)         -- the label of the DGRP record
        at_point (str)     -- the label of the AT station
        """

        # Validate parameters
        assert isinstance(label, str), "DirGroup label must be a str"
        assert label != "", "DirGroup label cannot be empty"
        assert isinstance(at_point, str), "DirGroup at_point must be a str"
        assert at_point != "", "DirGroup at_point cannot be empty"

        label = label.encode('utf-8')
        at_point = at_point.encode('utf-8')

        self.lsawrappers.new_LSADirGroup.argtypes = [c.c_char_p, c.c_char_p]
        self.lsawrappers.new_LSADirGroup.restype = c.c_void_p

        self.lsawrappers.delete_LSADirGroup.argtypes = [c.c_void_p]
        self.lsawrappers.delete_LSADirGroup.restype = None

        self.lsawrappers.setDirGroupIsReduced.argtypes = [c.c_void_p, c.c_bool]
        self.lsawrappers.setDirGroupIsReduced.restype = None

        self.lsawrappers.setDirGroupUNCRLabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setDirGroupUNCRLabel.restype = None

        self.lsawrappers.setDirGroupVSCALabel.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.setDirGroupVSCALabel.restype = None

        self.lsawrappers.setDirGroupVSCAValue.argtypes = [c.c_void_p, c.c_double]
        self.lsawrappers.setDirGroupVSCAValue.restype = None

        self.lsawrappers.getLSADirGroupString.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.getLSADirGroupString.restype = None

        self.obj = c.c_void_p(self.lsawrappers.new_LSADirGroup(label, at_point))

    def __del__(self):
        self.lsawrappers.delete_LSADirGroup(self.obj)

    def set_is_reduced(self, is_reduced):
        assert isinstance(is_reduced, bool), "is_reduced must be a bool"
        value = c.c_bool(is_reduced)
        self.lsawrappers.setDirGroupIsReduced(self.obj, is_reduced)

    def set_uncr_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setDirGroupUNCRLabel(self.obj, label.encode('utf-8'))

    def set_vsca_label(self, label):
        assert isinstance(label, str),  "label must be a str"
        assert label != "",             "label cannot be empty"
        self.lsawrappers.setDirGroupVSCALabel(self.obj, label.encode('utf-8'))

    def set_vsca_value(self, value):
        assert isinstance(value, float), "value must be a float"
        value = c.c_double(value)
        self.lsawrappers.setDirGroupVSCAValue(self.obj, value)

    def get_lsa_string(self):
        buffer = c.create_string_buffer(200)
        self.lsawrappers.getLSADirGroupString(self.obj, buffer)
        return buffer.value.decode('utf-8')
