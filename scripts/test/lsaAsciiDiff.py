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

# The purpose of this python script is to demonstrate
# a basic integration test capability.  We compare an lsa solver
# output file against an expected value file.  If we are able to
# identify common values, and all of them compare to a threshold,
# we return 0 (pass).  Anything else should be interpreted as a
# failure, and we'll do our best to return an informative string.

import argparse
import sys
import numpy as np
import os
import lsaversion

salsa_version_short = "SALSA Version " + lsaversion.LSAVERSION_MAJOR_MINOR_PATCH
SALSA_VERSION_STRING = lsaversion.LSAVERSION_MAJOR_MINOR_PATCH + \
                       " -- Backward compatible to " \
                       + lsaversion.LSASUPPORTEDTOVERSION_MAJOR_MINOR_PATCH
np.set_printoptions(precision=5)   

# Get command line input
parser = argparse.ArgumentParser(description="Compare ascii files")

parser.add_argument('truthfile',type=argparse.FileType('r'),help='file, in our .out format, represents expected values')
parser.add_argument('testfile',type=argparse.FileType('r'), help='file to be compared against the truth file')
parser.add_argument('-binPath', type=str, default="", action='store')
parser.add_argument('-rootPath', type=str, default="", action='store', required=False)
parser.add_argument('-d', action='store_true', help='enable debug output')
parser.add_argument('-n', type=int, default=0, action='store', required=False)

args = parser.parse_args()

truthfile = args.truthfile
testfile = args.testfile
binPath = args.binPath
rootPath = args.rootPath
debug = args.d
numSkipped = args.n

installDir = binPath + "/"

if debug:
    print('')
    print('Starting lsadiff.py...')
    print('DIFF CMD - output file: {}'.format(testfile.name))
    print('DIFF CMD - truth file: {}'.format(truthfile.name))
    print('DIFF CMD - debug enabled: {}'.format(debug))

# get a set of lines for each file and correct num of exponent digits in windows file
testlinesRead = testfile.readlines()
truthlinesRead = truthfile.readlines()


# Get a path to the lsa directory
lsa_dir = rootPath + '/'

# replace any install tokens in truthlines
truthlines = []
ctr=0
for line in truthlinesRead:
    ctr = ctr + 1
    if ctr<=numSkipped:
        continue
    line = str.replace(line, "[$INSTALL_PATH$]" , installDir)
    line = str.replace(line, "[$LSA_DIR$]", lsa_dir)
    line = str.replace(line, "[$SALSA_VERSION$]", SALSA_VERSION_STRING)
    line = str.replace(line, "[$SALSA_VERSION_SHORT$]", salsa_version_short)
    if ("lsapost" in line):
        continue
    if ("Dump of binary file" in line): 
        continue
    if ("lsainverse timing:" in line): 
        continue
    truthlines.append(line)

#strip out lsapost time stamps and input file paths from output
testlines = []
ctr=0
for line in testlinesRead:
    ctr = ctr + 1
    if ctr<=numSkipped:
        continue
    if ("lsapost" in line):
        continue
    if ("Dump of binary file" in line): 
        continue
    if ("lsainverse timing:" in line): 
        continue

    testlines.append(line)

import difflib
d = difflib.Differ()
diff = d.compare(truthlines, testlines)

# filter lines in the diff output that begin with whitepsace
userOutput = []
for line in diff:
    if not line.startswith('  '):
        userOutput.append(line)

if len(userOutput) == 0:
    """Print 0 to stdout and exit."""
    sys.stdout.write('0')
    sys.stdout.flush()
    sys.exit(0)
else:
    sys.stdout.write(("").join(userOutput))
    sys.stdout.flush()
    sys.exit(1)



