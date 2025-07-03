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

class Comment(LSAWrapper):
    def __init__(self, contents):
        # Validate parameters
        assert isinstance(contents, str), "comment contents must be a str"
        contents = contents.encode('utf-8')

        self.lsawrappers.new_LSAComment.argtypes = [c.c_char_p]
        self.lsawrappers.new_LSAComment.restype = c.c_void_p

        self.lsawrappers.delete_LSAComment.argtypes = [c.c_void_p]
        self.lsawrappers.delete_LSAComment.restype = None

        self.lsawrappers.getLSACommentString.argtypes = [c.c_void_p, c.c_char_p]
        self.lsawrappers.getLSACommentString.restype = None

        self.obj = c.c_void_p(self.lsawrappers.new_LSAComment(contents))

    def __del__(self):
        self.lsawrappers.delete_LSAComment(self.obj)

    def get_lsa_string(self):
        buffer = c.create_string_buffer(200)
        self.lsawrappers.getLSACommentString(self.obj, buffer)
        return buffer.value.decode('utf-8')
