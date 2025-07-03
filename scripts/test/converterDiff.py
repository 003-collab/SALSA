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
import re
import sys
import operator
#import numpy as np

#np.set_printoptions(precision=5)

def returnProblem(message,value=1):
    """Print a message and exit with a non-zero return value."""
    sys.stdout.write(("").join(list(diffs)))
    sys.stdout.flush()
    sys.exit(value)
    
def returnSuccess():
    """Print 0 to stdout and exit."""
    sys.stdout.write('0')
    sys.stdout.flush()
    sys.exit(0)

print('In converterDiff.py, command line = {0}'.format(str(sys.argv)))
# Get command line input
parser = argparse.ArgumentParser(description="Compare ascii files")

parser.add_argument("truthfile", type=str,
        help='file, in our .out format, represents expected values')

parser.add_argument("testfile",  type=str,
        help='file to be compared against the truth file')

parser.add_argument('-d', action='store_true',
        help='enable debug output')
args = parser.parse_args()

truthfile = args.truthfile
testfile = args.testfile
debug = args.d

if debug:
    print('')
    print('Starting lsadiff.py...')
    print('DIFF CMD - output file: {}'.format(testfile))
    print('DIFF CMD - truth file: {}'.format(truthfile))
    print('DIFF CMD - debug enabled: {}'.format(debug))

if testfile.startswith('"') and testfile.endswith('"'):
    testfile = testfile[1:-1]
if truthfile.startswith('"') and truthfile.endswith('"'):
    truthfile = truthfile[1:-1]
with open(testfile,'r') as test:
    with open(truthfile,'r') as truth:
        testlines = test.readlines()
        truthlines = truth.readlines()
        del testlines[0:2]
        del truthlines[0:2]
        diffs = set(testlines).symmetric_difference(truthlines)

if not diffs:
    returnSuccess()
else:
    returnProblem(str(list(diffs)))




