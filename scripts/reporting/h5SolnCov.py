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
#This script reads a SALSA .h5 output file and generates a CSV file for the solution covariance matrix
#Usage examples...
#example: python h5SolnCov.py --h5 C:/Users/dtucker/Desktop/project1.h5 --scaleByAPV
#example: python h5SolnCov.py --h5 C:/Users/dtucker/Desktop/project1.h5 --cov C:/Users/dtucker/Desktop/project1cov.csv --scaleByAPV
#example: python h5SolnCov.py --h5 C:/Users/dtucker/Desktop/project1.h5

#python imports
import argparse as arg
import os
import sys
import h5py

#define the expected command-line argument flags, corresponding variable names, and default values (if any)
#Running 'python h5SolnCov.py --help' outputs the help text
parser = arg.ArgumentParser(usage="")
parser.add_argument('--in', action='store', dest='h5file', required=True, help=".h5 file name with full path (REQUIRED)")
parser.add_argument('--scaleByAPV', dest='APV', action='store_true', help="If present, scale by APV (OPTIONAL)")
parser.set_defaults(APV=False)
parser.add_argument('--cov', action='store', dest='outfile', required=False, help="Full path to CSV output file (OPTIONAL)")
parser.add_argument('--out', action='store', dest='outDir', required=False, help="Output directory for csv file. Not necessary if --cov is provided (OPTIONAL)")

#parse the command-line to the script
args = parser.parse_args()

#alert the user if the provided command-line is invalid
if args.h5file == None:
    sys.stderr.write('h5SolnCov.py: No input h5 file specified.\n')
    parser.print_help()
    exit(1)
if not os.path.exists(args.h5file):
    sys.stderr.write('h5SolnCov.py: Could not locate input h5 file %s. Exiting. ' % args.h5file)
    exit(1)
#if no output path is specified then place outfile in same directory as .h5 file with the .cov extension
if args.outfile == None:
    if args.outDir is None:
        pre, ext = os.path.splitext(args.h5file)
        args.outfile = pre + ".cov"
    else:
        baseName = os.path.basename(args.h5file)
        pre, ext = os.path.splitext(baseName)
        args.outfile = os.path.join(args.outDir, pre + ".cov")

#read the h5 file
with h5py.File(args.h5file, 'r') as h5file:#open h5 file
    #open group
    adjResultsGroup = h5file["Adjustment Results"]
    #open dataset
    stateNames = []
    solnCovDataSet = adjResultsGroup["Solution Covariance"]
    keylist = []
    for key in solnCovDataSet.attrs.keys():
        keylist.append(int(key))
    keylist.sort()
    for introw in keylist:
        stateName = solnCovDataSet.attrs[str(introw)]
        stateNames.append(stateName)

    covMatrix = solnCovDataSet[...]

    #open data set
    statsDataSet = adjResultsGroup["Statistics"]
    APVvalue = float(statsDataSet["APV"][0])

#prepare to scale by APV if the user has passed --scaleByAPV
if args.APV:
    scale = APVvalue
else:
    scale = 1.0

#write the csv file
with open(args.outfile,'w') as outfile:
    #write the column headers
    numCols = len(stateNames)
    outfile.write("STATE,")
    for i in range(numCols-1):
        outfile.write("{0},".format(stateNames[i]))
    outfile.write("{0}\n".format(stateNames[numCols-1]))
    #write the rows
    precision = 4#number of digits after the decimal point
    for i in range(numCols):
        outfile.write("{0},".format(stateNames[i]))
        for j in range(numCols-1):
            elem = covMatrix[i][j]*scale
            outfile.write("{0:.{1}e},".format(elem,precision))
        elem = covMatrix[i][numCols-1]*scale
        outfile.write("{0:.{1}e}\n".format(elem,precision))

print("\nSolution covariance CSV file written to {0}.".format(args.outfile))
