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
import os
import ctypes as c
import abc
from ctypes.util import find_library

import sys

class LSAWrapper:
    ___metaclass__ = abc.ABCMeta

    """Base class for all python wrappers used to access code in liblsacommon"""

    source_dir = os.path.dirname(os.path.abspath(__file__))
    dll_dir = ''

    if os.name == 'nt':
        # Needed for the python upgrades
        dll_dir = os.path.normpath(os.path.join(source_dir, '../../../lib/'))
        os.add_dll_directory(dll_dir)

        # bin of installed SALSA
        salsa_bin_dir = os.path.normpath(os.path.join(source_dir, '../../../bin/'))

        # if this is a source build of salsa, we'll get the dlls from env var
        qt_install = os.path.normpath(os.environ.get('CMAKE_PREFIX_PATH', 'C:\Qt\Qt5.15.8\bin'))
        qt_bin_dir = os.path.normpath(os.path.join(qt_install, 'bin'))

        if os.path.isdir(salsa_bin_dir):
            os.add_dll_directory(salsa_bin_dir)
        elif os.path.isdir(qt_bin_dir):
            os.add_dll_directory(qt_bin_dir)
        os.add_dll_directory(r'C:\Windows\System32') # (not sure if needed)
        dllname = os.path.normpath(os.path.join(source_dir, '../../../lib/lsawrappers.dll'))
    else:
        dllname = os.path.normpath(os.path.join(source_dir, '../../../lib/liblsawrappers.so'))

    lsawrappers = c.cdll.LoadLibrary(dllname)

    @abc.abstractmethod
    def get_lsa_string(self):
        """Get the lsa string for this object"""
        return


