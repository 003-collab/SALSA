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

#reads an OPUS file and generates an .LSA file with POSG records for
#NAD83 (if shown) and ITRF/IGS positions 
#B. Beal 1/15/2020

#python imports
from datetime import datetime
import sys
import os
#LSA imports
from lsaconverter import LSAConverter

class OpusPoint(LSAConverter):

    def getVersion(self):
        version_str = ""
        for line in self.input_lines:
            if ("SOFTWARE:" in line):
                start = line.find('SOFTWARE:')+10
                end = line.find('START:')
                version_str = line[start:end].strip()
        return version_str


    def isValid(self):
        for line in self.input_lines:
            if ("SOLUTION REPORT" in line):
                return True
        return False


    def getPointName(self):
        #Opus does not show the point name in the report file!
        #Use the filename instead (before the '.')
        filename = os.path.basename(self.infile_name)
        firstDot = filename.find('.')
        pointName = filename[0:firstDot]
        return pointName


    def parseOpusOutput(self):
        latDegITRF=0
        latMinITRF=0
        latSecITRF=0.0
        latDirITRF = ''
        latDegNAD=0
        latMinNAD=0
        latSecNAD=0.0
        latDirNAD = ''
        latPeak = 0.0
        lonDegITRF=0
        lonMinITRF=0
        lonSecITRF=0
        lonDirITRF= ''
        lonDegNAD=0
        lonMinNAD=0
        lonSecNAD=0
        lonDirNAD= ''
        lonPeak = 0.0
        heightITRF=0
        heightNAD=0
        heightPeak = 0.0

        cxx = 0.0
        cxy = 0.0
        cxz = 0.0
        cyy = 0.0
        cyz = 0.0
        czz = 0.0

        ITRFindex = 0
        NADindex = 0
        NADflag = False
        ITRFflag = False
        ref_frame_ITRF=''
        ref_frame_NAD=''
        cFlag = False   #covariance flag

        (currentLineNumber,line) = self.getNextLine(-1)
        while currentLineNumber < self.num_lines:
#             REF FRAME: NAD_83(PA11)(EPOCH:2010.0000)          ITRF2014 (EPOCH:2020.0005)

