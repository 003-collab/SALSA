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
#! python3

#standary library imports
from datetime import datetime
#python imports
import sys
import os
import math
import collections
#LSA imports
from lsaconverter import LSAConverter
from lsaconverter import Config

class SetsOfAngles(LSAConverter):

    SUPPORTED_VERSIONS = ['2.04', '2.22', '2.24', '4.50','5.00', '5.05', '5.61', '5.70', '8.02','8.03']
    SUPPORTED_INSTRUMENTS = ['TS15', 'TS30', 'TCRA1101', 'TCRA1101plus', 'TCA2003', 'TM5100A']
    instrumentType = None #Needed to make these global because UNCR records for distance generated in addCurrentMeasurement
    have_DIST_IR_UNCR = False #Needed to make these global because UNCR records for distance generated in addCurrentMeasurement
    have_DIST_RED_UNCR = False #Needed to make these global because UNCR records for distance generated in addCurrentMeasurement
    have_DIST_NON_UNCR = False #Needed to make these global because UNCR records for distance generated in addCurrentMeasurement

    #Implementation of abstract methods
    def writeOutputFile(self):
        includes = []
        heights = []
        uncertainties = []
        past_header = False
        datestr = datetime.now().strftime("%H:%M:%S %b %d, %Y")
        try:
            with open(self.outfile_name,'w') as f:
                f.write("Created by SALSA version {0}\n".format(self.SALSA_VERSION_STRING))
                f.write("# Converted with the {0} converter from {1} on {2}\n".format(self.converter_string,self.infile_name,datestr))
                for record in self.output_records:
                    out_string = record.get_lsa_string()
                    if out_string[:10] == "--include ":
                        includes.append(out_string)
                    if out_string[0:5] == "HGHT ":
                        heights.append(out_string)
                    if out_string[0:5] == "UNCR ":
                        uncertainties.append(out_string)
                if len(includes) > 1:#main output file is just a list of child includes
                    f.write("\n")
                    for uncertainty in uncertainties:
                        f.write("{0}\n".format(uncertainty))
                    f.write("\n")
                    for height in heights:
                        f.write("{0}\n".format(height))
                    f.write("\n")
                    for include in includes:
                        f.write("{0}\n".format(include))
                else:#main output file only has one set - write it out, omitting the child include record
                    for record in self.output_records:
                        out_string = record.get_lsa_string()
                        if "--include" not in out_string:
                            f.write("{0}\n".format(out_string))
            #write out the child include files
            if len(includes) > 1:
                output_line = 0
                for include in includes:
                    include_file_name = include[10:]
                    if include_file_name.startswith("\"") and include_file_name.endswith("\""):
                        include_file_name = include_file_name[1:-1]
                    include_file_name = os.path.join(os.path.dirname(self.outfile_name),include_file_name)
                    with open(include_file_name,'w') as f:
                        f.write("Created by SALSA version {0}\n".format(self.SALSA_VERSION_STRING))
                        f.write("# Converted with the {0} converter from {1} on {2}\n\n".format(self.converter_string,self.infile_name,datestr))
                        ctr=0
                        for record in self.output_records[output_line:]:
                            out_string = record.get_lsa_string()
                            if out_string[:5] == "DGRP ":
                                past_header = True
                            #check for new set
                            if ((("Sets of Angles" in out_string) and (out_string[:14] != "Sets of Angles"))
                                and past_header):
                                    past_header = False
                                    break#start new include
                            elif (out_string[:10] != "--include ") and (out_string[0:5] != "HGHT ") and (out_string[0:5] != "UNCR "):
                                f.write("{0}\n".format(out_string))
                            ctr += 1
                        output_line += ctr

        except EnvironmentError:
            print("Error - unable to find/write {0}. Exiting.\n".format(self.outfile_name))
            exit(1)
    
    def getVersion(self):
        ctr=0
        for line in self.input_lines:
            ctr = ctr + 1
            if ("Logfile" in line) and ("Sets of Angles" in line):
                parts = line.split()            
                versionStr = parts[len(parts)-2]
                return versionStr
            if ctr > 100:
                break
        ctr=0
        for line in self.input_lines:
            ctr = ctr + 1
            if "Sets of Angles V " in line:
                parts = line.split()            
                versionStr = parts[len(parts)-1]
                return versionStr
            if ctr > 100:
                return None

    def isValid(self):
        for line in self.input_lines:
            if "Sets of Angles" in line:
                return True
        return False

    def iterateIncludeFileName(self,include_file_name, include_file_names):
        """handle the case where the same station is used for multiple sets"""
        iteration=2
        new_include_file_name = include_file_name
        while new_include_file_name in include_file_names:
            #expect name to be projectname_station.lsa
            parts = new_include_file_name.split('_')
            last_part = parts[len(parts)-1]
            pieces = last_part.split('.')
            if len(pieces) > 2:#previously iterated
                last_iteration = int(pieces[1])
                iteration = last_iteration+1
            new_include_file_name = include_file_name[:include_file_name.rfind('.')] + '.' + str(iteration) + '.lsa'
        return new_include_file_name

    def parseInstrumentType(self,line):
        parts = line.split(':')
        pieces = parts[1].split()
        InstrumentType = pieces[0].lstrip()
        if InstrumentType not in self.SUPPORTED_INSTRUMENTS:
            self.write_warning("Warning - {0} is not in the list of supported Leica Total Stations: {1}".format(InstrumentType,str(self.SUPPORTED_INSTRUMENTS)))
        return InstrumentType

    def parseInstrumentType204(self,line):
        parts = line.split(':')
        pieces = parts[1].split(',')
        InstrumentType = pieces[0].lstrip()
        if InstrumentType not in self.SUPPORTED_INSTRUMENTS:
            self.write_warning("Warning - {0} is not in the list of supported Leica Total Stations: {1}".format(InstrumentType,str(self.SUPPORTED_INSTRUMENTS)))
        return InstrumentType

    def parseFromStation(self,line):
        parts = line.split(':')
        FromStation = parts[1].lstrip().rstrip()
        return FromStation

    def parseFromHeight204(self,line):
        parts = line.split('=')
        FromHeight = parts[2].lstrip().replace('\n','').replace('\r','')[:-1]#remove units
        floatFromHeight = float(FromHeight)
        return floatFromHeight

    def parseFromHeight(self,line):
        parts = line.split('=')
        FromHeight = parts[1].lstrip()
        floatFromHeight = float(FromHeight)
        #round to tenth of a millimeter (noise at end results in duplicate HGHT records)
        return round(floatFromHeight,4)

    def parseNumPoints(self,line):
        parts = line.split(':')
        numPoints = parts[2].lstrip()
        return int(numPoints)

    def parseToStation(self,line):
        return self.parseFromStation(line)

    def parseToStationFromSingleSet(self,line):
        parts = line.split(':')
        ToStation = parts[2].lstrip().rstrip()
        return ToStation

    def parseToHeight(self,line):
        parts = line.split(':')
        pieces = parts[1].split()
        return float(pieces[0])

    def parseToHeight204(self,line):
        parts = line.split(':')
        ToHeightStr = parts[1].lstrip().replace('\n','').replace('\r','')[:-1]#remove units
        floatToHeight = float(ToHeightStr)
        return floatToHeight

    def parseAngle(self,line):
        angle = line.split(':')[1].lstrip().rstrip()
        parts = angle.split('.')
        degree = int(parts[0])
        minute = int(parts[1][0:2])
        secondStr = parts[1][2:]
        second = float(secondStr)
        num_second_decimals = len(secondStr) - 2
        if num_second_decimals >= 1:
            second /= float(10**num_second_decimals)
        return (degree,minute,second)

    def parseAngle2XX(self,line,version):
        angle = line.split(':')[1].lstrip().rstrip()# NNN NN'NN"
        if version == "2.04":
            parts = angle.split()
            degree = int(parts[0])
            pieces = parts[1].split("'")
            minute = int(pieces[0])
            secondStr = pieces[1][:-1]#remove "
            second = float(secondStr)
        elif (version == "2.22") or (version == "2.24"):
            parts = angle.split("'")
            secondStr = parts[1][:-1]#remove "
            second = float(secondStr)
            minute = int(parts[0][len(parts[0])-2:len(parts[0])])
            degree = int(parts[0][:len(parts[0])-3])
        return (degree,minute,second)

    def parseDistance(self,line):
        parts = line.split(':')
        distance = parts[1].lstrip()
        return float(distance)

    def parseAngularSigma(self,line):
        parts = line.split(':')
        sigmaStr = parts[1].lstrip()
        degreeStr = sigmaStr.split('.')[0]
        degree = int(degreeStr)
        minuteStr = sigmaStr.split('.')[1][0:2]
        minute = int(minuteStr)
        secondStr = sigmaStr.split('.')[1][2:].replace('\n','').replace('\r','')
        second = float(secondStr)
        num_second_decimals = len(secondStr) - 2
        if num_second_decimals >= 1:
            second /= float(10**num_second_decimals)
        sigma = 3600*degree + 60*minute + second
        return sigma

    def parseAngularSigma2XX(self,line,version):
        parts = line.split(':')
        sigmaStr = parts[1].lstrip()
        if version == "2.04":
            degreeStr = sigmaStr.split()[0]
            degree = int(degreeStr)
            minuteStr = sigmaStr.split()[1][0:2]
            minute = int(minuteStr)
            secondStr = sigmaStr.split()[1].split("'")[1].replace('\n','').replace('\r','')[:-1]#remove "
            second = float(secondStr)
        elif (version == "2.22") or (version == "2.24"):
            parts = sigmaStr.split("'")
            secondStr = parts[1].replace('\n','').replace('\r','')[:-1]#remove "
            second = float(secondStr)
            minute = int(parts[0][len(parts[0])-2:len(parts[0])])
            degree = int(parts[0][:len(parts[0])-3])
        sigma = 3600*degree + 60*minute + second
        return sigma

    def parseNumSetsNumPoints(self,line):
        parts = line.split(':')
        numSetsStr = parts[1].lstrip().split(' ')[0]
        numPointsStr = parts[2].lstrip().rstrip()
        numSets = int(numSetsStr)
        numPoints = int(numPointsStr)
        return (numSets,numPoints)

    def parseToStationFromSingleSet(self,line):
        parts = line.split(':')
        toStation = parts[2].lstrip().rstrip()
        return toStation

    def parseMeanOfFaceAngle(self,line):
        angle = line.split(':')[1].lstrip().split(' ')[0]
        parts = angle.split('.')
        degree = int(parts[0])
        minute = int(parts[1][0:2])
        secondStr = parts[1][2:]
        second = float(secondStr)
        num_second_decimals = len(secondStr) - 2
        if num_second_decimals >= 1:
            second /= float(10**num_second_decimals)
        return (degree,minute,second)

    def parseReducedMean(self,line):
        return self.parseMeanOfFaceAngle(line)

    def parseMeanOfFaceDistance(self,line):
        distanceStr = line.split(':')[1].lstrip().split(' ')[0]
        return float(distanceStr)

    def parseDistanceType(self,line):
        parts = line.split(':')
        EDM_type = parts[5].lstrip().rstrip()
        if EDM_type == "No EDM Typ":
            return "DIST_NON"
        elif EDM_type == "Infrared D":
            return "DIST_IR"
        elif EDM_type == "Red Laser":
            return "DIST_RED"
        else:
            self.write_warning("Warning - unknown EDM Type in line: {0}".format(line))
            return "DIST_NON"

    def parseDistanceSigma(self,line):
        #expect line = Standrad Deviation Of Single Distance:     0.0001
        parts = line.split(':')
        sigmaStr = parts[1].lstrip().rstrip()
        sigma = float(sigmaStr)
        return sigma

    def computeSigmas(self,meanAngles,averageAngles,angle_sigma):
        sigmas = collections.OrderedDict()
        for key,value in meanAngles.items():
            sum = 0.0
            ddmeanAngle = value[0] + value[1]/60.0 + value[2]/3600.0
            #sum the square of the differences from the mean
            try:
                thisAngle = averageAngles[key]
                for angle in thisAngle:
                    ddavgAngle = angle[0] + angle[1]/60.0 + angle[2]/3600.0
                    sum += math.pow(ddmeanAngle-ddavgAngle,2.0)
            except:
                self.write_warning("Warning - in computeSigmas, no average angles for {0}".format(key))
                continue
            N = len(averageAngles[key])
            #if only 1 set or sigma less than 1 soa, default to sigma of all measurements
            if N <= 1:
                sigmas[key] = angle_sigma
            else:
                sum = sum/float(N)
                sigmas[key] = math.sqrt(sum) * 3600.0 #convert from decimal degrees to soa
                if sigmas[key] < 1.0:
                    sigmas[key] = angle_sigma
        return sigmas

    def computeDistanceSigmas(self,meanDistances,averageDistances,distance_sigma):
        sigmas = collections.OrderedDict()
        for key,value in meanDistances.items():
            sum = 0.0
            #sum the square of the differences from the mean
            try:
                thisDistances = averageDistances[key]
                for distance in thisDistances:
                    sum += math.pow(value - distance, 2.0)
            except:
                self.write_warning("Warning - in computeDistanceSigmas, no averageDistances for {0}".format(key))
                continue


            N = len(averageDistances[key])
            #if only 1 set or sigma less than 1 mm, default to sigma of all measurements
            if N <= 1:
                sigmas[key] = distance_sigma
            else:
                sum = sum/float(N)
                sigmas[key] = math.sqrt(sum)
                if sigmas[key] < 0.0001:
                    sigmas[key] = distance_sigma
        return sigmas

    def fixInvalidDMSValue(self,degree,minute,second):
        if second >= 60:
            second -= 60
            minute += 1
        if minute >= 60 :
            minute -= 60
            if degree < 0:
                degree -= 1
            else:
                degree += 1
        if degree <= -360:
            degree += 360
        elif degree >= 360:
            degree -= 360

        return (degree,minute,second)

    def addCurrentMeasurement(self,fromStation,fromHeightLabel,currentLineNumber,NumSets,NumPoints,
                              DGRPLabel,angle_sigma,HDIR_uncer_label,distance_sigma,ZANG_uncer_label):
        toStation = ""
        toHeight = 0.0
        toHeightLabel = ""
        measurementType = None
        distance = 0
        degree = 0
        minute = 0
        second = 0.0
        meanAngles = collections.OrderedDict()
        meanDistances = collections.OrderedDict()
        distanceTypes = collections.OrderedDict()
        reducedAngles = collections.OrderedDict()
        meanOfFaceDistances = collections.OrderedDict()
        sigmas = collections.OrderedDict()
        heights = collections.OrderedDict()
        pointCtr = 0

        line = self.input_lines[currentLineNumber]
        while currentLineNumber < self.num_lines:
            try:
                if line[0:8] == "Point ID":
                    toStation = self.parseToStation(line)
                    if not toStation:
                        self.write_warning('Warning - unable to parse station from line {0} of {1}. line: >>> {2}'.format(
                            str(currentLineNumber+1),os.path.basename(self.infile_name),self.input_lines[currentLineNumber]))
                elif "Reflector Height" in line:
                    toHeight = self.parseToHeight(line)
                    if toHeight != 0 and toStation:
                        toHeightLabel = self.getNextHeightLabel(toStation,toHeight)
                        self.addHGHTRecord(toHeightLabel,toHeight)
                    else:
                        toHeightLabel = ""
                    heights[toStation] = toHeightLabel
                elif ("EDM Type" in line) and (measurementType == "DIST") and fromStation and toStation:
                    distance_type = self.parseDistanceType(line)
                    distanceTypes[toStation] = distance_type
                elif "Mean of all Sets (V)" in line:
                    meanAngles[toStation] = self.parseAngle(line)
                    measurementType = "ZANG"                
                elif "Mean of all Sets (Hz)" in line:
                    meanAngles[toStation] = self.parseAngle(line)                
                    measurementType = "HDIR"                
                elif "Mean Distance Of All Sets" in line:
                    meanDistances[toStation] = self.parseDistance(line)
                    measurementType = "DIST"
                elif line[0:3] == "Set":
                    toStation = self.parseToStationFromSingleSet(line)
                    (currentLineNumber,line) = self.getNextLine(currentLineNumber)
                    if measurementType == "ZANG":
                        if toStation not in reducedAngles:
                            reducedAngles[toStation] = []
                        reducedAngles[toStation].append(self.parseMeanOfFaceAngle(line))
                        pointCtr += 1
                    elif measurementType == "HDIR":
                        if toStation not in reducedAngles:
                            reducedAngles[toStation] = []
                        reducedAngles[toStation].append(self.parseReducedMean(line))
                        pointCtr += 1
                    elif measurementType == "DIST":
                        distance = self.parseMeanOfFaceDistance(line)
                        if(distance < 0.0): #See this in the .log files sometimes
                            self.write_warning("Warning - ignored distance less than zero in {} at line number {}.  Line: >>> {}".format(
                                os.path.basename(self.infile_name), currentLineNumber+1, line))
                            if toStation not in meanOfFaceDistances:
