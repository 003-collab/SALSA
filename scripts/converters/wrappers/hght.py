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

class LSAHeight(LSAWrapper):
    def __init__(self, label, height, lunits, sigma, sigmaUnits):
        """Encapsulate a LSAHeight object

        Required Arguments:
        label (str)      -- the label of the HGHT record
        height (double)  -- the value of the height at that station
        lunits (str)     -- the label of the linear unit for the height
        sigma (double)   -- the value of the instrument height uncertainty
        sigmaUnits (str) -- the label of the linear unit for the uncertainty
        """

        # Validate parameters
        assert isinstance(label, str), "Height label must be a str"
        assert label != "", "Height label cannot be empty"
        assert isinstance(height, float), "Height value must be a float"
        assert isinstance(lunits, str), "Height lunits must be a str"
        assert lunits != "", "Height lunits cannot be empty"
        assert isinstance(sigma, float), "Sigma value must be a float"
        assert isinstance(sigmaUnits, str), "Sigma units must be a str"
        assert sigmaUnits != "", "Sigma units cannot be empty"

        self.lsawrappers.new_LSAHeight.argtypes = [c.c_char_p, c.c_double, c.c_char_p, c.c_double, c.c_char_p]
        self.lsawrappers.new_LSAHeight.restype = c.c_void_p

        self.lsawrappers.delete_LSAHeight.argtypes = [c.c_void_p]
        self.lsawrappers.delete_LSAHeight.restype = None

        self.lsawrappers.getLSAHeightString.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.getLSAHeightString.restype = None

        label = label.encode('utf-8')
        height = c.c_double(height)
        lunits = lunits.encode('utf-8')
        sigma = c.c_double(sigma)
        sigmaUnits = sigmaUnits.encode('utf-8')

        self.obj = c.c_void_p(self.lsawrappers.new_LSAHeight(label, height, lunits, sigma, sigmaUnits))

    def __del__(self):
        self.lsawrappers.delete_LSAHeight(self.obj)

    def get_lsa_string(self):
        buffer = c.create_string_buffer(200)
        self.lsawrappers.getLSAHeightString(self.obj, buffer)
        return buffer.value.decode('utf-8')
