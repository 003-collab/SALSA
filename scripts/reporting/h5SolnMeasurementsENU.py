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
import math
import numpy

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
    DataSet = adjResultsGroup["Points"]
    DataSet = DataSet["LLH"]

    #create dict of reference position labels to their lat/lot
    refLabelToPos = {}
    pointData = DataSet[...]
    numRows = len(pointData)
    for i in range(numRows):
        elems = pointData[i][0]
        try:
            # label to (lat, lon) in radians
            label = str(elems[0].decode())
            refLabelToPos[label] = (math.radians(elems[1]), math.radians(elems[2]))
        except:
            pass

def getRotationMatrix(refLat, refLon):
    return numpy.array([[ -math.sin(refLon), math.cos(refLon), 0.0 ],
                        [ -math.sin(refLat)*math.cos(refLon), -math.sin(refLat)*math.sin(refLon), math.cos(refLat) ],
                        [ math.cos(refLat)*math.cos(refLon), math.cos(refLat)*math.sin(refLon), math.sin(refLat) ]])

#write the csv file
with open(args.outfile,'w') as outfile:
    headers = "Measurement Type, From Point, At Point, To Point, Tag, Initial Measurement, Adjusted Measurement, Raw Residual, Relative Residual, Standard Residual, Redundancy, Minimum Detectable Bias, External Reliability Magnitude, Blunders Present\n"
    outHeaders = "Measurement Type, From Point, To Point, Tag, Raw Residual\n"
    #write the column headers
    outfile.write(outHeaders)
    numRows = len(data)
    numCols = len(headers.split(","))
    #build the dictionary of axes -> from-to pairs -> raw residuals
    XLocsToRaw = {}
    YLocsToRaw = {}
    ZLocsToRaw = {}
    for i in range(numRows):
        try:
            elems = data[i][0]
            axis = elems[0]
            axis = axis.decode()
            fromPoint = elems[1]
            fromPoint = fromPoint.decode()
            toPoint = elems[3]
            toPoint = toPoint.decode()
            rawResid = elems[7]
            if str(axis) == "DXYZ.X":
                XLocsToRaw[fromPoint+toPoint] = rawResid
            if str(axis) == "DXYZ.Y":
                YLocsToRaw[fromPoint+toPoint] = rawResid
            if str(axis) == "DXYZ.Z":
                ZLocsToRaw[fromPoint+toPoint] = rawResid
        except: pass

    #write the rows
    for i in range(numRows):
        elems = data[i][0]

        try:
            type = elems[0]
            type = type.decode()
        except:  pass
        if "DXYZ" not in type:
            continue # we're only outputting DXYZ records

        for j in range(numCols):
            # Measurement Type(0), From Point(1), To Point(3), Tag(4), Raw Residual(7)
            if j in [0,1,3,4,7]:
                elem = elems[j]
                try: elem = elem.decode()
                except: pass

                if j==0:
                    if elem == "DXYZ.X":
                        elem = "DXYZ.E"
                    if elem == "DXYZ.Y":
                        elem = "DXYZ.N"
                    if elem == "DXYZ.Z":
                        elem = "DXYZ.U"

                if j in [1,3]:
                    if len(elem)>0: elem='"'+elem+'"'
                if j==7:
                    try:
                        axis = elems[0]
                        axis = axis.decode()
                        fromPoint = elems[1]
                        fromPoint = fromPoint.decode()
                        toPoint = elems[3]
                        toPoint = toPoint.decode()

                        lat, lon = refLabelToPos[fromPoint]
                        rotMat = getRotationMatrix(lat, lon)

                        #retrieve the unrotated raw residuals
                        xRaw = XLocsToRaw[fromPoint+toPoint]
                        yRaw = YLocsToRaw[fromPoint+toPoint]
                        zRaw = ZLocsToRaw[fromPoint+toPoint]
                        rawVec = numpy.array([xRaw,yRaw,zRaw])

                        enuRawVec = numpy.dot(rotMat, rawVec)
                        if str(axis) == "DXYZ.X":
                            rotatedRaw = enuRawVec[0] #E
                        if str(axis) == "DXYZ.Y":
                            rotatedRaw = enuRawVec[1] #N
                        if str(axis) == "DXYZ.Z":
                            rotatedRaw = enuRawVec[2] #U
                        outfile.write("{:.4f}".format(float(rotatedRaw)))
                    except: pass
                else:
                    outfile.write(str(elem))
                    outfile.write(", ")
        outfile.write("\n")

print("\nMeasurement CSV file written to {0}.".format(args.outfile))