#                                (currentLineNumber,line) = self.getNextLine(currentLineNumber)
#                                continue
                                meanOfFaceDistances[toStation] = []
                                meanOfFaceDistances[toStation].append(meanDistances[toStation])
                                pointCtr += 1
                            else: #if valid station and invalid distance, reduce sigma by making this difference = 0
                                meanOfFaceDistances[toStation].append(meanDistances[toStation])
                                pointCtr += 1
                        else:
                            if toStation not in meanOfFaceDistances:
                                meanOfFaceDistances[toStation] = []
                            meanOfFaceDistances[toStation].append(distance)
                            pointCtr += 1
                elif len(line) < 2 and pointCtr == NumSets * NumPoints:
                    if measurementType == "DIST":
                        sigmas = self.computeDistanceSigmas(meanDistances,meanOfFaceDistances,distance_sigma)
                        for key,value in sigmas.items():
                            if distanceTypes[key] == "DIST_IR":
                                if not self.have_DIST_IR_UNCR:
                                    self.have_DIST_IR_UNCR = True
                                    DIST_uncer_label = self.addUNCRRecord(self.InstrumentType,"DIST_IR")
                                else:
                                    DIST_uncer_label = self.TOTSTA_DIST_IR.label
                                if self.TOTSTA_DIST_IR.use_inst:
                                    self.addDISTRecord(fromStation,key,fromHeightLabel,heights[key],meanDistances[key],value,DIST_uncer_label)
                                else:
                                    self.addDISTRecord(fromStation,key,fromHeightLabel,heights[key],meanDistances[key],self.TOTSTA_DIST_IR.sigma,DIST_uncer_label)
                            elif distanceTypes[key] == "DIST_RED":
                                if not self.have_DIST_RED_UNCR:
                                    self.have_DIST_RED_UNCR = True
                                    DIST_uncer_label = self.addUNCRRecord(self.InstrumentType,"DIST_RED")
                                else:
                                    DIST_uncer_label = self.TOTSTA_DIST_RED.label
                                if self.TOTSTA_DIST_RED.use_inst:
                                    self.addDISTRecord(fromStation,key,fromHeightLabel,heights[key],meanDistances[key],value,DIST_uncer_label)
                                else:
                                    self.addDISTRecord(fromStation,key,fromHeightLabel,heights[key],meanDistances[key],self.TOTSTA_DIST_RED.sigma,DIST_uncer_label)
                            elif distanceTypes[key] == "DIST_NON":
                                if not self.have_DIST_NON_UNCR:
                                    self.have_DIST_NON_UNCR = True
                                    DIST_uncer_label = self.addUNCRRecord(self.InstrumentType,"DIST_NON")
                                else:
                                    DIST_uncer_label = self.TOTSTA_DIST_NON.label
                                if self.TOTSTA_DIST_NON.use_inst:
                                    self.addDISTRecord(fromStation,key,fromHeightLabel,heights[key],meanDistances[key],value,DIST_uncer_label)
                                else:
                                    self.addDISTRecord(fromStation,key,fromHeightLabel,heights[key],meanDistances[key],self.TOTSTA_DIST_NON.sigma,DIST_uncer_label)
                        break
                    else:
                        sigmas = self.computeSigmas(meanAngles,reducedAngles,angle_sigma)
                        for key,value in sigmas.items():
                            (degree,minute,second) = self.fixInvalidDMSValue(meanAngles[key][0],meanAngles[key][1],meanAngles[key][2])
                            if measurementType == "ZANG" and fromStation:
                                if self.TOTSTA_VERT.use_inst:
                                    self.addZANGRecord(fromStation,key,fromHeightLabel,heights[key],degree,minute,second,value,ZANG_uncer_label)
                                else:
                                    self.addZANGRecord(fromStation,key,fromHeightLabel,heights[key],degree,minute,second,self.TOTSTA_VERT.sigma,ZANG_uncer_label)
                            elif measurementType == "HDIR" and toStation:
                                if self.TOTSTA_HORZ_DIR.use_inst:
                                    self.addHDIRRecord(key,heights[key],degree,minute,second,value,DGRPLabel)
                                else:
                                    self.addHDIRRecord(key,heights[key],degree,minute,second,self.TOTSTA_HORZ_DIR.sigma,DGRPLabel)
                        break

            except:
                self.write_warning('Warning - unable to parse line {0} of {1}. line: >>> {2}'.format(str(currentLineNumber+1),
                                   os.path.basename(self.infile_name),self.input_lines[currentLineNumber]))

            (currentLineNumber,line) = self.getNextLine(currentLineNumber)

        return currentLineNumber


    def addCurrentMeasurement2XX(self,fromStation,fromHeightLabel,currentLineNumber,DGRPLabel,
                              angle_sigma,HDIR_uncer_label,ZANG_uncer_label,meas_type,version):
        toStation = ""
        toHeight = 0.0
        toHeightLabel = ""
        distance = 0
        degree = 0
        minute = 0
        second = 0.0
        meanAngles = collections.OrderedDict()
        averageAngles = collections.OrderedDict()
        sigmas = collections.OrderedDict()
        heights = collections.OrderedDict()
        parsingSingles = False

        line = self.input_lines[currentLineNumber]
        while currentLineNumber < self.num_lines:
            try:
                if( (line[5:13] == "Point Id" and version == "2.04") or
                    (line[5:13] == "Point No" and (version == "2.22" or version == "2.24")) ):
                    toStation = self.parseToStation(line)
                    if not toStation:
                        self.write_warning('Warning - unable to parse station from line {0} of {1}. line: >>> {2}'.format(
                            str(currentLineNumber+1),os.path.basename(self.infile_name),self.input_lines[currentLineNumber]))
                elif( ("Refl. Ht." in line and version == "2.04") or
                      ("Refl.Height" in line and (version == "2.22" or version == "2.24")) ):
                    toHeight = self.parseToHeight204(line)
                    if toHeight != 0 and toStation:
                        toHeightLabel = self.getNextHeightLabel(toStation,toHeight)
                        self.addHGHTRecord(toHeightLabel,toHeight)
                    else:
                        toHeightLabel = ""
                    heights[toStation] = toHeightLabel
                elif "Mean direction" in line:
