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
import subprocess
import re
import sys
import numpy as np
import h5py
import converterUtil as util
import lsaversion as ver
import math as m
import traceback

global isEGM08
global isEGM96

class h5CSVconverter(object):
    def __init__(self, h5FileName, cfgFileName, outFileName, cgeoid, geoFile2Name, apriori):
        self.h5FileName = h5FileName
        self.cfgFileName = cfgFileName
        self.outFileName = outFileName
        self.cgeoidInputFileName = self.h5FileName[:-3] + ".cgi"
        self.cgeoidOutputFileName = self.h5FileName[:-3] + ".cgo"
        self.llhData = 0 
        self.longitudeWest = False
        self.linPrecision = 0
        self.angPrecision = 0
        self.APVflag = False
        self.includeUnused = False
        self.geoidFile1Name = ""
        self.geoidFile2Name = geoFile2Name
        self.geoidHeightsFromApriori = apriori
        self.cgeoid = cgeoid


    def computeUndulationsForSecondGeoid(self,rdata):
        lats = []
        lons = []
        egmUnd = []
        for i in range(len(rdata)):
            # Is this point used
            ptUtilized = rdata[i]["Utilized"][0]

            if self.includeUnused or ptUtilized: 
                # Latitude
                lats.append(float(rdata[i]["Latitude"][0]))
                # Longitude
                lons.append(float(rdata[i]["Longitude"][0]))
        maxLat = max(lats)
        minLat = min(lats)
        maxLon = max(lons)
        minLon = min(lons)

        #create the cgeoid input file
        with open(self.cgeoidInputFileName, 'w') as cgeoidInput:
            cgeoidInput.write("--latmin {0}\n".format(str(minLat)))
            cgeoidInput.write("--latmax {0}\n".format(str(maxLat)))
            cgeoidInput.write("--lonmin {0}\n".format(str(minLon)))
            cgeoidInput.write("--lonmax {0}\n".format(str(maxLon)))
            cgeoidInput.write("--gfile \"{0}\"\n".format(self.geoidFile2Name))
            for i in range(len(lats)):
                cgeoidInput.write("--pos {0}:{1}\n".format(str(lats[i]),str(lons[i])))

        # build up the list of args to pass to cgeoid
        args = []
        args.append(self.cgeoid)
        args.append("--file")
        args.append(self.cgeoidInputFileName)
        args.append("--log")
        args.append(self.cgeoidOutputFileName)

        try:
            process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()

            #read the cgeoid output file
            with open(self.cgeoidOutputFileName, 'r') as cgeoidOutput:
                lines = cgeoidOutput.read().splitlines()
            for line in lines:
                linesplt=line.split()
                pos=0
                try:
                    pos=linesplt.index('DOV')
                except ValueError:
                    pass
                if pos>1:
                    egmUnd.append(linesplt[pos-1])

            #remove temporary input and output files
            os.remove(self.cgeoidInputFileName)
            os.remove(self.cgeoidOutputFileName)

            return egmUnd
        except IOError:
            sys.stderr.write("An error occurred with cgeoid")


    def readLLHData(self):
        #open file
        with h5py.File(self.h5FileName, "r") as h5file:
            #open group
            pointsGroup = h5file["Adjustment Results/Points"]
            #open dataset
            if self.geoidHeightsFromApriori:
                dset = pointsGroup["Initial LLH"]
            else:
                dset = pointsGroup["LLH"]
            numOfEntries = len(dset)
            # numOfLabels = len(dset[0][0])
            rdata = dset[...]

            #open statistics for APV
            statGroup = h5file["Adjustment Results/Statistics"]
            statData = statGroup[...]

            APV = statData["APV"]

        linPrecision = self.linPrecision
        angPrecision = self.angPrecision
        decDegreePrecision = 9
        latDir = "N"
        lonDir = "E"
        self.llhData = rdata
        dataList = []
        secondUndList = []
        if self.geoidFile2Name:
            secondUndList = self.computeUndulationsForSecondGeoid(rdata)

        usedCounter = 0
        for i in range(len(rdata)):
            # Is this point used
            ptUtilized = rdata[i]["Utilized"][0]

            if self.includeUnused or ptUtilized: 
                # Station
                postemp = rdata[i]["Point Label"]
                pos = []
                for j in postemp:
                    pos.append(j.decode())

                # Latitude
                if rdata[i]["Latitude"] < 0.:
                    latDir = "S"
                    lat = util.decDegToDMS(util.northLatitudeToSouth(rdata[i]["Latitude"]), angPrecision)
                else:
                    latDir = "N"
                    lat = util.decDegToDMS(rdata[i]["Latitude"], angPrecision)
                latD = util.formatInt(lat[0])
                latM = util.formatInt(lat[1])
                latS = util.formatFloat(lat[2], angPrecision)

                latDecStr = util.formatFloat(rdata[i]["Latitude"], decDegreePrecision)


                # Longitude 
                #DMS Must be 0<= lon <= 360 (relative to lonDir)
                if (self.longitudeWest):
                    lon = util.eastLongitudeToWest(rdata[i]["Longitude"])
                    lonDir = "W"
                else:
                    lon = rdata[i]["Longitude"]
                    lonDir = "E"
                lon = util.decDegToDMS(lon, angPrecision)
                lonD = util.formatInt(lon[0])
                lonM = util.formatInt(lon[1])
                lonS = util.formatFloat(lon[2], angPrecision)

                #DecDeg Must be -180 <= lon <= 180 (defined as East longitude)
                londec = util.under180(rdata[i]["Longitude"])
                lonDecStr = util.formatFloat(londec, decDegreePrecision)


                # Ellipsoidal Height
                ehtStr = util.formatFloat(rdata[i]["Height"], linPrecision)

                # Orthometric Height (look betlow Sigma U)

                # Sigma N
                sigNStr = util.formatFloat(np.sqrt(APV)*rdata[i]["sigma North"], linPrecision)

                # Sigma E
                sigEStr = util.formatFloat(np.sqrt(APV)*rdata[i]["sigma East"], linPrecision)

                # Sigma U
                sigUStr = util.formatFloat(np.sqrt(APV)*rdata[i]["sigma Up"], linPrecision)


                # Orthometric Height
                if(self.geoidFile1Name != ''): ohtStr = util.formatFloat(rdata[i]["Oht"], linPrecision)
                else: ohtStr = "?"
                
                # Second Geoid
                try: geoid2name = re.split("\/",self.geoidFile2Name)[-1]
                except TypeError: geoid2name=''
                if ((geoid2name == "egm1996_2.5m.und") and isEGM08):
                    oht2 = rdata[i]["Height"] - float(secondUndList[usedCounter])

                    oht2Str = util.formatFloat(oht2, linPrecision)

                    dataList.append([pos, latDir, latD, latM, latS, latDecStr, lonDir, lonD, lonM, lonS, lonDecStr, ehtStr, ohtStr, oht2Str, sigNStr, sigEStr, sigUStr])
                    usedCounter = usedCounter + 1
                elif (isEGM96):
                    dataList.append([pos, latDir, latD, latM, latS, latDecStr, lonDir, lonD, lonM, lonS, lonDecStr, ehtStr, "?", ohtStr, sigNStr, sigEStr, sigUStr])
                else:
                    dataList.append([pos, latDir, latD, latM, latS, latDecStr, lonDir, lonD, lonM, lonS, lonDecStr, ehtStr, ohtStr, "?", sigNStr, sigEStr, sigUStr])

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
            if line_args[0] == "--angprecP":
                self.angPrecision = int(line_args[1])
            if line_args[0] == "--westLon":
                self.longitudeWest = True
            if line_args[0] == "--includeUnused":
                self.includeUnused = True
            if line_args[0] == "--geoidfile":
                self.geoidFile1Name = line_args[1]
            global isEGM08
            isEGM08 = (self.geoidFile1Name=="egm2008_2.5m.und")
            global isEGM96
            isEGM96 = (self.geoidFile1Name=="egm1996_2.5m.und")

    def writeToCSVFile(self):
        try:
            with open(self.outFileName,'w') as outFile:
                dataList = self.readLLHData()
                if (isEGM08 or isEGM96):
                    outFile.write("Station,LatHem,Lat_deg,Lat_min,Lat_sec,Lat_decdeg,LonHem,Lon_deg,Lon_min,Lon_sec,Lon_decdeg,EllipHt_m,OrthHt08_m,OrthHt96_m,sigmaN_m,sigmaE_m,sigmaU_m\n")
                else:
                    outFile.write("Station,LatHem,Lat_deg,Lat_min,Lat_sec,Lat_decdeg,LonHem,Lon_deg,Lon_min,Lon_sec,Lon_decdeg,EllipHt_m,OrthHt_m,OrthHt_m,sigmaN_m,sigmaE_m,sigmaU_m\n")
                dataList.sort(key=lambda tup: tup[0]) #sort list alphabetically by station name 
                for data in dataList:
                    pointlabel = " ".join(data[0])
                    dataString = map(str, data[1:])
                    outFile.write(pointlabel + ',' + self.cleanData(dataString) + "\n")
        except PermissionError:
            sys.stderr.write('h5CSV.py: ERROR: Cannot to write to %s.\n' % self.outFileName)
        except EnvironmentError:
            sys.stderr.write('h5CSV.py: ERROR: Unable to find/write {0}. Exiting.\n'.format(self.outFileName))
        except RuntimeError as error:
            sys.stderr.write('h5CSV.py: Parsing error. Unexpected value found.')
            print(error) 

    def cleanData(self, dataString):
        # newString = (''.join(dataString)).replace(",", "").replace("'","").replace("[", "").replace("]","")
        # return(newString.replace(" ",","))
        return ','.join(dataString)

