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
import sys
import os
import glob
import importlib

FILE_DIR = os.path.dirname(os.path.realpath(__file__))
SCRIPTS_DIR = os.path.join(FILE_DIR,"..")

# don't iterate over all subdirectories, just converters and reporting
SUBDIRS = ['converters', 'reporting']

if __name__ == '__main__':
    files_to_import = []
    os.chdir(SCRIPTS_DIR)

    for directory in SUBDIRS:
        sys.path.append(os.path.join(SCRIPTS_DIR, directory))
        files_to_import.extend(glob.glob(directory + '/*.py'))

    modules_to_import = [os.path.basename(file.replace('.py','')) for file in files_to_import if '__' not in file]

    for module in modules_to_import:
        print(importlib.import_module(module))