#                    (degree,minute,second) = self.parseAngle2XX(line,version)
                    meanAngles[toStation] = self.parseAngle2XX(line,version)
                elif "Results of single sets" in line:
                    parsingSingles = True
                elif "Average" in line:
                    if toStation not in averageAngles:
                        averageAngles[toStation] = []
                    averageAngles[toStation].append(self.parseAngle2XX(line,version))
                elif len(line) < 2 and parsingSingles and len(averageAngles) > 0:
                    sigmas = self.computeSigmas(meanAngles,averageAngles,angle_sigma)
                    for key,value in sigmas.items():
                        (degree,minute,second) = self.fixInvalidDMSValue(meanAngles[key][0],meanAngles[key][1],meanAngles[key][2])
                        if meas_type == "ZANG" and fromStation:
                            if self.TOTSTA_VERT.use_inst:
                                self.addZANGRecord(fromStation,key,fromHeightLabel,heights[key],degree,minute,second,
                                          value,ZANG_uncer_label)
                            else:
                                self.addZANGRecord(fromStation,key,fromHeightLabel,heights[key],degree,minute,second,
                                          self.TOTSTA_VERT.sigma,ZANG_uncer_label)
                        elif meas_type == "HDIR":
                            if self.TOTSTA_HORZ_DIR.use_inst:
                                self.addHDIRRecord(key,heights[key],degree,minute,second,value,DGRPLabel)
                            else:
                                self.addHDIRRecord(key,heights[key],degree,minute,second,self.TOTSTA_HORZ_DIR.sigma,DGRPLabel)
                    break
 
            except:
                self.write_warning('Warning - unable to parse line {0} of {1}. line: >>> {2}'.format(
                    str(currentLineNumber+1),os.path.basename(self.infile_name),self.input_lines[currentLineNumber]))

            (currentLineNumber,line) = self.getNextLine(currentLineNumber)

        return currentLineNumber


    def parseVersion450(self):
        fromStation = None
        fromHeightLabel = None
        fromHeight = 0
        DIST_uncer_label = None
        ZANG_uncer_label = None
        HDIR_uncer_label = None
        numPoints = 0
        haveUNCR_HDIR = False
        haveUNCR_ZANG = False
        angle_sigma = 0.0
        distance_sigma = 0.0
        DGRPLabel = None
        fromStations = []
        includes = []
