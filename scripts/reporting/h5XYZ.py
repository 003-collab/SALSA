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
#python imports
import argparse as arg
import os
import subprocess
import re
import sys
import numpy as np
import h5py
import converterUtil as util
import lsaversion as ver
import math as m
import traceback
import time

class h5XYZconverter(object):
    def __init__(self, h5FileName, cfgFileName, outFileName):
        self.h5FileName = h5FileName
        self.cfgFileName = cfgFileName
        self.outFileName = outFileName
        self.xyzData = 0 
        self.linPrecision = 0
        self.includeUnused = False


    def readXYZData(self):
        #open file
        with h5py.File(self.h5FileName,'r') as h5file:
            #open group
            pointsGroup = h5file["Adjustment Results/Points"]
            #open dataset
            dset = pointsGroup["XYZ"]
            numOfEntries = len(dset)
            rdata = dset[...]

        linPrecision = self.linPrecision
        self.xyzData = rdata
        dataList = []

        if(len(rdata) > 0):
            for i in range(len(rdata)):
                # Is this point used
                ptUtilized = rdata[i]["Utilized"][0]

                if self.includeUnused or ptUtilized: 
                    # Station
                    postemp = rdata[i]["Point Label"]
                    pos = []
                    for j in postemp:
                        pos.append(j.decode())

                    # Coordinates
                    X = float(rdata[i]["X"][0])
                    Y = float(rdata[i]["Y"][0])
                    Z = float(rdata[i]["Z"][0])

                    XStr = util.formatFloat(X, linPrecision)
                    YStr = util.formatFloat(Y, linPrecision)
                    ZStr = util.formatFloat(Z, linPrecision)

                    dataList.append([pos, XStr, YStr, ZStr])
        else:
            sys.stderr.write("WARNING- no points found to generate XYZ\n")

        return dataList

    def readCfgData(self):
        with open(self.cfgFileName, 'r') as cfgFile:
            lines = cfgFile.readlines()
            #print(lines)
        for n in lines:
            line_args = n.split()
            if not line_args:
                continue
            if line_args[0] == "--linprecP":
                self.linPrecision = int(line_args[1])
            if line_args[0] == "--includeUnused":
                self.includeUnused = True

    def writeToXYZFile(self):
        try:
            with open(self.outFileName,'w') as outFile:
                dataList = self.readXYZData()
                outFile.write("Station,X_m,Y_m,Z_m\n")
                dataList.sort(key=lambda tup: tup[0]) #sort list alphabetically by station name
                for data in dataList:
                    pointlabel = " ".join(data[0])
                    dataString = map(str, data[1:])
                    outFile.write(pointlabel + ',' + self.cleanData(dataString)+ "\n")

        except EnvironmentError:
            sys.stderr.write('h5XYZ.py: ERROR: Unable to find/write {0}. Exiting.\n'.format(self.outFileName))
            exit(1)
        except RuntimeError as error:
            sys.stderr.write('h5XYZ.py: Parsing error. Unexpected value found.')
            sys.stderr.write(error)
            exit(1)

    def cleanData(self, dataString):
        return ','.join(dataString)

    def convert(self):
        self.readCfgData()
        self.writeToCSVFile()


def writeSalsaOutBegin():
    script = "h5XYZ"
    version = "1.0 8/11/16"
    SALSAv = (ver.LSAVERSION_MAJOR_MINOR_PATCH + " " +
              ver.LSAVERSION_BUILD_DATE + " " +
              ver.LSAVERSION_BUILD_TIME)
    runDate = time.strftime("%Y/%m/%d")
    runAt = time.strftime("%I:%M:%S")

    f1 = "{0}, Ver. {1} rev, SALSA Ver. {2}, Run {3} at {4}\n"
    print(f1.format(script, version, SALSAv, runDate, runAt))

def writeSalsaOutputFile(outFileArg):
    f2 = "Output directed to log file {0}\n"
    print(f2.format(outFileArg))

def writeSalsaOutFinish(start, end):
    timeLapse = end - start
    f3 = "h5XYZ timing: {0:4.3f} seconds. ({1:4.3f} sec)\n"
    print(f3.format(timeLapse, round(timeLapse)))

#code execution starts here
if __name__ == '__main__':

    startTime = time.time()
    writeSalsaOutBegin()

    parser = arg.ArgumentParser(usage="")
    parser.add_argument('--h5', action='store', dest='h5file', required=True)
    parser.add_argument('--cfg', action='store', dest='cfgfile', required=True)
    parser.add_argument('--xyz', action='store', dest='outfile', required=False)

    args = parser.parse_args()
    writeSalsaOutputFile(args.outfile)

    if args.h5file == None:
        sys.stderr.write('h5XYZ.py: No input h5 file specified.\n')
        parser.print_help()
        exit(1)
    if args.cfgfile == None:
        sys.stderr.write('h5XYZ.py: No input config file specified.\n')
        parser.print_help()
        exit(1)  
    #if no output path is specified then place outfile in same directory as .h5 file
    if args.outfile == None:
        args.outfile = args.h5file[:-3] + ".xyz"
    if not os.path.exists(args.h5file):
        sys.stderr.write('h5XYZ.py: Could not locate input h5 file %s. Exiting. ' % args.h5file)
        exit(1)
    if not os.path.exists(args.cfgfile):
        sys.stderr.write('h5XYZ.py: Could not locate input config file %s. Exiting. ' % args.cfgfile)
        exit(1)

    converter = h5XYZconverter(args.h5file, args.cfgfile, args.outfile)

    try:
        converter.readCfgData()
        converter.writeToXYZFile()
    except Exception as e:
        sys.stderr.write(" ERROR generating XYZ file\n")
    finally: #executes if there are no exceptions
        endTime = time.time()
        writeSalsaOutFinish(startTime, endTime)

