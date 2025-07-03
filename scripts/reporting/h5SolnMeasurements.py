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
#This script reads a SALSA .h5 output file and generates a CSV file for the measurement data

#python imports
import argparse as arg
import os
import sys
import h5py

#define the expected command-line argument flags, corresponding variable names, and default values (if any)
parser = arg.ArgumentParser(usage="")
parser.add_argument('--in', action='store', dest='h5file', required=True, help=".h5 file name with full path (REQUIRED)")
parser.add_argument('--csv', action='store', dest='outfile', required=False, help="Full path to CSV output file (OPTIONAL)")
parser.add_argument("--out", action='store', dest='outDir', required=False, help="Output directory for csv file. Not necessary if --csv is provided (OPTIONAL)")

#parse the command-line to the script
args = parser.parse_args()

#alert the user if the provided command-line is invalid
if args.h5file == None:
    sys.stderr.write('h5SolnMeasurements.py: No input h5 file specified.\n')
    parser.print_help()
    exit(1)
if not os.path.exists(args.h5file):
    sys.stderr.write('h5SolnMeasurements.py: Could not locate input h5 file %s. Exiting. ' % args.h5file)
    exit(1)
#if no output path is specified then place outfile in same directory as .h5 file with the .csv extension
if args.outfile == None:
    if args.outDir is None:
        pre, ext = os.path.splitext(args.h5file)
        args.outfile = pre + ".resid.csv"
    else:
        baseName = os.path.basename(args.h5file)
        pre, ext = os.path.splitext(baseName)
        args.outfile = os.path.join(args.outDir, pre + ".resid.csv")

#read the h5 file
with h5py.File(args.h5file,'r') as h5file:#open h5 file
    #open group
    adjResultsGroup = h5file["Adjustment Results"]
    #open dataset
    DataSet = adjResultsGroup["Measurements"]
    DataSet = DataSet["Measurements"]
    #extract data set as a 2D array
    data = DataSet[...]

#write the csv file
with open(args.outfile,'w') as outfile:
    headers = "Measurement Type, From Point, At Point, To Point, Tag, Initial Measurement, Adjusted Measurement, Raw Residual, Relative Residual, Standard Residual, Redundancy, Minimum Detectable Bias, External Reliability Magnitude, Possible Outlier\n"
    outfile.write(headers)
    #write the column headers
    numRows = len(data)
    numCols = len(headers.split(","))
    # angularMeasurements array
    angularMeasurements = ["AZIM", "ZANG", "VANG", "HANG"]
    #write the rows
    for i in range(numRows):
        elems = data[i][0]
        isAngular = False
        for j in range(numCols):
            elem = elems[j]
            try:  elem = elem.decode()
            except:  pass
            # Measurement type
            if (j == 0):
              # Discover if the measurment is angular
              if elem in angularMeasurements:
                isAngular = True
              else:
                isAngular = False
              outfile.write(str(elem))
            # From, At, To points as strings
            elif j in [1,2,3]:  
              if len(elem)>0: 
                elem='"'+elem+'"'
              outfile.write(str(elem))
            # Tag as unformatted number
            elif (j == 4):
                outfile.write(str(elem))
            # Initial measurement and residuals, formatted depending on type
            elif j in [5,6,7,8,9]:
              if isAngular:
                outfile.write("{:.1f}".format(float(elem))) # 1 decimal for SOA
              else:
                outfile.write("{:.4f}".format(float(elem))) # 4 decimals for linear
            # Redundancy
            elif (j==10):
              outfile.write("{:.2f}".format(float(elem))) # 2 decimals for redundancy
            # Internal reliability, should match measurements
            elif (j==11):
              if isAngular:
                outfile.write("{:.1f}".format(float(elem))) # 1 decimal for SOA
              else:
                outfile.write("{:.4f}".format(float(elem))) # 4 decimals for linear
            # External reliability, is in state space so should be linear
            elif (j==12):
              outfile.write("{:.4f}".format(float(elem))) # 4 decimals for linear
            # Possible outliers?
            else:
              outfile.write(str(elem))
            if j<numCols-1:outfile.write(", ")
        outfile.write("\n")
print("\nMeasurement CSV file written to {0}.".format(args.outfile))