#        reading_header = False
#        duplicate_header = False
        NumSets = 0
        NumPoints = 0

        (currentLineNumber,line) = self.getNextLine(-1)
        while currentLineNumber < self.num_lines:
            try:
                if ("Sets of Angles" in line) or ("Instrument Type" in line) or ("Instrument Serial No." in line):#retain metadata
                    if "Instrument Type" in line:
                        self.InstrumentType = self.parseInstrumentType(line)
                    self.addComment(line)
                    if "Sets of Angles Start" in line:
                        self.addComment("")
#                        if reading_header:
#                            duplicate_header = True
#                        reading_header = True
                elif "TPS Station" in line:
                    fromStation = self.parseFromStation(line)
                elif "Instrument Height" in line:
                    fromHeight = self.parseFromHeight(line)
                elif "Horizontal Set Results" in line:
#                    reading_header = False
#                    duplicate_header = False
                    if not fromStation:
                        self.write_warning('Warning - unable to parse station from line {0} of {1}. line: >>> {2}'.format(
                            str(currentLineNumber+1),os.path.basename(self.infile_name),self.input_lines[currentLineNumber]))
                    else:
                        outfile_basename = os.path.basename(self.outfile_name)
                        include_file_name = outfile_basename[:outfile_basename.rfind('.')] + '_' + fromStation + '.lsa'
                        if include_file_name in includes:
                            include_file_name = self.iterateIncludeFileName(include_file_name,includes)
                        includes.append(include_file_name)
                        self.addInclude(include_file_name)
                        if fromStation not in fromStations:
                            fromStations.append(fromStation)
                    if fromHeight != 0 and fromStation:
                        fromHeightLabel = self.getNextHeightLabel(fromStation,fromHeight)
                        self.addHGHTRecord(fromHeightLabel,fromHeight)
                    else:
                        fromHeightLabel = ""

                    if not haveUNCR_HDIR:#insert UNCR record the first time this measurement type is encountered
                        HDIR_uncer_label = self.addUNCRRecord(self.InstrumentType,"HDIR")
                        haveUNCR_HDIR = True
                    DGRPLabel = self.getNextDGRPLabel()
                    if fromStation == None:
                        print("Error - AT station not found.  File {0} needs header.  Exiting.".format(self.infile_name), file=sys.stderr)
                        exit(1)
                    else:
                        self.addDGRPRecord(DGRPLabel,fromStation,HDIR_uncer_label)# from height information is lost
                elif "Vertical Set Results" in line:
                    self.addComment("")
                    if not haveUNCR_ZANG:#insert UNCR record the first time this measurement type is encountered
                        ZANG_uncer_label = self.addUNCRRecord(self.InstrumentType,"ZANG")
                        haveUNCR_ZANG = True
                elif ("Number Of Sets" in line) or ("Number of Sets" in line):
                    (NumSets,NumPoints) = self.parseNumSetsNumPoints(line)
                elif "Standard Deviation Of Single Measurement" in line:
                    angle_sigma = self.parseAngularSigma(line)
                elif "Standrad Deviation Of Single Distance" in line: #The typo matches that in the .log file!
                    self.addComment("")
                    distance_sigma = self.parseDistanceSigma(line)
                elif line[0:9] == "Point ID:":
                    if fromStation == None:
                        print("Error - AT station not found.  File {0} needs header.  Exiting.".format(self.infile_name), file=sys.stderr)
                        exit(1)
                    else:
                        currentLineNumber = self.addCurrentMeasurement(fromStation,fromHeightLabel,currentLineNumber,NumSets,NumPoints,
                                                                       DGRPLabel,angle_sigma,HDIR_uncer_label,distance_sigma,
                                                                       ZANG_uncer_label)
                        NumSets = 0
                        NumPoints = 0

            except:
                self.write_warning('Warning - unable to parse line {0} of {1}. line: >>> {2}'.format(
                    str(currentLineNumber+1),os.path.basename(self.infile_name),self.input_lines[currentLineNumber]))

            (currentLineNumber,line) = self.getNextLine(currentLineNumber)

    def parseVersion2XX(self,version):
        fromStation = None
        fromHeightLabel = None
        fromHeight = 0
        ZANG_uncer_label = None
        HDIR_uncer_label = None
        haveUNCR_DIST = False
        haveUNCR_HDIR = False
        haveUNCR_ZANG = False
        angle_sigma = 0.0
        DGRPLabel = None
        parse_means = False
        meas_type = None
        fromStations = []
        includes = []
