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
#Author Brad Beal
#reads a SALSA .h5 output file and generates a CSV (.txt) file for 
#input into NGS' NCAT online program
#https://www.ngs.noaa.gov/NCAT/

#python imports
import argparse as arg
import os
import sys
import h5py
import ctypes
import csv

#define the expected command-line argument flags, corresponding variable names, and default values (if any)
parser = arg.ArgumentParser(usage="")
parser.add_argument('--in', action='store', dest='h5file', required=True, help=".h5 file name with full path (REQUIRED)")
parser.add_argument('--out', action='store', dest='outfile', required=False, help="Full path to CSV output file (OPTIONAL)")

#parse the command-line to the script
args = parser.parse_args()

#alert the user if the provided command-line is invalid
if args.h5file == None:
    sys.stderr.write('h5NGS.py: No input h5 file specified.\n')
    parser.print_help()
    exit(1)
if not os.path.exists(args.h5file):
    sys.stderr.write('h5NGS.py: Could not locate input h5 file %s. Exiting. ' % args.h5file)
    exit(1)
#if no output path is specified then place outfile in same directory as .h5 file with the .cov extension
if args.outfile == None:
    pre, ext = os.path.splitext(args.h5file)
    args.outfile = pre + "_NCAT.txt"
    

#read the h5 file
with h5py.File(args.h5file, 'r') as h5file:#open h5 file
    #open group
    pointsGroup = h5file["Adjustment Results/Points"]
    dset = pointsGroup["LLH"]
    numOfEntries = len(dset)
    # numOfLabels = len(dset[0][0])
    rdata = dset[...] #need this to take raw h5 data and convert into an array
    #print (rdata)
    

#Open file and assign LLH values
with open(args.outfile,'w',newline='') as outfile:
    writer = csv.writer(outfile)
    numRows = len(rdata)

    #write the rows
    precision = 3 #number of digits after the decimal point
    dataList = [] #initialize the list of data
    for i in range(numRows):
        # Station
        postemp = rdata[i]["Point Label"] #This gets the Point Label attribute (ex ['VNDP'])
        pos = []
        for j in postemp:
            pos.append(j.decode()) #convert byte data from the h5 file into a string
        pointName = " ".join(pos)   #use this to strip out [] and '' from the point label (ex. VNDP) 
        #print (pointName)
        # Latitude DD
        LatDD = rdata[i]["Latitude"][0]
        LatDDstr = "{:.9f}".format(float(LatDD)) #limit number of fixed digits after decimal point
        #print ('latDD=',LatDD)
        #print ('latDDstr=',LatDDstr)
        # Longitude DD
        LonDD = rdata[i]["Longitude"][0]
        if LonDD > 180:
            LonDD = -1*(360-LonDD)
        LonDDstr = "{:.9f}".format(float(LonDD)) #limit number of fixed digits after decimal point
        # Ellipsoid height
        elHt = rdata[i]["Height"][0]
        elHtstr = "{:.3f}".format(float(elHt)) #limit number of fixed digits after decimal point
        # Orthometric height
        orHt = rdata[i]["Oht"][0]
        orHtstr = "{:.3f}".format(float(orHt)) #limit number of fixed digits after decimal point
        LLH = (LonDD, LatDD, orHt)

        #WGS84 parameters:
        a = str(6378137.0) #Semi-major axis 
        finv = str(298.257223563) #Semi-minor axis 
        dataStr = [pointName,LatDDstr,LonDDstr,elHtstr,a,finv,'auto']         
        writer.writerow(dataStr)
             
            
    #MessageBox = ctypes.windll.user32.MessageBoxW
    #MessageBox(None, 'UTM Export Complete', 'UTM Export', 0)


print("\nSolution NGS CSV file written to {0}.".format(args.outfile))
