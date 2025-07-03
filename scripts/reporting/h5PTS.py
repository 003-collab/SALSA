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

class h5PTSconverter(object):

    def __init__(self, h5File, cfgFile, outFile):
        self.h5FileName = h5File
        self.cfgFileName = cfgFile
        self.outFileName = outFile
        self.h5FileVersion = 0 
        self.longitudeWest = False
        self.linPrecision = 0
        self.angPrecision = 0
        self.APVflag = False
        self.includeUnused = False
        self.geoidFile = ""
        self.confidence = 0

    def readH5File(self):        
        # open file
        with h5py.File(self.h5FileName, "r") as h5file:
            #read file Version
            self.h5FileVersion = h5file.attrs["H5 Version"]

            #read Project Configuration dataset
            self.geoidFile = h5file["Project Configuration"]["Geoid File Name"][0].decode()

            # open group
            pointsGroup = h5file["Adjustment Results/Points"]
            # open LLH dataset
            dset = pointsGroup["LLH"]
            numOfEntries = len(dset)
            rdata = dset[...]

            # open statistics for APV
            statGroup = h5file["Adjustment Results/Statistics"]
            statData = statGroup[...]

            APV = statData["APV"][0]
            #check if data needs to be scaled by APV
            if self.APVflag:
                APV = m.sqrt(APV)
            else:
                APV = 1

        linPrecision = self.linPrecision
        angPrecision = self.angPrecision
        secSpace = angPrecision + 3

        # new list for prepping output
        dataList = []

        # dataset labels
        if(self.longitudeWest == True):
            direction = "W"
        else:
            direction = "E"
        labelFormat = " {0:10s}{1:>14s}{2:>16s}{3:>10s}{4:>10s}{5:>8s}{6:>8s}{7:>8s}\n"
        dataList.append(labelFormat.format("Position","Latitude N","Longitude " + direction,"EHt(m)","OHt(m)","sigN(m)","sigE(m)","sigU(m)"))

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
                fixedNum = int(rdata[i]["Fixed Type"][0])
                myFixedType = util.FixedType(fixedNum).name

                # Position
                pos = self.byteArrayToStr(rdata[i]["Point Label"])
                posStr = " {0:{1}s}".format(pos, maxPointWidth+3)

                # Latitude
                lat = util.decDegToDMS(rdata[i]["Latitude"][0], angPrecision)
                lat[2] = round(lat[2], angPrecision) + 0
                if type(lat[0]) is str:
                    latStr = "{l[0]} {l[1]:2d} {l[2]:{1}.{0}f}".format(angPrecision, secSpace,l=lat)
                else:
                    latStr = "{l[0]:2d} {l[1]:2d} {l[2]:{1}.{0}f}".format(angPrecision, secSpace,l=lat)

                # Longitude
                if self.longitudeWest == True:
                    lon = util.eastLongitudeToWest(rdata[i]["Longitude"])
                else:
                    lon = rdata[i]["Longitude"]

                lon = util.decDegToDMS(lon, angPrecision)
                lon[2] = round(lon[2], angPrecision) + 0
                if type(lon[0]) is str:
                    lonStr = "{l[0]} {l[1]:2d} {l[2]:{1}.{0}f}".format(angPrecision, secSpace,l=lon)
                else:
                    lonStr = "{l[0]:4d} {l[1]:2d} {l[2]:{1}.{0}f}".format(angPrecision, secSpace,l=lon)

                # Ellipsoidal Height
                eht = util.rmNegZero(rdata[i]["Height"][0], linPrecision)
                eht = "{0:10.{1}f}".format(eht, linPrecision)

                # Orthemetic Height
                oht = util.rmNegZero(rdata[i]["Oht"][0], linPrecision)
                oht = "{0:10.{1}f}".format(oht, linPrecision)

                if("FIXED" == myFixedType):
                    # Store finished row of data
                    dataList.append([posStr,latStr, lonStr, eht, oht, "\n"])
                else:
                   # Sigma N
                   sigNVal = float(APV*rdata[i]["sigma North"][0])
                   sigNStr = "{0:8.{1}f}".format(sigNVal, linPrecision)
                    
                    # Sigma E
                   sigEVal = float(APV*rdata[i]["sigma East"][0])
                   sigEStr = "{0:8.{1}f}".format(sigEVal, linPrecision)
                   
                   # Sigma U
                   sigUVal = float(APV*rdata[i]["sigma Up"][0])
                   sigUStr = "{0:8.{1}f}".format(sigUVal, linPrecision)

                   dataList.append([posStr,latStr, lonStr, eht, oht, sigNStr, sigEStr, sigUStr,"\n"])
            
        return dataList;

    def readCfgData(self):
        with open(self.cfgFileName, 'r') as cfgFile:
            lines = cfgFile.readlines()
        for n in lines:
            line_args = n.split()
            if not line_args:
                continue
            if line_args[0] == "--linprecP":
                self.linPrecision = int(line_args[1])
            if line_args[0] == "--angprecP":
                self.angPrecision = int(line_args[1])
            if line_args[0] == "--westLon":
                self.longitudeWest = True
            if line_args[0] == "--APV":
                self.APVflag = True
            if line_args[0] == "--includeUnused":
                self.includeUnused = True
            if line_args[0] == "--confid":
                if line_args[1] == "1sig":
                    self.confidence = 68
                else:
                    self.confidence = line_args[1]

    def writeHeader(self, outFile):
        # header info
        outFile.writelines("# Points file generated by h5PTS.py, SALSA Version " +
                           lsaversion.LSAVERSION_MAJOR_MINOR_PATCH + "\n")
        outFile.writelines("# Input HDF5 file: {0} version {1}\n".format(self.h5FileName, self.h5FileVersion))
        if self.geoidFile == "":
            outFile.writelines("# No geoid file input (Eht = Oht)\n")
        else:
            outFile.writelines("# geoid: {0}\n".format(self.geoidFile))
        if self.APVflag == True:
            outFile.writelines("# Confidence ellipse is scaled with APV\n\n")
        else:
            outFile.writelines("# Confidence ellipse is not scaled with APV\n\n")
        outFile.writelines(" Final Positions with Confidence Ellipse (m) (confidence level {0}%)\n".format(self.confidence))
 

    def writeToPts(self):
        try:
            with open(self.outFileName, 'w') as outFile:
                dataList = self.readH5File()
                self.writeHeader(outFile)
                dataList.sort(key=lambda tup: tup[0]) #sort list alphabetically by station name  
                for data in dataList:
                    dataString = map(str, data)
                    outFile.writelines(dataString)

        except EnvironmentError:
            sys.stderr.write('h5PTS.py: ERROR: Unable to find/write {0}. Exiting.\n'.format(self.outFileName))
            exit(1)
        except RuntimeError as error:
            sys.stderr.write('h5PTS.py: Parsing error. Unexpected value found.')
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
    script = "h5PTS"
    version = "1.0 8/05/16"
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
    f3 = "h5PTS timing: {0:4.3f} seconds. ({1:4.3f} sec)\n"
    print(f3.format(timeLapse, round(timeLapse)))
    