#        reading_header = False
#        duplicate_header = False

        (currentLineNumber,line) = self.getNextLine(-1)
        while currentLineNumber < self.num_lines:
            try:
                if ("Sets of Angles" in line) or ("Instrument" in line) or ("Program Start" in line) or ("User Templ." in line):#retain metadata
                    if "Instrument" in line:
                        self.InstrumentType = self.parseInstrumentType204(line)
                    self.addComment(line)
                    if "Program Start" in line:
                        self.addComment("")
#                        if reading_header:
#                            duplicate_header = True
#                        reading_header = True
                elif( ("Station Id" in line and version == "2.04") or
                      (line[0:7] == "Station" and (version == "2.22" or version == "2.24")) ):
                    fromStation = self.parseFromStation(line)
                elif "hi=" in line:
                    fromHeight = self.parseFromHeight204(line)
                elif "Horizontal set results" in line:
                    if not fromStation:
                        self.write_warning('Warning - unable to parse station from line {0} of {1}. line: >>> {2}'.format(
                            str(currentLineNumber+1),os.path.basename(self.infile_name),self.input_lines[currentLineNumber]))
                    else:
                        outfile_basename = os.path.basename(self.outfile_name)
                        include_file_name = outfile_basename[:outfile_basename.rfind('.')] + '_' + fromStation + '.lsa'
                        if include_file_name in includes:
                            include_file_name = self.iterateIncludeFileName(include_file_name,includes)
                        includes.append(include_file_name)
                        self.addInclude(include_file_name)
                        if fromStation not in fromStations:
                            fromStations.append(fromStation)
