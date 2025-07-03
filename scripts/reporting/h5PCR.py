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
import time
startTime = time.time()
import argparse as arg
import os
import sys
import numpy as np
import h5py
import math as m
import traceback

import converterUtil as util
import lsaversion as ver
import lsaversion

# PCR - Point Confidence Regions
# Mostly a copy of h5PTS.py
class h5PCRconverter(object):

    def __init__(self, h5File, cfgFile, outFile):
        self.h5FileName = h5File
        self.cfgFileName = cfgFile
        self.outFileName = outFile
        self.longitudeWest = False
        self.linPrecision = 0
        self.angPrecision = 0
        self.APVflag = False
        self.includeUnused = False
        self.confidence = 0

    def readH5File(self):        
        # open file
        with h5py.File(self.h5FileName, "r") as h5file:
            # open group
            pointsGroup = h5file["Adjustment Results/Points"]
            # open LLH dataset
            dset = pointsGroup["LLH"]
            numOfEntries = len(dset)
            rdata = dset[...]

            # open statistics for APV
            statGroup = h5file["Adjustment Results/Statistics"]
            statData = statGroup[...]

            APV = statData["APV"]
            #check if data needs to be scaled by APV
            if self.APVflag:
                APV = m.sqrt(APV)
            else:
                APV = 1

        linPrecision = self.linPrecision
        angPrecision = self.angPrecision
        decDegreePrecision = 9

        # new list for prepping output
        columnHeaders = ""
        dataList = []

        # dataset labels
        if(self.longitudeWest == True):
            direction = "W"
        else:
            direction = "E"
        labelFormat = "{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12}\n"
        columnHeaders = labelFormat.format("Position","Latitude N","Longitude " + direction,"Latitude Dec","Longitude Dec","EHt(m)","OHt(m)","3DMaj(m)","2DMaj(m)","Vert(m)","3DMajRR(m)","2DMajRR(m)","VertRR(m)")

        # calculate the position width
        maxPointWidth = 0
        for label in rdata["Point Label"]:
            pointLabel = self.byteArrayToStr(label)
            if len(pointLabel) > maxPointWidth:
                maxPointWidth = len(pointLabel)

        for i in range(len(rdata)):
            # Is this point used
            ptUtilized = rdata[i]["Utilized"][0]

            if self.includeUnused or ptUtilized:    
                # Is this point Fixed
                fixedNum = int(rdata[i]["Fixed Type"])
                myFixedType = util.FixedType(fixedNum).name

                # Position
                pos = self.byteArrayToStr(rdata[i]["Point Label"])
                posStr = "{0},".format(pos)

                # Latitude
                lat = util.decDegToDMS(rdata[i]["Latitude"], angPrecision)
                lat[2] = round(lat[2], angPrecision) + 0
                if type(lat[0]) is str:
                    latStr = "{l[0]} {l[1]:2d} {l[2]:.{0}f},".format(angPrecision,l=lat)
                else:
                    latStr = "{l[0]:2d} {l[1]:2d} {l[2]:.{0}f},".format(angPrecision,l=lat)

                latDec = util.formatFloat(rdata[i]["Latitude"], decDegreePrecision)
                latDecStr = "{0},".format(latDec)

                # Longitude
                if self.longitudeWest == True:
                    lon = util.eastLongitudeToWest(rdata[i]["Longitude"])
                else:
                    lon = rdata[i]["Longitude"]

                lon = util.decDegToDMS(lon, angPrecision)
                lon[2] = round(lon[2], angPrecision) + 0
                if type(lon[0]) is str:
                    lonStr = "{l[0]} {l[1]:d} {l[2]:.{0}f},".format(angPrecision,l=lon)
                else:
                    lonStr = "{l[0]:d} {l[1]:d} {l[2]:.{0}f},".format(angPrecision,l=lon)

                lonDec = util.formatFloat(rdata[i]["Longitude"], decDegreePrecision)
                lonDecStr = "{0},".format(lonDec)

                # Ellipsoidal Height
                eht = util.rmNegZero(rdata[i]["Height"], linPrecision)
                eht = "{0:.{1}f},".format(eht, linPrecision)

                # Orthemetic Height
                oht = util.rmNegZero(rdata[i]["Oht"], linPrecision)
                oht = "{0:.{1}f}".format(oht, linPrecision)

                if("FIXED" == myFixedType):
                    threeDMajVal = 0
                    # 2d Maj
                    twoDMajVal = 0
                    # Vert
                    VertVal = 0
                    # 3d Maj RR
                    threeDMajRRVal = 0
                    # 2d Maj RR
                    twoDMajRRVal = 0
                    # Vert RR
                    VertRRVal = 0
                else:
                    threeDMajVal = float(APV*rdata[i]["3D Major"])
                    # 2d Maj
                    twoDMajVal = float(APV*rdata[i]["2D Major"])
                    # Vert
                    VertVal = float(APV*rdata[i]["Vertical"])
                    # 3d Maj RR
                    threeDMajRRVal = float(rdata[i]["3D Major RR"])
                    # 2d Maj RR
                    twoDMajRRVal = float(rdata[i]["2D Major RR"])
                    # Vert RR
                    VertRRVal = float(rdata[i]["Vertical RR"])

                threeDMajStr = ",{0:.{1}f},".format(threeDMajVal, linPrecision)
                # 2d Maj
                twoDMajStr = "{0:.{1}f},".format(twoDMajVal, linPrecision)
                # Vert
                VertStr = "{0:.{1}f},".format(VertVal, linPrecision)
                # 3d Maj RR
                threeDMajRRStr = "{0:.{1}f},".format(threeDMajRRVal, linPrecision)
                # 2d Maj RR
                twoDMajRRStr = "{0:.{1}f},".format(twoDMajRRVal, linPrecision)
                # Vert RR
                VertRRStr = "{0:.{1}f}".format(VertRRVal, linPrecision)
                # Store finished row of data
                dataList.append([posStr,latStr, lonStr, latDecStr, lonDecStr, eht, oht, threeDMajStr, twoDMajStr, VertStr, threeDMajRRStr, twoDMajRRStr, VertRRStr,"\n"])
            
        return columnHeaders, dataList;

    def readCfgData(self):
        with open(self.cfgFileName, 'r') as cfgFile:
            lines = cfgFile.readlines()
        for n in lines:
            line_args = n.split()
            if not line_args:
                continue
            elif line_args[0] == "--linprecP":
                self.linPrecision = int(line_args[1])
            elif line_args[0] == "--angprecP":
                self.angPrecision = int(line_args[1])
            elif line_args[0] == "--westLon":
                self.longitudeWest = True
            elif line_args[0] == "--APV":
                self.APVflag = True
            elif line_args[0] == "--includeUnused":
                self.includeUnused = True
            elif line_args[0] == "--confid":
                if line_args[1] == "1sig":
                    self.confidence = 68
                else:
                    self.confidence = line_args[1]

    def writeToPcr(self):
        try:
            with open(self.outFileName, 'w') as outFile:
                columnHeaders, dataList = self.readH5File()
                dataList.sort(key=lambda tup: tup[0]) #sort list alphabetically by station name  
                outFile.writelines(columnHeaders)
                for data in dataList:
                    dataString = map(str, data)
                    outFile.writelines(dataString)

        except EnvironmentError:
            sys.stderr.write('h5PCR.py: ERROR: Unable to find/write {0}. Exiting.\n'.format(self.outFileName))
            exit(1)
        except RuntimeError as error:
            sys.stderr.write('h5PCR.py: Parsing error. Unexpected value found.')
            sys.stderr.write(error)
            exit(1)


    def toStr(self, notString):
        if(type(notString) == list):
            return " ".join(notString)

    def byteArrayToStr(self, bItem):
        pos = []
        for j in bItem:
            pos.append(j.decode())
        pos = self.toStr(pos)
        return pos