def generatePTS(h5file, cfgfile, outfile):
    startTime = time.time()
    writeSalsaOutBegin()

    writeSalsaOutputFile(outfile)

    if h5file == None:
        sys.stderr.write('h5PTS.py: No input h5 file specified.\n')
        parser.print_help()
        # exit(1)
    if cfgfile == None:
        sys.stderr.write('h5PTS.py: No input config file specified.\n')
        parser.print_help()
        # exit(1)
    #if no output path is specified then place outfile in same directory as .h5 file
    if outfile == None:
        outfile = string.replace(h5file, ".pts")
    if not os.path.exists(h5file):
        sys.stderr.write('h5PTS.py: Could not locate input h5 file %s. Exiting. ' % h5file)
        # exit(1)
    if not os.path.exists(cfgfile):
        sys.stderr.write('h5PTS.py: Could not locate input config file %s. Exiting. ' % h5file)
        # exit(1)

    converter = h5PTSconverter(h5file, cfgfile, outfile)


    try:
        converter.readCfgData()
        converter.writeToPts()
    except:
        sys.stderr.write(" ERROR generating PTS file")
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
    parser.add_argument('--pts', action='store', dest='outfile', required=False)

    args = parser.parse_args()
    writeSalsaOutputFile(args.outfile)

    if args.h5file == None:
        sys.stderr.write('h5PTS.py: No input h5 file specified.\n')
        parser.print_help()
        exit(1)
    if args.cfgfile == None:
        sys.stderr.write('h5PTS.py: No input config file specified.\n')
        parser.print_help()
        exit(1)
    #if no output path is specified then place outfile in same directory as .h5 file
    if args.outfile == None:
        args.outfile = os.path.splitext(args.h5file)[0] + '.pts'
    if not os.path.exists(args.h5file):
        sys.stderr.write('h5PTS.py: Could not locate input h5 file %s. Exiting. ' % args.h5file)
        exit(1)
    if not os.path.exists(args.cfgfile):
        sys.stderr.write('h5PTS.py: Could not locate input config file %s. Exiting. ' % args.h5file)
        exit(1)

    converter = h5PTSconverter(args.h5file, args.cfgfile, args.outfile)


    try:
        converter.readCfgData()
        converter.writeToPts()
    except:
        sys.stderr.write(" ERROR generating PTS file")
        sys.stderr.write(traceback.format_exc())
    finally:
        endTime = time.time()
        writeSalsaOutFinish(startTime, endTime)


    
