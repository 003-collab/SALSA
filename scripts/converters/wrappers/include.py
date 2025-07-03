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

class Include(LSAWrapper):
    def __init__(self, filename):
        # Validate parameters
        assert isinstance(filename, str), "comment contentes must be a str"

        self.lsawrappers.new_LSAInclude.argtypes = [c.c_char_p, c.c_char_p]
        self.lsawrappers.new_LSAInclude.restype = c.c_void_p

        self.lsawrappers.delete_LSAInclude.argtypes = [c.c_void_p]
        self.lsawrappers.delete_LSAInclude.restype = None

        self.lsawrappers.getLSAIncludeString.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.getLSAIncludeString.restype = None

        filename = filename.encode('utf-8')

        # NOTE: new_LSAInclude should take parameters of lsaPath and absolutePath but
        # absolute path is never needed by a converter
        # We did this as quick fix for a crash bug where we were only passing the lsa path
        self.obj = c.c_void_p(self.lsawrappers.new_LSAInclude(filename, filename))

    def __del__(self):
        self.lsawrappers.delete_LSAInclude(self.obj)

    def get_lsa_string(self):
        buffer = c.create_string_buffer(200)
        self.lsawrappers.getLSAIncludeString(self.obj, buffer)
        return buffer.value.decode('utf-8')