def writeSalsaOutBegin():
    script = "h5PCR"
    version = "1.0 10/25/23"
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
    f3 = "h5PCR timing: {0:4.3f} seconds. ({1:4.3f} sec)\n"
    print(f3.format(timeLapse, round(timeLapse)))
    
def generatePCR(h5file, cfgfile, outfile):
    startTime = time.time()
    writeSalsaOutBegin()

    writeSalsaOutputFile(outfile)

    if h5file == None:
        sys.stderr.write('h5PCR.py: No input h5 file specified.\n')
        parser.print_help()
        # exit(1)
    if cfgfile == None:
        sys.stderr.write('h5PCR.py: No input config file specified.\n')
        parser.print_help()
        # exit(1)
    #if no output path is specified then place outfile in same directory as .h5 file
    if outfile == None:
        outfile = string.replace(h5file, ".pcr")
    if not os.path.exists(h5file):
        sys.stderr.write('h5PCR.py: Could not locate input h5 file %s. Exiting. ' % h5file)
        # exit(1)
    if not os.path.exists(cfgfile):
        sys.stderr.write('h5PCR.py: Could not locate input config file %s. Exiting. ' % h5file)
        # exit(1)

    converter = h5PCRconverter(h5file, cfgfile, outfile)


    try:
        converter.readCfgData()
        converter.writeToPcr()
    except:
        sys.stderr.write(" ERROR generating PCR file")
        sys.stderr.write(traceback.format_exc())
    finally:
        endTime = time.time()
        writeSalsaOutFinish(startTime, endTime)


#code execution starts here
if __name__ == '__main__':
    
    
    writeSalsaOutBegin()

    parser = arg.ArgumentParser(usage="")
    parser.add_argument('--h5', action='store', dest='h5file', required=True)
    parser.add_argument('--cfg', action='store', dest='cfgfile', required=True)
    parser.add_argument('--pcr', action='store', dest='outfile', required=False)

    args = parser.parse_args()
    writeSalsaOutputFile(args.outfile)

    if args.h5file == None:
        sys.stderr.write('h5PCR.py: No input h5 file specified.\n')
        parser.print_help()
        exit(1)
    if args.cfgfile == None:
        sys.stderr.write('h5PCR.py: No input config file specified.\n')
        parser.print_help()
        exit(1)
    #if no output path is specified then place outfile in same directory as .h5 file
    if args.outfile == None:
        args.outfile = os.path.splitext(args.h5file)[0] + '.pcr'
    if not os.path.exists(args.h5file):
        sys.stderr.write('h5PCR.py: Could not locate input h5 file %s. Exiting. ' % args.h5file)
        exit(1)
    if not os.path.exists(args.cfgfile):
        sys.stderr.write('h5PCR.py: Could not locate input config file %s. Exiting. ' % args.h5file)
        exit(1)

    converter = h5PCRconverter(args.h5file, args.cfgfile, args.outfile)


    try:
        converter.readCfgData()
        converter.writeToPcr()
    except:
        sys.stderr.write(" ERROR generating PCR file")
        sys.stderr.write(traceback.format_exc())
    finally:
        endTime = time.time()
        writeSalsaOutFinish(startTime, endTime)


    
