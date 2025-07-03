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
#LSA imports
from lsaconverter import LSAConverter
from lsaconverter import Config

class LSAPoint(LSAConverter):

    #Implementation of abstract methods
    def writeOutputFile(self):
        datestr = datetime.now().strftime("%H:%M:%S %b %d, %Y")
        try:
            with open(self.outfile_name,'w') as f:
                f.write("Created by SALSA version {0}\n".format(self.SALSA_VERSION_STRING))
                f.write("# Converted with the {0} {1} on {2}\n\n".format(self.converter_string,self.infile_name,datestr))
                for record in self.output_records:
                    out_string = record.get_lsa_string()
                    f.write("{0}\n".format(out_string))
        except EnvironmentError:
            print("Error - unable to find/write {0}. Exiting.\n".format(self.outfile_name), file=sys.stderr)
            exit(1)

    def getVersion(self):
        if "grape" in self.input_lines[0]:
            start = self.input_lines[0].find('Ver')
            end = self.input_lines[0].find('Run')
            version_str = 'grape ' + self.input_lines[0][start:end-2]
        else:
            version_str = "merge"
        return version_str

    def isValid(self):
        for line in self.input_lines:
            if ("MERGEDGDP" in line) or ("FINALFKFGDP" in line):
                return True
        return False

    def convert(self):
        try:
            self.readInputFile()
        except:
            print('Error - unable to read {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        try:
            if not self.isValid():
                print("Error - neither MERGEDGDP nor FINALFKFGDP found.  {0} is not a valid grape or merge output file.  Exiting.\n".format(self.infile_name), file=sys.stderr)
                exit(1)
        except:
            print('Error - unable to determine validity of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        try:
            version = self.getVersion()
        except:
            print('Error - unable to determine version of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        self.converter_string = "PPP converter from {0} output".format(version)
        if 'grape' in version:
            self.parseGrapeOutput()
        else:
            self.parseMergeOutput()        
        self.writeOutputFile()
        if self.warnings_generated:
            print("Warning - warnings generated when parsing {}. Refer to {} for details.".format(os.path.basename(self.infile_name), self.warnfile_name))


    #non-abstract methods
    def parseMergeOutput(self):
        ht_next_line = False
        latlon_next_line = False
        latDeg=0
        latMin=0
        latSec=0.0
        latDir = ''
        lonDeg=0
        lonMin=0
        lonSec=0
        height=0
        cxx=0.0
        cxy=0.0
        cxz=0.0
        cyy=0.0
        cyz=0.0
        czz=0.0

        (currentLineNumber,line) = self.getNextLine(-1)
        while currentLineNumber < self.num_lines:
            try:
                if 'MERGEDGDP' in line:
                    if 'Geodetic position' in line:
                        ht_next_line = True
                    elif 'dms' in line:
                        if 'Lat S' in line:
                            latDir = 'S'
                        else:
                            latDir = 'N'
                        latlon_next_line = True
                    elif ht_next_line:
                        parts = line.split()
                        height = float(parts[3])
                        ht_next_line = False
                    elif latlon_next_line:
                        parts = line.split()
                        latDeg = int(parts[1])
                        latMin = int(parts[2])
                        latSec = float(parts[3])
                        lonDeg = int(parts[7])
                        lonMin = int(parts[8])
                        lonSec = float(parts[9])
                        latlon_next_line = False
                elif ('MERGEDGDC' in line) and ('covariance' not in line):
                    if 'North' in line:
                        parts = line.split()
                        cxx = float(parts[3])
                        cxy = float(parts[4])
                        cxz = float(parts[5])
                    elif 'East' in line:
                        parts = line.split()
                        cyy = float(parts[4])
                        cyz = float(parts[5])
                    elif 'Ht' in line:
                        parts = line.split()
                        czz = float(parts[5])
                        self.addPosgWithCovarianceRecord(self.getPointName(), latDeg, latMin, latSec, latDir, lonDeg, lonMin, lonSec, 'W', height, 'm', cxx, cxy, cxz, cyy, cyz, czz)
                        break
                (currentLineNumber,line) = self.getNextLine(currentLineNumber)
            except:
                self.write_warning('Warning - unable to parse line {0} of {1}. line: >>> {2}'.format(
                    str(currentLineNumber+1),os.path.basename(self.infile_name),self.input_lines[currentLineNumber]))
                (currentLineNumber,line) = self.getNextLine(currentLineNumber)

    def parseGrapeOutput(self):
        ht_next_line = False
        latlon_next_line = False
        latDeg=0
        latMin=0
        latSec=0.0
        latDir = ''
        lonDeg=0
        lonMin=0
        lonSec=0
        height=0
        cxx=0.0
        cxy=0.0
        cxz=0.0
        cyy=0.0
        cyz=0.0
        czz=0.0
        (currentLineNumber,line) = self.getNextLine(-1)
        while currentLineNumber < self.num_lines:
            try:
                if 'FINALFKFGDP' in line:
                    if 'Geodetic position' in line:
                        ht_next_line = True
                    elif 'dms' in line:
                        if 'Lat S' in line:
                            latDir = 'S'
                        else:
                            latDir = 'N'
                        latlon_next_line = True
                    elif ht_next_line:
                        parts = line.split()
                        height = float(parts[3])
                        ht_next_line = False
                    elif latlon_next_line:
                        parts = line.split()
                        latDeg = int(parts[1])
                        latMin = int(parts[2])
                        latSec = float(parts[3])
                        lonDeg = int(parts[7])
                        lonMin = int(parts[8])
                        lonSec = float(parts[9])
                        latlon_next_line = False
                elif ('FINALFKFGDC' in line) and ('covariance' not in line):
                    if 'North' in line:
                        parts = line.split()
                        cxx = float(parts[3])
                        cxy = float(parts[4])
                        cxz = float(parts[5])
                    elif 'East' in line:
                        parts = line.split()
                        cyy = float(parts[4])
                        cyz = float(parts[5])
                    elif 'Ht' in line:
                        parts = line.split()
                        czz = float(parts[5])
                        self.addPosgWithCovarianceRecord(self.getPointName(), latDeg, latMin, latSec, latDir, lonDeg, lonMin, lonSec, 'W', height, 'm', cxx, cxy, cxz, cyy, cyz, czz)
                        break
                (currentLineNumber,line) = self.getNextLine(currentLineNumber)
            except:
                self.write_warning('Warning - unable to parse line {0} of {1}. line: >>> {2}'.format(
                    str(currentLineNumber+1),os.path.basename(self.infile_name),self.input_lines[currentLineNumber]))
                (currentLineNumber,line) = self.getNextLine(currentLineNumber)

    def getPointName(self):
        filename = os.path.basename(self.infile_name)
        if 'WIPPS_job_' in filename:
            parts = filename.split('_')
            pointName = parts[3]
            index = pointName.find('.')
            if index != -1:
                pointName = pointName[:index]
        elif 'Merge_job_' in filename:
            parts = filename.split('_')
            pointName = parts[3]
            index = pointName.find('.')
            if index != -1:
                pointName = pointName[:index]
        else:
            pointName = 'newPOSG'
        return pointName
