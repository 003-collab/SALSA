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
import sys, os, math
#LSA imports
from lsaconverter import LSAConverter
from wrappers.hdif import LSAHDif

class TrimbleConverter(LSAConverter):
    outFileType = ""
    heightUnits = "Unknown"
    coordUnits = "Unknown"
    firstTimeStatic = True
    firstTimeKinematic = True
    firstTimeLevel = True
    uncrStatLabel = ""
    uncrKinLabel = ""
    uncrLvlLabel = ""
    warnings = []
    rnd_exists = False
    rounds_filenames = []
    setup_ids=[]
    last_output_record_num=0
    # Converts units of mm (sigma convention) to units appropriate for measurement
    distSigMap = {'m' : 0.001, 'km' : 0.000001, 'ft_survey' : 3937.0/1200000.0, 'ft' : 1.0/304.8}
    # Converts units of m (covariance convention) to units appropriate for delta records
    mapMetersToUnits = {'m' : 1.0, 'km' : 0.001, 'ft_survey' : 3937.0/1200.0, 'ft' : 1.0/0.3048}
    distSigTDEF_lsaMap = {'kilometers' : 'km', 'meters' : 'm', 'feet' : 'ft',
                          'us survey feet': 'ft_survey', 'international feet': 'ft'}

    @property
    def validatedHeight(self):
        if self.heightUnits != 'ft_survey':
            return self.heightUnits
        else:
            return 'ft'

    @property
    def validatedCoord(self):
        if self.coordUnits != 'ft_survey':
            return self.coordUnits
        else:
            return 'ft'

    @property
    def validatedDist(self):
        if self.distUnits != 'ft_survey':
            return self.distUnits
        else:
            return 'ft'

    def OutputFileType(self, outputfile):
        if outputfile.endswith(".gps.lsa"):
            self.outFileType = "GPS"
        elif outputfile.endswith(".pos.lsa"):
            self.outFileType = "POS"
        elif outputfile.endswith(".lvl.lsa"):
            self.outFileType = "LVL"
        elif outputfile.endswith(".rnd.lsa"):
            self.outFileType = "RND"
        elif outputfile.endswith(".lsa"):
            self.outFileType = "LSA"
        else:
            self.outFileType = "Invalid"

    def validOutputFile(self):
        if self.outFileType == "Invalid":
            return False
        return True

    def decdeg_to_DMS(self, decdeg):
        if decdeg < 0.0:
            decdeg += 360.0
        D = int(decdeg)
        deg = 60. * abs(decdeg - float(D)) # added abs in case D > deg(e.g. 111.9999999 - 112)
        M = int(deg)
        sec = 60. * (deg - float(M))
        #check for seconds value rounding up to 60.0 when converting to LSA String
        INSTRUMENT_PRECISION_SOA = 1
        if int(math.floor(sec * pow(10, INSTRUMENT_PRECISION_SOA) + 0.5) / pow(10, INSTRUMENT_PRECISION_SOA)) >= 60:
            sec -= 60.0
            if sec < 0.0:
                sec = 0.0
            M += 1
            if M >= 60:
                M -= 60
            D += 1
            if D >= 360:
                D -= 360
        return D, M, sec

    def parseGeneral(self, isLSA):
        #search entire file untill you find the "[General]" section
        lineItr = iter(self.input_lines)        
        for line in lineItr:            
            if "[General]" in line:
                #gather needed info from General section
                currentLine = next(lineItr)
                while currentLine != "\n":
                    lineItems = currentLine.split("=")
                    if len(lineItems) <= 1:
                        currentLine = next(lineItr)
                    else:
                        unitKey = lineItems[1].rstrip().lower()
                        unitVal = self.distSigTDEF_lsaMap.get(unitKey)
                        if lineItems[0] == "ElevationHeightUnits":
                            if unitVal is None:
                                raise RuntimeError("Parser unable to set valid height unit")
                            if unitKey == "US survey feet" and isLSA:
                                print("Warning - height unit is US survey feet. Salsa treats feet measurements as International feet.", file=sys.stderr)
                            self.heightUnits = unitVal
                        elif lineItems[0] == "CoordinateUnits":
                            if unitVal is None:
                                raise RuntimeError("Parser unable to set valid coordinate unit")
                            if unitKey == "US survey feet" and isLSA:
                                print("Warning - coordinate unit is US survey feet. Salsa treats feet measurements as International feet.", file=sys.stderr)
                            self.coordUnits = unitVal
                        elif lineItems[0] == "DistanceUnits":
                            if unitVal is None:
                                raise RuntimeError("Parser unable to set valid distance unit.")
                            if unitKey == "US survey feet" and isLSA:
                                print("Warning - distance unit is US survey feet. Salsa treats feet measurements as International feet.", file=sys.stderr)
                            self.distUnits = unitVal

                        currentLine = next(lineItr)
                break
               
    def parseDXYZ(self):
        #search entire file untill you find the GPS list        
        lineItr = iter(self.input_lines)
        gps_exists = False
        for line in lineItr:            
            if "[GPS]" in line:
                #create a DXYZ record from each station in list
                covarianceUnitsConversion = self.mapMetersToUnits.get(self.coordUnits)
                if covarianceUnitsConversion is None:
                    raise Exception('Unable to find conversion for units of {0}'.format(self.coordUnits))
                currentLine = next(lineItr)
                while currentLine != "\n":
                    gps_exists = True
                    lineItems = currentLine.split(":")
                    fromPoint = lineItems[2]
                    toPoint = lineItems[3]
                    #Fix to Bug #1313
                    if( (lineItems[4] == '?') or (lineItems[5] == '?') or (lineItems[6] == '?') or
                        (lineItems[7] == '?') or (lineItems[8] == '?') or (lineItems[9] == '?') or
                        (lineItems[10] == '?') or (lineItems[11] == '?') or (lineItems[12] == '?')):
                        message = "Warning - invalid covariance at line {0} of {1}.  GPS Vector omitted.".format(self.input_lines.index(currentLine)+1,os.path.basename(self.infile_name))
                        if message not in self.warnings:
                            self.warnings.append(message)
                        validDeltaAndCovariance = False
                    else:
                        dx = float(lineItems[4])
                        dy = float(lineItems[5])
                        dz = float(lineItems[6])
                        cxx = float(lineItems[7]) * pow(covarianceUnitsConversion, 2.0)
                        cxy = float(lineItems[8]) * pow(covarianceUnitsConversion, 2.0)
                        cxz = float(lineItems[9]) * pow(covarianceUnitsConversion, 2.0)
                        cyy = float(lineItems[10]) * pow(covarianceUnitsConversion, 2.0)
                        cyz = float(lineItems[11]) * pow(covarianceUnitsConversion, 2.0)
                        czz = float(lineItems[12]) * pow(covarianceUnitsConversion, 2.0)
                        startDate = lineItems[19]
                        startTime = lineItems[20]
                        endDate = lineItems[21]
                        endTime = lineItems[22]
                        validDeltaAndCovariance = True
                    statOrKin = lineItems[-4]
                    if statOrKin == "Static or fast static" or statOrKin == "RTK":
                        if self.firstTimeStatic:
                           #add UNCR_STATIC Record
                            self.firstTimeStatic = False
                            self.uncrStatLabel = self.addUNCRRecord("Trimble", "DXYZ_STATIC")
                        if validDeltaAndCovariance:
                            self.addDeltaRecord(fromPoint, toPoint, dx, dy, dz, self.validatedCoord, cxx, cxy, cxz, cyy, cyz, czz, self.uncrStatLabel, startDate, endDate, startTime, endTime)
                    elif statOrKin == "PPKinematic":
                        if self.firstTimeKinematic:
                            #add UNCR_KINEMATIC Record
                            self.firstTimeKinematic = False
                            self.uncrKinLabel = self.addUNCRRecord("Trimble", "DXYZ_KINEMATIC")
                        if validDeltaAndCovariance:
                            self.addDeltaRecord(fromPoint, toPoint, dx, dy, dz, self.validatedCoord, cxx, cxy, cxz, cyy, cyz, czz, self.uncrKinLabel, startDate, endDate, startTime, endTime)
                    else:
                        raise RuntimeError("Parser Unable to determine if UNCR is static or kinematic")                   
                    currentLine = next(lineItr)
                break
        return gps_exists

    def parseHDIF(self):
        #search entire file untill you find the level data
        lineItr = iter(self.input_lines)
        lvl_exists = False
        firstTimeHDIF = True
        for line in lineItr:
            if "[Level Run]" in line:
                #create a HDIF record from each station pair in list
                currentLine = next(lineItr)
                if self.DIFF_LEVELS.use_inst:
                    sigmaUnitsConversion = self.distSigMap.get(self.heightUnits)
                else:
                    sigmaUnitsConversion = self.mapMetersToUnits.get(self.heightUnits)

                if sigmaUnitsConversion is None:
                    raise Exception('Unable to find conversion for units of {0}'.format(self.heightUnits))
                while currentLine != "\n":
                    lvl_exists = True
                    lineItems = currentLine.split(":")
                    fromLabel = lineItems[2]
                    toLabel = lineItems[3]
                    height_dif = float(lineItems[4])
                    hdif_sigma = float(lineItems[5]) * sigmaUnitsConversion #convert from mm
                    status = lineItems[10]
                    if self.firstTimeLevel:
                        # add UNCR_HDIF Record
                        self.firstTimeLevel = False
                        self.uncrLvlLabel = self.addUNCRRecord("Trimble", "HDIF")
                    if 'E' in status or 'C' in status:
                        if self.DIFF_LEVELS.use_inst:
                            self.addHDIFRecord(fromLabel, toLabel, height_dif, hdif_sigma, self.validatedHeight, self.uncrLvlLabel, False)
                        else:
                            self.addHDIFRecord(fromLabel, toLabel, height_dif, self.DIFF_LEVELS.sigma * sigmaUnitsConversion, self.validatedHeight, self.uncrLvlLabel, False)
                    elif 'D' in status:
                        if self.DIFF_LEVELS.use_inst:
                            hdif_record = LSAHDif(fromLabel, toLabel, height_dif, hdif_sigma, self.validatedHeight)
                        else:
                            hdif_record = LSAHDif(fromLabel, toLabel, height_dif, self.DIFF_LEVELS.sigma * sigmaUnitsConversion, self.validatedHeight)
                        hdif_record.set_uncr_label(self.uncrLvlLabel)
                        self.addComment(' ' + hdif_record.get_lsa_string())

                    currentLine = next(lineItr)
                break
        return lvl_exists

    def getRoundsOutputFilenames(self, terrestrial_lines):
        noPathInRND = self.outfile_name.split("/")
        output_filenames = []
        station_names=[]
        for id in self.setup_ids:
            for line in terrestrial_lines:
                lineItems = line.split(":")
                fromLabel = str(lineItems[2])
                setup_id = str(lineItems[16])
                if setup_id == id:
                    if fromLabel not in station_names:
                        station_names.append(fromLabel)
                    else:
                        ctr = 2
                        while '{}.{}'.format(fromLabel, ctr) in station_names:
                            ctr += 1
                        station_names.append('{}.{}'.format(fromLabel, ctr))
                    break
                else:
                    continue
        for name in station_names:
            if self.outfile_name.endswith(".rnd.lsa"):
                fname = noPathInRND[-1].replace(".rnd.lsa", "_{}.rnd.lsa".format(name))
            else:
                fname = noPathInRND[-1].replace(".lsa", "_{}.rnd.lsa".format(name))
            if ' ' in fname:
                fname = "\"" + fname + "\""
            output_filenames.append(fname)
        return output_filenames

    def parseRounds(self, setup_id=None):
        #search entire file untill you find the Terrestrial data
        lineItr = iter(self.input_lines)
        output_filenames=[]
        terrestrial_lines=[]
        line_to_corrn={}
        counter = 1
        currentCorrn = "1:0:?:?:0:?" # default of no correction
        for line in lineItr:
            if "[Terrestrial]" in line:
                #create an HDIR, ZANG, and DIST record from each station pair in list
                currentLine = next(lineItr)
                counter += 1
                while currentLine != "\n":
                    self.rnd_exists = True
                    lineCheckItems = currentLine.split(":")
                    if "TerrCorrn" in lineCheckItems[0]:
                        currentCorrn = currentLine
                        if("1" in lineCheckItems[2]):
                            warning = "Atmospheric corrections are ignored by SALSA."
                            if warning not in self.warnings:
                                self.warnings.append(warning)
                    # Make sure measurement values exist
                    elif '?' not in (lineCheckItems[5], lineCheckItems[7], lineCheckItems[12], lineCheckItems[13]):
                        terrestrial_lines.append(currentLine)
                        line_to_corrn[currentLine] = currentCorrn
                    else:
                        warning = "Insufficient measurements to point {} in line {} of {}.".format(lineCheckItems[4], counter, os.path.basename(self.infile_name))
                        if warning not in self.warnings:
                            self.warnings.append(warning)

                    currentLine = next(lineItr)
                    counter += 1
                break
            counter += 1

        if self.rnd_exists and (setup_id is None):
            # add UNCR_HDIR, UNCR_ZANG, and UNCR_DIST Records
            self.uncrHdirLabel = self.addUNCRRecord("Trimble", "HDIR")
            self.uncrZangLabel = self.addUNCRRecord("Trimble", "ZANG")
            self.uncrDistLabel = self.addUNCRRecord("Trimble", "DIST_IR")

            self.addComment("")

            #add HGHT records
            for line in terrestrial_lines:
                lineItems = line.split(":")
                fromLabel = str(lineItems[2])
                toLabel = str(lineItems[4])
                instrument_height = float(lineItems[12])
                target_height = float(lineItems[13])
                setup_id = str(lineItems[16])
                status = str(lineItems[17])
                if 'E' in status or 'C' in status:
                    if abs(instrument_height) > 1e-6:
                        instrument_height_label = self.getNextHeightLabel(fromLabel, instrument_height)
                        self.addHGHTRecord(instrument_height_label, instrument_height, self.validatedHeight)
                    if abs(target_height) > 1e-6:
                        target_height_label = self.getNextHeightLabel(toLabel, target_height)
                        self.addHGHTRecord(target_height_label, target_height, self.validatedHeight)
                    if setup_id not in self.setup_ids:
                        self.setup_ids.append(setup_id)

            output_filenames = self.getRoundsOutputFilenames(terrestrial_lines)

        elif self.rnd_exists and (setup_id is not None):
            first_DGRP = True
            #add DGRP record and HDIR records
            for line in terrestrial_lines:
                lineItems = line.split(":")
                fromLabel = str(lineItems[2])
                toLabel = str(lineItems[4])
                hdir = float(lineItems[5])
                try:
                    hdir_sigma = float(lineItems[6])
                except ValueError:
                    hdir_sigma = 0.0
                target_height = float(lineItems[13])
                this_setup_id = str(lineItems[16])
                status = str(lineItems[17])
                if ('E' in status or 'C' in status) and (setup_id == this_setup_id):
                    if first_DGRP:
                        dgrp_label = self.getNextDGRPLabel()
                        self.addDGRPRecord(dgrp_label, fromLabel, self.uncrHdirLabel)
                        first_DGRP = False
                    if abs(target_height) < 1e-6:
                        target_height_label = ""
                    else:
                        target_height_label = self.getNextHeightLabel(toLabel, target_height)
                    degree, minute, second = self.decdeg_to_DMS(hdir)
                    if self.TOTSTA_HORZ_DIR.use_inst:
                        self.addHDIRRecord(toLabel, target_height_label, degree, minute, second, hdir_sigma*3600.0, dgrp_label)
                    else:
                        self.addHDIRRecord(toLabel, target_height_label, degree, minute, second, self.TOTSTA_HORZ_DIR.sigma,
                                           dgrp_label)

            self.addComment("")

            # add ZANG records records
            for line in terrestrial_lines:
                lineItems = line.split(":")
                fromLabel = str(lineItems[2])
                toLabel = str(lineItems[4])
                try:
                    zang = float(lineItems[7])
                except ValueError:
                    continue
                try:
                    zang_sigma = float(lineItems[8])
                except ValueError:
                    zang_sigma = 0.0
                instrument_height = float(lineItems[12])
                target_height = float(lineItems[13])
                this_setup_id = str(lineItems[16])
                status = str(lineItems[17])
                # get the correction value
                refract_coeff = None
                corrnLine = line_to_corrn[line]
                corrnWords = corrnLine.split(":")
                if len(corrnWords) == 6 and corrnWords[4] == "1": # there is a refraction correction given
                    try:
                        refract_coeff = float(corrnWords[5])
                    except ValueError:
                        refract_coeff = None
                if ('E' in status or 'C' in status) and (setup_id == this_setup_id):
                    if abs(instrument_height) < 1e-6:
                        instrument_height_label = ""
                    else:
                        instrument_height_label = self.getNextHeightLabel(fromLabel, instrument_height)
                    if abs(target_height) < 1e-6:
                        target_height_label = ""
                    else:
                        target_height_label = self.getNextHeightLabel(toLabel, target_height)
                    degree, minute, second = self.decdeg_to_DMS(zang)
                    if self.TOTSTA_VERT.use_inst:
                        self.addZANGRecord(fromLabel, toLabel, instrument_height_label, target_height_label, degree,
                                           minute, second, zang_sigma*3600.0, self.uncrZangLabel, refract_coeff)
                    else:
                        self.addZANGRecord(fromLabel, toLabel, instrument_height_label, target_height_label, degree,
                                           minute, second, self.TOTSTA_VERT.sigma, self.uncrZangLabel, refract_coeff)

            self.addComment("")

            # add DIST records records
            for line in terrestrial_lines:
                lineItems = line.split(":")
                fromLabel = str(lineItems[2])
                toLabel = str(lineItems[4])
                try:
                    dist = float(lineItems[9])
                except ValueError:  # ? for dist
                    continue
                try:
                    dist_sigma = float(lineItems[10])
                except ValueError:  # ? for sigma
                    dist_sigma = 0.0  # will warn the user for zero sigma in the GUI
                instrument_height = float(lineItems[12])
                target_height = float(lineItems[13])
                this_setup_id = str(lineItems[16])
                status = str(lineItems[17])
                if ('E' in status or 'C' in status) and (setup_id == this_setup_id):
                    if abs(instrument_height) < 1e-6:
                        instrument_height_label = ""
                    else:
                        instrument_height_label = self.getNextHeightLabel(fromLabel, instrument_height)
                    if abs(target_height) < 1e-6:
                        target_height_label = ""
                    else:
                        target_height_label = self.getNextHeightLabel(toLabel, target_height)
                    if self.TOTSTA_DIST_IR.use_inst:
                        try:
                            sigmaUnitsConversion = self.distSigMap.get(self.distUnits)
                            if sigmaUnitsConversion is None:
                                raise Exception('Unable to find conversion for units of {0}'.format(self.distUnits))

                        except Exception as E:
                            print(E.args[0])
                            exit(1)

                        self.addDISTRecord(fromLabel, toLabel, instrument_height_label, target_height_label, dist,
                                           dist_sigma * sigmaUnitsConversion, self.uncrDistLabel, self.validatedDist)
                    else:
                        try:
                            sigmaUnitsConversion = self.mapMetersToUnits.get(self.distUnits)
                            if sigmaUnitsConversion is None:
                                raise Exception('Unable to find conversion for units of {0}'.format(self.distUnits))

                        except Exception as E:
                            print(E.args[0])
                            exit(1)

                        self.addDISTRecord(fromLabel, toLabel, instrument_height_label, target_height_label, dist,
                                           self.TOTSTA_DIST_IR.sigma * sigmaUnitsConversion, self.uncrDistLabel, self.validatedDist)
        return output_filenames, len(self.output_records)

    def parsePOSG(self):
        #search entire file until you find the stations list
        pos_exists = False
        lineItr = iter(self.input_lines)        
        for line in lineItr:            
            if "[Stations]" in line:
                #create a POSG record from each station in list
                currentLine = next(lineItr)
                while currentLine != "\n":
                    lineItems = currentLine.split(":")
                    if lineItems[3] == "?" or lineItems[4] == "?" or lineItems[5] == "?":
                        message = "Warning - invalid lat, lon, or height component at line {0} of {1}. Station Data omitted.".format(self.input_lines.index(currentLine) + 1, os.path.basename(self.infile_name))
                        if message not in self.warnings:
                            self.warnings.append(message)
                        currentLine = next(lineItr)
                        continue
                    pos_exists = True
                    label = lineItems[2]
                    lat = lineItems[3]
                    lon = lineItems[4]
                    height = float(lineItems[5])
                    CTRL = lineItems[-2]
                    #separate Degree from Direction
                    latDecDeg = float(lat[0:-1])
                    latDir = lat[-1]
                    lonDecDeg = float(lon[0:-1])
                    lonDir = lon[-1]                    
                    self.addPosgRecord(label, latDecDeg, latDir, lonDecDeg, lonDir, height, self.validatedHeight, CTRL)
                    currentLine = next(lineItr)
                break                   
        return pos_exists

    def writeHeader(self,f):
        datestr = datetime.now().strftime("%H:%M:%S %b %d, %Y")
        #if infile_name is a path instead of a file name, pull out file name
        noPathInName = self.infile_name.split("/")        
        f.write("Created by SALSA version {0}\n".format(self.SALSA_VERSION_STRING))
        f.write("# Converted with TrimbleToLSA.py converter from {0} on {1}\n".format(noPathInName[-1],datestr))
        f.write("\n")
    
    def writeOutLSA(self):            
        try:
            gps_exists = self.parseDXYZ()
            lvl_exists = self.parseHDIF()
            pos_exists = self.parsePOSG()
            self.rounds_filenames, self.last_output_record_num = self.parseRounds()
            #create a list of all UNCRRecords from output_records                    
            uncrRecordList = filter(lambda x: x.get_lsa_string()[0:5] == "UNCR ", self.output_records)
            with open(self.outfile_name,'w') as f:
                self.writeHeader(f)
                for uncertainty in uncrRecordList:
                    f.write(uncertainty.get_lsa_string() + "\n")
                f.write("\n")
                if self.rnd_exists:
                    hghtRecordList = filter(lambda x: x.get_lsa_string()[0:5] == "HGHT ", self.output_records)
                    for height in hghtRecordList:
                        f.write(height.get_lsa_string() + "\n")
                    f.write("\n")
                #if outfile_name is a path instead of a file name, pull out file name
                noPathInPOS = self.outfile_name.split("/")
                noPathInGPS = self.outfile_name.split("/")
                noPathInLVL = self.outfile_name.split("/")
                posOut_name = noPathInPOS[-1].replace(".lsa", ".pos.lsa")
                gpsOut_name = noPathInGPS[-1].replace(".lsa", ".gps.lsa")
                lvlOut_name = noPathInLVL[-1].replace(".lsa", ".lvl.lsa")
                if ' ' in posOut_name:
                    posOut_name = "\"" + posOut_name + "\""
                if ' ' in gpsOut_name:
                    gpsOut_name = "\"" + gpsOut_name + "\""
                if ' ' in lvlOut_name:
                    lvlOut_name = "\"" + lvlOut_name + "\""
                if pos_exists:
                    f.write("#--include {0}\n".format(posOut_name))
                if gps_exists:
                    f.write("--include {0}\n".format(gpsOut_name))
                if lvl_exists:
                    f.write("--include {0}\n".format(lvlOut_name))
                if self.rnd_exists:
                    for fname in self.rounds_filenames:
                        f.write("--include {0}\n".format(fname))

        except EnvironmentError:
            print("Error - unable to find/write {0}. Exiting.\n".format(self.outfile_name), file=sys.stderr)
            exit(1)

    def writeOutGPS(self):        
        try:
            gps_exists = self.parseDXYZ()
            if not gps_exists:
                return
            for warning in self.warnings:
                self.write_warning(warning)
            with open(self.outfile_name,'w') as f:
                self.writeHeader(f)
                for record in self.output_records:
                    if record.get_lsa_string()[0:5] != "UNCR ":                  
                        f.write(record.get_lsa_string() + "\n")
        except EnvironmentError:
            print("Error - unable to find/write {0}. Exiting.\n".format(self.outfile_name), file=sys.stderr)
            exit(1)
        except RuntimeError as error:
            print("Error - unexpected value found while parsing.\n{}".format(error), file=sys.stderr)
            exit(1)
            
    def writeOutLVL(self):
        try:
            lvl_exists = self.parseHDIF()
            if not lvl_exists:
                return
            for warning in self.warnings:
                self.write_warning(warning)
            with open(self.outfile_name,'w') as f:
                self.writeHeader(f)
                for record in self.output_records:
                    if record.get_lsa_string()[0:5] != "UNCR ":
                        f.write(record.get_lsa_string() + "\n")
        except EnvironmentError:
            print("Error - unable to find/write {0}. Exiting.\n".format(self.outfile_name), file=sys.stderr)
            exit(1)
        except RuntimeError as error:
            print("Error - unexpected value found while parsing.\n{}".format(error), file=sys.stderr)
            exit(1)

    def writeOutRND(self):
        try:
            pathToRND = os.path.dirname(self.outfile_name)
            self.rounds_filenames, self.last_output_record_num = self.parseRounds()
            if not self.rnd_exists:
                 return
            for warning in self.warnings:
                self.write_warning(warning)
            for indx in range(len(self.rounds_filenames)):
                fileName = self.rounds_filenames[indx].strip('\"')
                with open(os.path.join(pathToRND, fileName),'w') as f:
                    self.writeHeader(f)
                    _ , output_record_num = self.parseRounds(self.setup_ids[indx])
                    for record in self.output_records[self.last_output_record_num:]:
                        f.write(record.get_lsa_string() + "\n")
                    self.last_output_record_num = output_record_num
        except EnvironmentError:
            print("Error - unable to find/write {0}. Exiting.\n".format(self.outfile_name), file=sys.stderr)
            exit(1)
        except RuntimeError as error:
            print("Error - unexpected value found while parsing.\n{}".format(error), file=sys.stderr)
            exit(1)

    def writeOutPOS(self):
        pos_exists = self.parsePOSG()
        if not pos_exists:
            return
        try:
            with open(self.outfile_name,'w') as f:
                self.writeHeader(f)
                for record in self.output_records:
                    f.write("{0}\n".format(record.get_lsa_string()))
        except EnvironmentError:
            print("Error - unable to find/write {0}. Exiting.\n".format(self.outfile_name), file=sys.stderr)
            exit(1)

    #Implementation of abstract methods
    def isValid(self):
        for line in self.input_lines:
            if "[General]" in line:
                return True
        return False

    #Trimble only has one version
    def getVersion(self):
        return "Version 1"

    def writeOutputFile(self):
        if self.outFileType == "LSA":
            self.writeOutLSA()
        elif self.outFileType == "GPS":
            self.writeOutGPS()
        elif self.outFileType == "LVL":
            self.writeOutLVL()
        elif self.outFileType == "POS":
            self.writeOutPOS()
        elif self.outFileType == "RND":
            self.writeOutRND()
        else:
            print("Error - output file is of unknown type", file=sys.stderr)
            exit(1) 

    def convert(self, isLSA=False):
        try:
            self.readInputFile()
        except:
            print('Error - unable to read {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        
        try:
            if not self.isValid():
                print("Error - [General] header not found.  {0} is not a valid .asc file.  Exiting.\n".format(self.infile_name), file=sys.stderr)
                exit(1)
        except:
            print('Error - unable to determine validity of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        
        try:
            version = self.getVersion()
        except:
            print('Error - unable to determine version of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        
        self.OutputFileType(self.outfile_name)
        if not self.validOutputFile():
            print("Error - output file must have either .gps.lsa, .pos.lsa, .lvl.lsa, .rnd.lsa, or .lsa as file extension", file=sys.stderr)
            exit(1)
                
        try:
            self.parseGeneral(isLSA)
        except RuntimeError as error:
            print('Error - unable to parse needed information from the "[General]" section', file=sys.stderr)
            print(error, file=sys.stderr)
            exit(1)

        self.writeOutputFile()
        if self.warnings_generated:
            print("Warning - warnings generated when parsing {}. Refer to {} for details.".format(os.path.basename(self.infile_name), self.warnfile_name))