#          X:     -6100258.791(m)   0.031(m)          -6100260.193(m)   0.031(m)
#          Y:      -996506.160(m)   0.041(m)           -996502.600(m)   0.041(m)
#          Z:     -1567978.831(m)   0.012(m)          -1567977.234(m)   0.012(m)
#         0     1  2   3               4            5   6  7              8
#        LAT:  -14 19 33.98225      0.016(m)       -14 19 33.92540      0.016(m)
#      0 1      2  3    4              5            6   7   8               9
#      E LON:  189 16 39.33332      0.045(m)       189 16 39.20851      0.045(m)
#      W LON:  170 43 20.66668      0.045(m)       170 43 20.79149      0.045(m)
#     EL HGT:           53.085(m)   0.022(m)                53.474(m)   0.022(m)
#  ORTHO HGT:           19.710(m)   0.049(m) [ H = h-N (N = GEOID12B HGT)]
            try:
                if 'REF FRAME:' in line:
                    parts = line.split()
                    for i in range(0,len(parts)):
                        if 'NAD' in parts[i]:
                            NADflag=True
                            ref_frame_NAD = parts[i]
                        if ('ITRF' in parts[i]) or ('IGS' in parts[i]):
                            ITRFflag=True
                            ref_frame_ITRF = parts[i] + parts[i+1]
                if NADflag == True:
                    ITRFindex = 4
                if 'LAT:' in line:
                    parts = line.split()
                    #Latitude info:
                    if NADflag == True:
                        latDegNAD = int(parts[NADindex+1])
                        latMinNAD = int(parts[NADindex+2])
                        latSecNAD = float(parts[NADindex+3])
                        latPeakStr = parts[NADindex+4]
                        latPeak = float(latPeakStr[:-3:])  #need to remove '(m)'
                        if parts[NADindex+1][0] == '-':
                            if latDegNAD != 0: #int(-0) = 0
                                latDegNAD = -1*latDegNAD
                            latDirNAD = 'S'
                        else:
                            latDirNAD = 'N'
                    if ITRFflag == True:
                        latDegITRF = int(parts[ITRFindex+1])
                        latMinITRF = int(parts[ITRFindex+2])
                        latSecITRF = float(parts[ITRFindex+3])
                        latPeakStr = parts[ITRFindex+4]
                        latPeak = float(latPeakStr[:-3:])  #need to remove '(m)'
                        if parts[ITRFindex+1][0] == '-':
                            if latDegITRF != 0: #int(-0) = 0
                                latDegITRF = -1*latDegITRF
                            latDirITRF = 'S'
                        else:
                            latDirITRF = 'N'

                if 'E LON:' in line:
                    #E Longitude info:
                    parts = line.split()
                    if NADflag == True:
                        lonDegNAD = int(parts[NADindex+2])
                        lonMinNAD = int(parts[NADindex+3])
                        lonSecNAD = float(parts[NADindex+4])
                        lonPeakStr = parts[NADindex+5]
                        lonPeak = float(lonPeakStr[:-3:])  #need to remove '(m)'
                    if ITRFflag == True:
                        lonDegITRF = int(parts[ITRFindex+2])
                        lonMinITRF = int(parts[ITRFindex+3])
                        lonSecITRF = float(parts[ITRFindex+4])
                        lonPeakStr = parts[ITRFindex+5]
                        lonPeak = float(lonPeakStr[:-3:])  #need to remove '(m)'
                if 'W LON' in line:
                    #W Longitude info:
                    parts = line.split()
                    if NADflag == True:
                        lonTemp = int(parts[NADindex+2])
                        #If west long is smaller than east lon, then use west lon in salsa:
                        if lonTemp<180:
                            lonDirNAD = 'W'
                            lonDegNAD = int(parts[NADindex+2])
                            lonMinNAD = int(parts[NADindex+3])
                            lonSecNAD = float(parts[NADindex+4])
                        else:
                            lonDirNAD = 'E'
                    if ITRFflag == True:
                        lonTemp = int(parts[ITRFindex+2])
                        #If west long is smaller than east lon, then use west lon in salsa:
                        if lonTemp<180:
                            lonDirITRF = 'W'
                            lonDegITRF = int(parts[ITRFindex+2])
                            lonMinITRF = int(parts[ITRFindex+3])
                            lonSecITRF = float(parts[ITRFindex+4])
                        else:
                            lonDirITRF = 'E'
                if 'EL HGT:' in line:
                    #Ellipsoid height info:
                    parts = line.split()
                    if NADflag == True:
                        heightStr = parts[NADindex+2]
                        heightNAD = float(heightStr[:-3:])  #need to remove '(m)'
                        heightPeakStr = parts[NADindex+3]
                        heightPeak = float(heightPeakStr[:-3:])  #need to remove '(m)'
                        ITRFindex = 3
                    if ITRFflag == True:
                        if NADflag == True:
                            heightStr = parts[ITRFindex+1]
                            heightITRF = float(heightStr[:-3:])  #need to remove '(m)'
                            heightPeakStr = parts[ITRFindex+2]
                            heightPeak = float(heightPeakStr[:-3:])  #need to remove '(m)'
                        else:
                            heightStr = parts[ITRFindex+2]
                            heightITRF = float(heightStr[:-3:])  #need to remove '(m)'
                            heightPeakStr = parts[ITRFindex+3]
                            heightPeak = float(heightPeakStr[:-3:])  #need to remove '(m)'
                if 'Covariance Matrix for the enu OPUS Position' in line:
                    cFlag = True
                    (currentLineNumber, line) = self.getNextLine(currentLineNumber)
                    parts = line.split()
                    cxx,cxy,cxz = float(parts[0]), float(parts[1]), float(parts[2])
                    (currentLineNumber, line) = self.getNextLine(currentLineNumber)
                    parts = line.split()
                    cyy,cyz = float(parts[1]), float(parts[2])
                    (currentLineNumber, line) = self.getNextLine(currentLineNumber)
                    parts = line.split()
                    czz = float(parts[2])
                (currentLineNumber,line) = self.getNextLine(currentLineNumber)
            except:
                self.write_warning('Warning - unable to parse line {0} of {1}. line: >>> {2}'.format(
                    str(currentLineNumber + 1), os.path.basename(self.infile_name), self.input_lines[currentLineNumber]))
                (currentLineNumber, line) = self.getNextLine(currentLineNumber)

        #Covariance info:
        #Note1: OPUS provides only 'peak-to-peak' error, not covariance terms
        #       Use the 'peak-to-peak' approximates standard dev.
        #       Using any fraction thereof gives overly optimistic solution
        #       Leave co-var terms as zero.
        #Note2: The peak-to-peak errors are the same for NAD and ITRF
        if not cFlag:
            cxx = (lonPeak)**2
            cyy = (latPeak)**2
            czz = (heightPeak)**2

        if cFlag:
            self.addComment('Co-var from extended OPUS results.')
        else:
            self.addComment('Peak-to-peak errors used as std deviations.')

        # Example POSG record:
        # POSG newPOSG Con 30 0 0.00000 N 120 0 0.00000 W 100.0000 m ...
        #  1.000e-04          0.000e+00          0.000e+00 ...
        #                     1.000e-03          0.000e+00 ...
        #                                        1.000e-03
        if NADflag == True:
            if latDirNAD != '' and lonDirNAD != '':
                self.addComment(ref_frame_NAD)
                self.addPosgWithCovarianceRecord(self.getPointName() + '_' + ref_frame_NAD, latDegNAD, latMinNAD, latSecNAD,
                                                 latDirNAD, lonDegNAD, lonMinNAD, lonSecNAD, lonDirNAD, heightNAD, 'm',
                                                 cxx, cxy, cxz, cyy, cyz, czz)
        if ITRFflag == True:
            #by default, the ITRF position is disabled
            if latDirITRF != '' and lonDirITRF != '':
                self.addComment(ref_frame_ITRF)
                self.addPosgWithCovarianceRecord(self.getPointName() + '_' + ref_frame_ITRF, latDegITRF, latMinITRF, latSecITRF,
                                                 latDirITRF, lonDegITRF, lonMinITRF, lonSecITRF, lonDirITRF, heightITRF,
                                                 'm', cxx, cxy, cxz, cyy, cyz, czz)


    def convert(self):
        self.readInputFile()
        if not self.isValid():
            print("Error - SOLUTION REPORT not found.  {0} is not a valid OPUS output file.  Exiting.\n".format(self.infile_name), file=sys.stderr)
            exit(1)
        version = self.getVersion()
        if version == "":
            print('Error - unable to determine version of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        self.converter_string = 'OPUS converter from OPUS SOFTWARE "{0}" output'.format(version)
        self.parseOpusOutput()
        self.writeOutputFile()
        if self.warnings_generated:
            print("Warning - warnings generated when parsing {}. Refer to {} for details.".format(os.path.basename(self.infile_name), self.warnfile_name))


    
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
