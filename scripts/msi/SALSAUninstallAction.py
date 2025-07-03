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
import shutil

if __name__ == '__main__':
    FILE_DIR = os.path.join(os.path.dirname(os.path.realpath(__file__)), '../..')
    PYCACHE_DIRS = [result[0] for result in os.walk(FILE_DIR) if '__pycache__' in result[0]]

    for directory in PYCACHE_DIRS:
        shutil.rmtree(directory)