def writeSalsaOutBegin():
    script = "h5CSV"
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
    f3 = "hCSV timing: {0:4.3f} seconds. ({1:4.3f} sec)\n"
    print(f3.format(timeLapse, round(timeLapse)))
    
def generateCSV(h5file, cfgfile, outfile, cgeoid, geoidfile2, apriori):
    writeSalsaOutBegin()
    writeSalsaOutputFile(outfile)


    converter = h5CSVconverter(h5file, cfgfile, outfile, cgeoid, geoidfile2, apriori)
    
    try:
        converter.readCfgData()
        converter.writeToCSVFile()    
    except:
        sys.stderr.write(" ERROR generating CSV file\n")
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
    parser.add_argument('--csv', action='store', dest='outfile', required=False)
    parser.add_argument('--cgeoid', action='store', dest='cgeoid', required=False)
    parser.add_argument('--geo', action='store', dest='geoidfile2', required=False)
    parser.add_argument('--apriori', dest='apriori', action='store_true')
    parser.set_defaults(apriori=False)

    args = parser.parse_args()

    if args.h5file == None:
        sys.stderr.write('h5CSV.py: No input h5 file specified.\n')
        parser.print_help()
        exit(1)
    if args.cfgfile == None:
        sys.stderr.write('h5CSV.py: No input config file specified.\n')
        parser.print_help()
        exit(1)
    #if no output path is specified then place outfile in same directory as .h5 file
    if args.outfile == None:
        args.outfile = args.h5file[:-3] + ".csv"
    if not os.path.exists(args.h5file):
        sys.stderr.write('h5CSV.py: Could not locate input h5 file %s. Exiting. ' % args.h5file)
        exit(1)
    if not os.path.exists(args.cfgfile):
        sys.stderr.write('h5CSV.py: Could not locate input config file %s. Exiting. ' % args.cfgfile)
        exit(1)
    if args.geoidfile2 != None:
        if not os.path.exists(args.geoidfile2):
            sys.stderr.write('h5CSV.py: Could not locate geoid file %s. Exiting. ' % args.geoidfile2)
            exit(1)
    if args.cgeoid != None:
        if not os.path.exists(args.cgeoid):
            sys.stderr.write('h5CSV.py: Could not locate cgeoid executable %s. Exiting. ' % args.cgeoid)
            exit(1)

    writeSalsaOutputFile(args.outfile)
    converter = h5CSVconverter(args.h5file, args.cfgfile, args.outfile, args.cgeoid, args.geoidfile2, args.apriori)

    try:
        converter.readCfgData()
        converter.writeToCSVFile()    
    except:
        sys.stderr.write(" ERROR generating CSV file\n")
        sys.stderr.write(traceback.format_exc())
    finally:
        endTime = time.time()
        writeSalsaOutFinish(startTime, endTime)

