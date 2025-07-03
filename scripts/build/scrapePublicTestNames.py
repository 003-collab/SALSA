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
#!/usr/bin/python

import datetime as dt
import re
import os
import sys
import subprocess

# Get the directory of this script
ThisDir = os.path.dirname(os.path.realpath(__file__))

TESTLSAGUICPP = os.path.normpath(os.path.join(ThisDir,   '../../src/public_guitest/testpubliclsagui.cpp'))

parseEnabled = False
testnames=[]
with open(TESTLSAGUICPP) as f:
    for line in f.readlines():
        if "// START OF TEST NAMES" in line:
            parseEnabled = True
        if "// END OF TEST NAMES" in line:
            parseEnabled = False
        if parseEnabled:
            testname = re.search(r'(?<=    void )[^/(]*', line)
            if testname:
                testnames.append(testname.group(0))

sys.stdout.write(" ".join(testnames))