#                    reading_header = False
#                    duplicate_header = False
                    if fromHeight != 0 and fromStation:
                        fromHeightLabel = self.getNextHeightLabel(fromStation,fromHeight)
                        self.addHGHTRecord(fromHeightLabel,fromHeight)
                    else:
                        fromHeightLabel = ""

                    if not haveUNCR_HDIR:#insert UNCR record the first time this measurement type is encountered
                        HDIR_uncer_label = self.addUNCRRecord(self.InstrumentType,"HDIR")
                        haveUNCR_HDIR = True
                    DGRPLabel = self.getNextDGRPLabel()
                    if fromStation == None:
                        print("Error - AT station not found.  File {0} needs header.  Exiting.".format(self.infile_name), file=sys.stderr)
                        exit(1)
                    else:
                        self.addDGRPRecord(DGRPLabel,fromStation,HDIR_uncer_label)# from height information is lost
                    meas_type = "HDIR"
                elif "Vertical set results" in line:
                    self.addComment("")
                    if not haveUNCR_ZANG:#insert UNCR record the first time this measurement type is encountered
                        ZANG_uncer_label = self.addUNCRRecord(self.InstrumentType,"ZANG")
                        haveUNCR_ZANG = True
                    meas_type = "ZANG"
                elif "Results of single sets" in line:
                    parse_means = False
                elif "Standard deviation of any measurement" in line:
                    angle_sigma = self.parseAngularSigma2XX(line,version)
                    parse_means = True
            except:
                self.write_warning('Warning - unable to parse line {0} of {1}. line: >>> {2}'.format(
                    str(currentLineNumber+1),os.path.basename(self.infile_name),self.input_lines[currentLineNumber]))

            if( ( ((line[5:13] == "Point Id") and (version == "2.04")) or
                  ((line[5:13] == "Point No") and (version == "2.22" or version == "2.24")) ) and parse_means):
                if fromStation == None:
                    print("Error - AT station not found.  File {0} needs header.  Exiting.".format(self.infile_name), file=sys.stderr)
                    exit(1)
                else:
                    currentLineNumber = self.addCurrentMeasurement2XX(fromStation,fromHeightLabel,currentLineNumber,
                                                                      DGRPLabel,angle_sigma,HDIR_uncer_label,
                                                                      ZANG_uncer_label,meas_type,version)
                
            (currentLineNumber,line) = self.getNextLine(currentLineNumber)

    def convert(self):
        try:
            self.readInputFile()
        except:
            print('Error - unable to read {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        try:
            if not self.isValid():
                print("Error - Sets of Angles header not found.  {0} is not a valid Leica Sets of Angles log file.  Exiting.\n".format(self.infile_name), file=sys.stderr)
                exit(1)
        except:
            print('Error - unable to determine validity of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        try:
            version = self.getVersion()
        except:
            print('Error - unable to determine version of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        self.converter_string = "Leica Total Station v{0}".format(version)
        if version not in self.SUPPORTED_VERSIONS:
            print("Error - {0} is not a supported version.\n"
                "The following versions are supported: {1}.\n"
                "Exiting.\n".format(version,str(self.SUPPORTED_VERSIONS)), file=sys.stderr)
            exit(1)
        elif( (version == "8.03") or
              (version == "8.02") or
              (version == "5.70") or
              (version == "5.61") or
              (version == "5.00") or
              (version == "5.05") or
              (version == "4.50") ):
            self.parseVersion450()
        elif( (version == "2.04") or
              (version == "2.22") or
              (version == "2.24") ):
            self.parseVersion2XX(version)
        
        self.writeOutputFile()
        if self.warnings_generated:
            print("Warning - warnings generated when parsing {}. Refer to {} for details.".format(
                os.path.basename(self.infile_name), self.warnfile_name))



