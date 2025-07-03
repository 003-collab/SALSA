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

#standard library imports
import sys
from abc import ABCMeta, abstractmethod
#SALSA imports
from wrappers.dist import LSADist
from wrappers.zang import LSAZAngle
from wrappers.vang import LSAVAngle
from wrappers.comment import Comment
from wrappers.hdir import LSAHDir
from wrappers.hght import LSAHeight
from wrappers.dgrp import LSADirGroup
from wrappers.uncr import LSAUncr
from wrappers.include import Include
from wrappers.posg import LSAPosG
from wrappers.posc import LSAPosC
from wrappers.dxyz import LSADelta
from wrappers.hdif import LSAHDif
import lsaversion

class Config(object):
    def __init__(self,label,sigma,ppm,at_c,from_c,to_c):
        self.label = label
        if sigma == "INST":
            self.sigma = None
            self.use_inst = True
        else:
            self.sigma = float(sigma) #meters or soa
            self.use_inst = False
        self.ppm = float(ppm)
        self.at_c = float(at_c) #meters
        self.from_c = float(from_c) #meters
        self.to_c = float(to_c) #meters

class LSAConverter(object):
    __metaclass__ = ABCMeta

    MEASUREMENT_TYPES = ["ZANG", "VANG", "DIST", "HDIR", "POSG", "DXYZ", "HDIF"]#TODO - extend to other measurement types and add checks for being supported


    def __init__(self,infile,outfile,configfile,warnfile):
        """Base class for each instrument output type

        Required Arguments:
        infile     (str)   -- name of the instrument output file to convert
        outfile    (str)   -- name of the LSA output file to create
        configfile (str)   -- name of the configuration file that specifies centering errors and measurement sigmas
        warnfile   (str)   -- name of the .wrn file to which warning are written, should they occur during conversion
        """

        self.output_records = []            #list of python wrapper classes for LSA records
        self.input_lines = []               #list of lines read from the input file
        self.infile_name = infile
        self.outfile_name = outfile
        self.configfile_name = configfile
        self.warnfile_name = warnfile
        self.warnings_generated = False
        self.converter_string = ""          #string written out to new LSA files with SALSA version, specifying converter type
        self.num_lines = 0                  #number of lines in the input file
        self.DGRP_num = 1                   #counter for auto-generated DGRP labels
        self.height_dict = {}               #dict of height labels and heights
        self.SALSA_VERSION_STRING = lsaversion.LSAVERSION_MAJOR_MINOR_PATCH + " -- Backward compatible to " + lsaversion.LSASUPPORTEDTOVERSION_MAJOR_MINOR_PATCH
        self.GPS_STATIC = None
        self.GPS_KINEMATIC = None
        self.DIFF_LEVELS = None
        self.TOTSTA_DIST_IR = None
        self.TOTSTA_DIST_RED = None
        self.TOTSTA_DIST_NON = None
        self.TOTSTA_HORZ_DIR = None
        self.TOTSTA_HORZ_ANG = None
        self.TOTSTA_VERT = None
        self.TOTSTA_DE = None
        self.TOTSTA_INSTR_HT = None


    @abstractmethod
    def isValid(self):
        """Determine if the file is a valid one of the expected type"""
        pass

    @abstractmethod
    def getVersion(self,input_lines=None):
        """Scan through file for version info"""
        pass

    @abstractmethod
    def convert(self):
        """parse input_lines into output_records"""
        pass

    def write_warning(self, message):
        with open(self.warnfile_name, 'at') as f:
            f.write(message+'\n')
        if not self.warnings_generated:
            self.warnings_generated = True

    def addInclude(self,file_name):
        """adds a line as a LSAINCLUDE record to output_records"""
        lsaInclude = Include(file_name)
        self.output_records.append(lsaInclude)
    
    def readInputFile(self):
        try:
            with open(self.infile_name,'r') as f:
                self.input_lines = f.readlines()
            self.num_lines = len(self.input_lines)
        except EnvironmentError:
            print("Error - unable to find/read {0}. Exiting.\n".format(self.infile_name), file=sys.stderr)
            exit(1)

    def parse_config_file(self):
        lines = []
        legacy = False
        try:
            with open(self.configfile_name,'r') as f:
                lines = f.readlines()
            for line in lines:
                newline = line.replace('\t\t','\t').replace('\n','')
                parts = newline.split('\t')
                #handle legacy converter.cfg files
                if parts[0] == "MEAS_TYPE":
                    if parts[4] == "AT_C":
                        legacy = True
                if legacy:
                    if parts[0] == "GPS_STATIC":
                        self.GPS_STATIC = Config(parts[1],parts[2],0.0,parts[3],parts[4],parts[5])
                    elif parts[0] == "GPS_KINEMATIC":
                        self.GPS_KINEMATIC = Config(parts[1],parts[2],0.0,parts[3],parts[4],parts[5])
                    elif parts[0] == "DIFF_LEVELS":
                        self.DIFF_LEVELS = Config(parts[1],parts[2],0.0,parts[3],parts[4],parts[5])
                    elif parts[0] == "TOTSTA_DIST_IR":
                        self.TOTSTA_DIST_IR = Config(parts[1],parts[2],0.0,parts[3],parts[4],parts[5])
                    elif parts[0] == "TOTSTA_DIST_RED":
                        self.TOTSTA_DIST_RED = Config(parts[1],parts[2],0.0,parts[3],parts[4],parts[5])
                    elif parts[0] == "TOTSTA_DIST_NON":
                        self.TOTSTA_DIST_NON = Config(parts[1],parts[2],0.0,parts[3],parts[4],parts[5])
                    elif parts[0] == "TOTSTA_HORZ_DIR":
                        self.TOTSTA_HORZ_DIR = Config(parts[1],parts[2],0.0,parts[3],parts[4],parts[5])
                    elif parts[0] == "TOTSTA_HORZ_ANG":
                        self.TOTSTA_HORZ_ANG = Config(parts[1],parts[2],0.0,parts[3],parts[4],parts[5])
                    elif parts[0] == "TOTSTA_VERT":
                        self.TOTSTA_VERT = Config(parts[1],parts[2],0.0,parts[3],parts[4],parts[5])
                    elif parts[0] == "TOTSTA_DE":
                        self.TOTSTA_DE = Config(parts[1],parts[2],0.0,parts[3],parts[4],parts[5])
                    elif parts[0] == "TOTSTA_INSTR_HT":
                        self.TOTSTA_INSTR_HT = Config(parts[1],parts[2],0.0,parts[3],parts[4],parts[5])
                else:
                    if parts[0] == "GPS_STATIC":
                        self.GPS_STATIC = Config(parts[1],parts[2],parts[3],parts[4],parts[5],parts[6])
                    elif parts[0] == "GPS_KINEMATIC":
                        self.GPS_KINEMATIC = Config(parts[1],parts[2],parts[3],parts[4],parts[5],parts[6])
                    elif parts[0] == "DIFF_LEVELS":
                        self.DIFF_LEVELS = Config(parts[1],parts[2],parts[3],parts[4],parts[5],parts[6])
                    elif parts[0] == "TOTSTA_DIST_IR":
                        self.TOTSTA_DIST_IR = Config(parts[1],parts[2],parts[3],parts[4],parts[5],parts[6])
                    elif parts[0] == "TOTSTA_DIST_RED":
                        self.TOTSTA_DIST_RED = Config(parts[1],parts[2],parts[3],parts[4],parts[5],parts[6])
                    elif parts[0] == "TOTSTA_DIST_NON":
                        self.TOTSTA_DIST_NON = Config(parts[1],parts[2],parts[3],parts[4],parts[5],parts[6])
                    elif parts[0] == "TOTSTA_HORZ_DIR":
                        self.TOTSTA_HORZ_DIR = Config(parts[1],parts[2],parts[3],parts[4],parts[5],parts[6])
                    elif parts[0] == "TOTSTA_HORZ_ANG":
                        self.TOTSTA_HORZ_ANG = Config(parts[1],parts[2],parts[3],parts[4],parts[5],parts[6])
                    elif parts[0] == "TOTSTA_VERT":
                        self.TOTSTA_VERT = Config(parts[1],parts[2],parts[3],parts[4],parts[5],parts[6])
                    elif parts[0] == "TOTSTA_DE":
                        self.TOTSTA_DE = Config(parts[1],parts[2],parts[3],parts[4],parts[5],parts[6])
                    elif parts[0] == "TOTSTA_INSTR_HT":
                        self.TOTSTA_INSTR_HT = Config(parts[1],parts[2],parts[3],parts[4],parts[5],parts[6])

        except EnvironmentError:
            print("Error - unable to find/read {0}. Exiting.\n".format(self.infile_name), file=sys.stderr)
            exit(1)


    def getNextLine(self,currentLineNumber):
        if currentLineNumber >= self.num_lines-1:
            return (self.num_lines,None)
        else:
            return (currentLineNumber+1,self.input_lines[currentLineNumber+1])

    @abstractmethod
    def writeOutputFile(self):
        """write output_records into self.outfile_name (and others)"""
        pass

    def addComment(self,line):
        """adds line as a LSAComment record to output_records"""
        lsaComment = Comment(line)
        self.output_records.append(lsaComment)

    def addInclude(self,file_name):
        """adds a line as a LSAINCLUDE record to output_records"""
        lsaInclude = Include(file_name)
        self.output_records.append(lsaInclude)

    def getNextHeightLabel(self,station,height):
        lastEntryNum = 0
        for key in self.height_dict:#height label is of the form HGHT#<number>_<station_name>
            end = key.find('_')
            begin = key.find('#')
            stored_station = key[end+1:]
            if station == stored_station:#previous entry for this station
                if self.height_dict[key] == height:#duplicate value, don't create a new record
                    return key
                else:#different height value, keep looking for duplicates
                    entryNum = int(key[begin+1:end])
                    if entryNum > lastEntryNum:
                        lastEntryNum = entryNum
        #no duplicates found, add new entry to the dict
        labelPrefix = "HGHT#{0}_".format(lastEntryNum+1)
        heightLabel = labelPrefix + station
        return heightLabel    

    def getNextDGRPLabel(self):
        """adds line as a LSADGRP record to output_records"""
        DGRPlabel = "DGRP#{0}".format(self.DGRP_num)
        self.DGRP_num += 1
        return DGRPlabel

    def addHGHTRecord(self,heightLabel,height, heightUnit="m"):
        """adds LSAHGHT record to output_records"""
        if heightLabel in self.height_dict:
            return
        else:
            self.height_dict[heightLabel] = height
            if self.TOTSTA_INSTR_HT is not None:
                lsaHeight = LSAHeight(heightLabel,height,heightUnit,self.TOTSTA_INSTR_HT.sigma,"m")
            else:
                lsaHeight = LSAHeight(heightLabel,height,heightUnit,0.0,"m")
            self.output_records.append(lsaHeight)

    def addUNCRRecord(self,InstrumentType,currentMeasurementType):
        """adds LSAUNCR record to output_records"""
        #InstrumentType passed in case we have an instrument-dependence later
        #No duplication within a given file, so only one UNCR per measurement type expected

        if currentMeasurementType == "DIST_IR":
            label = self.TOTSTA_DIST_IR.label
            lsaUncer = LSAUncr(label)
            lsaUncer.set_from_center(self.TOTSTA_DIST_IR.from_c,"m")
            lsaUncer.set_to_center(self.TOTSTA_DIST_IR.to_c,"m")
            if self.TOTSTA_DIST_IR.ppm > 0:
                lsaUncer.set_PPM(self.TOTSTA_DIST_IR.ppm)
        elif currentMeasurementType == "DIST_RED":
            label = self.TOTSTA_DIST_RED.label
            lsaUncer = LSAUncr(label)
            lsaUncer.set_from_center(self.TOTSTA_DIST_RED.from_c,"m")
            lsaUncer.set_to_center(self.TOTSTA_DIST_RED.to_c,"m")
            if self.TOTSTA_DIST_RED.ppm > 0:
                lsaUncer.set_PPM(self.TOTSTA_DIST_RED.ppm)
        elif currentMeasurementType == "DIST_NON":
            label = self.TOTSTA_DIST_NON.label
            lsaUncer = LSAUncr(label)
            lsaUncer.set_from_center(self.TOTSTA_DIST_NON.from_c,"m")
            lsaUncer.set_to_center(self.TOTSTA_DIST_NON.to_c,"m")
            if self.TOTSTA_DIST_NON.ppm > 0:
                lsaUncer.set_PPM(self.TOTSTA_DIST_NON.ppm)
        elif currentMeasurementType == "ZANG":
            label = self.TOTSTA_VERT.label
            lsaUncer = LSAUncr(label)
            lsaUncer.set_from_center(self.TOTSTA_VERT.from_c,"m")
            lsaUncer.set_to_center(self.TOTSTA_VERT.to_c,"m")
        elif currentMeasurementType == "HDIR":
            label = self.TOTSTA_HORZ_DIR.label
            lsaUncer = LSAUncr(label)
            lsaUncer.set_from_center(self.TOTSTA_HORZ_DIR.from_c,"m")
            lsaUncer.set_to_center(self.TOTSTA_HORZ_DIR.to_c,"m")
        elif currentMeasurementType == "HDIF":
            label = self.DIFF_LEVELS.label
            lsaUncer = LSAUncr(label)
            lsaUncer.set_from_center(0.0,"m") #Fix to Bug #1425
            lsaUncer.set_to_center(0.0,"m") #Fix to Bug #1425
            if self.DIFF_LEVELS.ppm > 0:
                lsaUncer.set_PPM(self.DIFF_LEVELS.ppm)
        elif currentMeasurementType == "DXYZ_KINEMATIC":
            label = self.GPS_KINEMATIC.label
            lsaUncer = LSAUncr(label)
            lsaUncer.set_from_center(self.GPS_KINEMATIC.from_c,"m")
            lsaUncer.set_to_center(self.GPS_KINEMATIC.to_c,"m")
            if self.GPS_KINEMATIC.ppm > 0:
                lsaUncer.set_PPM(self.GPS_KINEMATIC.ppm)
        elif currentMeasurementType == "DXYZ_STATIC":
            label = self.GPS_STATIC.label
            lsaUncer = LSAUncr(label)
            lsaUncer.set_from_center(self.GPS_STATIC.from_c,"m")
            lsaUncer.set_to_center(self.GPS_STATIC.to_c,"m")
            if self.GPS_STATIC.ppm:
                lsaUncer.set_PPM(self.GPS_STATIC.ppm)

        self.output_records.append(lsaUncer)
        return label

    def addDISTRecord(self,from_station,to_station,from_height_label,to_height_label,distance,distance_sigma,uncer_label, distUnit="m"):
        """adds LSADIST record to output_records"""
        lsaDist = LSADist(from_station, to_station, distance, distance_sigma, distUnit)
        missingHFROM = False
        if from_height_label != "":
            lsaDist.set_from_height_label(from_height_label)
        else:
            missingHFROM = True
        if to_height_label != "":
            lsaDist.set_to_height_label(to_height_label)
        if uncer_label != "":
            lsaDist.set_uncr_label(uncer_label)
        if missingHFROM:#Fix to Bug #1361
            self.write_warning("Warning - missing HI on record: {0}".format(lsaDist.get_lsa_string()))
        self.output_records.append(lsaDist)

    def addZANGRecord(self,from_station,to_station,from_height_label,to_height_label,degree,minute,second,angle_sigma,uncer_label, refrac_coeff=None):
        """adds LSAZangle record to output_records"""
        lsaZang = LSAZAngle()
        missingHFROM = False
        lsaZang.set_required_values_dms(from_station, to_station, degree, minute, second, angle_sigma, "soa")
        if from_height_label != "":
            lsaZang.set_from_height_label(from_height_label)
        else:
            missingHFROM = True
        if to_height_label != "":
            lsaZang.set_to_height_label(to_height_label)
        lsaZang.set_uncr_label(uncer_label)
        if(refrac_coeff is not None):
            lsaZang.set_refract_coeff(refrac_coeff)
        if missingHFROM:#Fix to Bug #1361
            self.write_warning("Warning - missing HI on record: {0}".format(lsaZang.get_lsa_string()))
        self.output_records.append(lsaZang)

    def addVANGRecord(self,from_station,to_station,from_height_label,to_height_label,degree,minute,second,angle_sigma,uncer_label):
        """adds LSAZangle record to output_records"""
        lsaVang = LSAVAngle()
        lsaVang.set_required_values_dms(from_station, to_station, degree, minute, second, angle_sigma, "soa")
        if from_height_label != "" and from_height_label is not None:
            lsaVang.set_from_height_label(from_height_label)
            missingHFROM = False
        else:
            missingHFROM = True
        if to_height_label != "" and to_height_label is not None:
            lsaVang.set_to_height_label(to_height_label)
        lsaVang.set_uncr_label(uncer_label)
        if missingHFROM:
            self.write_warning("Warning - missing HI on record: {0}".format(lsaVang.get_lsa_string()))
        self.output_records.append(lsaVang)

    def addDGRPRecord(self,DGRPLabel,fromStation,uncer_label):
        """adds LSADGRP record to output_records"""
        lsaDgrp = LSADirGroup(DGRPLabel,fromStation)
        lsaDgrp.set_uncr_label(uncer_label)
        self.output_records.append(lsaDgrp)

    def addHDIRRecord(self,to_point,to_height_label,degree,minute,second,angle_sigma,dgrp_label):
        """adds LSAHDIR record to output_records"""
        lsaHDir = LSAHDir()
        lsaHDir.set_required_values_dms(dgrp_label, to_point, degree, minute, second, angle_sigma, "soa")
        if to_height_label != "":
            lsaHDir.set_to_height_label(to_height_label)
        self.output_records.append(lsaHDir)

    def addPosgRecord(self, label, latDecDeg, latDir, lonDecDeg, lonDir, height, heightUnits, fixOrFlt):       
        lsaPosG = LSAPosG(label)               
        lsaPosG.set_required_values_decdeg(latDecDeg, latDir, lonDecDeg, lonDir, height, heightUnits)
        if fixOrFlt == "CTRL":
            lsaPosG.set_state_fixed()
        else:
            lsaPosG.set_state_floating()
        self.output_records.append(lsaPosG)

    def addPosgWithCovarianceRecord(self, label, latDeg, latMin, latSec, latDir, lonDeg, lonMin, lonSec, lonDir, height, heightUnits, cnn, cne, cnu, cee, ceu, cuu):       
        lsaPosG = LSAPosG(label)
        lsaPosG.set_required_values_dms(latDeg, latMin, latSec, latDir, lonDeg, lonMin, lonSec, lonDir, height, heightUnits)
        lsaPosG.set_state_constrained()
        lsaPosG.set_covariance(cnn, cne, cnu, cee, ceu, cuu)
        self.output_records.append(lsaPosG)

    def addPoscRecord(self, label, x, y, z, units, fixOrFlt):       
        lsaPosC = LSAPosC(label, x, y, z, units)               
        if fixOrFlt == "FIX":
            lsaPosC.set_state_fixed()
        else:
            lsaPosC.set_state_floating()
        self.output_records.append(lsaPosC)

    def addPoscWithCovarianceRecord(self, label, x, y, z, units, cxx, cxy, cxz, cyy, cyz, czz):       
        lsaPosC = LSAPosC(label, x, y, z, 'm')
        lsaPosC.set_covariance(cxx, cxy, cxz, cyy, cyz, czz)
        lsaPosC.set_state_constrained()
        self.output_records.append(lsaPosC)

    def addDeltaRecord(self, from_point, to_point, dx, dy, dz, units, cxx, cxy, cxz, cyy, cyz, czz, statOrKin, start_d = "", end_d = "", start_t = "", end_t = ""):
        lsaDelta = LSADelta(from_point, to_point, dx, dy, dz, units, cxx, cxy, cxz, cyy, cyz, czz)
        lsaDelta.set_uncr_label(statOrKin)
        lsaDelta.set_date_time(start_d, end_d, start_t, end_t)
        self.output_records.append(lsaDelta)
        
    def addHDIFRecord(self, from_station, to_station, height_dif, sigma, unit, uncr_label, apply_correction):
        lsaHDif = LSAHDif(from_station, to_station, height_dif, sigma, unit)
        lsaHDif.set_curv_corr(apply_correction)
        lsaHDif.set_uncr_label(uncr_label)
        self.output_records.append(lsaHDif)
