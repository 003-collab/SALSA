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
from datetime import datetime
import sys, os, math
import enum
from functools import total_ordering
#LSA imports
from lsaconverter import LSAConverter

# enum class to facilitate POS type preference ordering
@total_ordering
class PosType(enum.Enum):
    REF = 7
    ADJ_cov = 6
    ADJ = 5
    MEAN_cov = 4
    MEAN = 3
    MEAS_cov = 2
    MEAS = 1
    NAV = 0
    def __lt__(self, other):
        if self.__class__ is other.__class__:
            return self.value < other.value
        return NotImplemented
    def __gt__(self, other):
        if self.__class__ is other.__class__:
            return self.value > other.value
        return NotImplemented
    def __eq__(self, other):
        if self.__class__ is other.__class__:
            return self.value == other.value
        return NotImplemented


class LeicaGNSSConverter(LSAConverter):
    '''This class handles the conversion of Leica GNSS .asc exported files into SALSA .lsa files
       containing records conforming to the SALSA record specification.

       This class was modeled on the TrimbleToLSA class, which is instantiated once for each
       expected output file type (one containing POSC records, one containing DXYZ records, and
       one containing INCLUDE, UNCR, and HGHT records.

       The input files must be in the Cartesian coordinate frame, with WGS 1984 reference ellipse,
       using meters for units.

       See gitlab Issue #1450 for history and development details.
    '''
    outFileType = ""
    firstTimeStatic = True
    uncrStatLabel = ""
    warnings_generated = False
    warnings = []

    def OutputFileType(self, outputfile):
        if outputfile.endswith(".gps.lsa"):
            self.outFileType = "GPS"
        elif outputfile.endswith(".pos.lsa"):
            self.outFileType = "POS"
        elif outputfile.endswith(".lsa"):
            self.outFileType = "LSA"
        else:
            self.outFileType = "Invalid"

    def validOutputFile(self):
        if self.outFileType == "Invalid":
            return False
        return True

    def parseDXYZ(self):
        '''we're looking for lines of the form:
           @+F2-3U                 0.52507388944     -1.49385243792       -25.395
           @-F2-11B               -0.00024448416      0.00041547545        -0.823
           @=   0.46548   0.000000001526   -0.000000000126   -0.000000000371   0.000000001537   0.000000000109   0.000000010538
           @:          1.735          0.000
           @;          1.590          0.000
           @*02.03.2009 09:21:45

           Where the tags at the beginning of the line represent:        
           @+ Individual baseline information (Reference point of baseline and its coordinates)
           @- Baseline vector components
           @= Variance-covariance information for baseline vector
           @: Reference antenna height and offset
           @; Rover antenna height and offset
           @* Date and time of first common epoch

           Locate the indices of all @+ records in self.input_lines, then go back and attempt to parse the expected group of
           6 lines into DXYZ and HGHT record.  HGHT records can change, so need to check if different from existing ones.

           Returns:
               gps_exists (bool) - True of valid DXYZ records are parsed, False if not.
        '''
        gps_exists = False
        baseline_start_indices = []
        lineItr = iter(self.input_lines)
        antenna_heights = {}
        i = 0

        # find the baseline records
        for line in lineItr:
            if line[:2] == "@+":
                baseline_start_indices.append(i)
            i += 1

        # loop over the Leica baseline records to construct Salsa DXYZ records
        for index in baseline_start_indices:
            parts = self.input_lines[index].strip().split()
            base_name = parts[0][2:]
            if self.input_lines[index+1][:2] == "@-":
                parts = self.input_lines[index+1].strip().split()
                rover_name = parts[0][2:]
                dx = float(parts[1])
                dy = float(parts[2])
                dz = float(parts[3])
                if self.input_lines[index+2][:2] == "@=":
                    parts = self.input_lines[index+2].strip().split()
                    m0 = float(parts[1])
                    cxx = m0*m0*float(parts[2])
                    cxy = m0*m0*float(parts[3])
                    cxz = m0*m0*float(parts[4])
                    cyy = m0*m0*float(parts[5])
                    cyz = m0*m0*float(parts[6])
                    czz = m0*m0*float(parts[7])
                    if self.input_lines[index+3][:2] == "@:":
                        parts = self.input_lines[index+3].strip().split()
                        ref_hgt = float(parts[1])
                        if not math.isclose(ref_hgt, 0.0, abs_tol=0.00001):  # Don't create HGHT records for zero heights
                            hfrom_label, antenna_heights = self.create_HGHT_record(antenna_heights, base_name, ref_hgt)
                        else:
                            hfrom_label = None
                        if self.input_lines[index+4][:2] == "@;":
                            parts = self.input_lines[index+4].strip().split()
                            rov_hgt = float(parts[1])
                            if not math.isclose(rov_hgt, 0.0, abs_tol=0.00001):  # Don't create HGHT records for zero heights
                                hto_label, antenna_neights = self.create_HGHT_record(antenna_heights, rover_name, rov_hgt)
                            else:
                                hto_label = None
                            if self.input_lines[index+5][:2] == "@*":
                                parts = self.input_lines[index+5].strip().split()
                                #get start time.  End time not reported.
                                date_str = parts[0][2:].replace(".", "-")
                                time_str = parts[1]
                                if self.firstTimeStatic:
                                    #add UNCR_STATIC Record
                                    self.firstTimeStatic = False
                                    self.uncrStatLabel = self.addUNCRRecord("Leica", "DXYZ_STATIC")
                                # Add DXYZ record to list of output records
                                self.addDeltaRecord(base_name, rover_name, dx, dy, dz, 'm', cxx, cxy, cxz, cyy, cyz, czz, self.uncrStatLabel, date_str, "???", time_str, "???")
                                # Set HFROM and HTO labels on the newly-added DXYZ record
                                if not hfrom_label is None:
                                    self.output_records[-1].set_from_height_label(hfrom_label)
                                if not hto_label is None:
                                    self.output_records[-1].set_to_height_label(hto_label)
                                gps_exists = True
                            else:
                                continue  # expected start date/time not found
                        else:
                            continue  # expected rover antenna height not found
                    else:
                        continue  # expected reference antenna height not found
                else:
                    continue  # expected covariance info not found
            else:
                continue  # expected vector not found

        return gps_exists

    def create_HGHT_record(self, antenna_heights, site_name, height):
        '''Check if an antenna height has already been stored for this site.
           If it has, and it has the same value, return the already stored HGHT label.
           If it has, but has a different value, or if it hasn't been stored, generate a
           HGHT label for it and add it to the list.

           Arguments:
               antenna_heights (dict) - keys = HGHT record labels derived from site_name,
                                        values = height values
               site_name (str)        - Name of a FROM or TO site from an DXYZ record
               height (float)         - antenna height from a DXYZ record for one of the sites

           Return:
               label, antenna_heights - Unique label corresponding to the site_name<->height value pair,
                                        dict of all current label<->height value pairs
        '''
        i = 1
        label = "HGHT#1_" + site_name
        while label in antenna_heights:
            if math.isclose(antenna_heights[label], height, abs_tol=0.00001):  # height value already stored in existing label
                return label, antenna_heights
            i += 1
            label = "HGHT#{}_{}".format(i, site_name)

        # new height value for this site not already captured.  Add it to the list.
        antenna_heights[label] = height
        self.addHGHTRecord(label, height)
        return label, antenna_heights

    def parsePOSC(self):
        '''We're looking for lines of the form:
           @#GRAR                352456.5190  -4659605.8631   4326698.4956            REF   12
           ideally, for reference stations, but there may also be adjusted/measured sites with covariance info:
           @#AA-1                352456.5190  -4659605.8631   4326698.4956            MEAS    0.000  22
           @&   0.31837   0.000000001479   -0.000000000061   -0.000000000407   0.000000001504   -0.000000000006   0.000000010129

           Gather all @# records and take the first REF, if it exists.  This will be POSC fixed.
           If no REF record exist, but other non-REF records POS records exist in the file, one of the records will be converted
           to a POSC LSA record based on the following preference order:

           REF > ADJ + cov > ADJ > MEAN + cov > MEAN > MEAS + cov > MEAS > NAV (or other) 

           Returns:
               pos_exists (bool) - True if a valid POSC record was parsed, False if not.
        '''
        pos_exists = False
        namelist={}
        pos_start_indices=[]
        lineItr = iter(self.input_lines)


        # find the pos records
        i=0
        for line in lineItr:
            if line[:2] == "@#":
                pos_start_indices.append(i)
            i += 1

        # loop over pos records
        for index in pos_start_indices:
            line =  self.input_lines[index].strip()
            parts = line.split()  # name, position, and type
            name = parts[0][2:]
            try:
                postype = PosType[parts[4]]
            except KeyError:
                postype = PosType.NAV
            if postype == PosType.REF:  # add this to the list of site, but don't expect covariance info
                namelist[name] = (line, postype)
            else:
                # check for accompanying covariance info (CONSTRAINED)
                if (index+1 < len(self.input_lines)) and self.input_lines[index+1][:2] == "@&":
                    if postype > PosType.NAV:
                        postype = PosType[parts[4] + "_cov"]
                        line += self.input_lines[index+1].strip()
                if not (name in namelist):  # new non-REF site
                    namelist[name] = (line, postype)
                else:  # site already in the list, check if postype is higher in the preference order
                    (stored_line, stored_postype) = namelist[name]
                    if postype > stored_postype:
                        namelist[name] = (line, postype)

        if len(namelist) == 0:
            return pos_exists
        else:
            pos_exists = True
            for key, value in namelist.items():
                (line, type) = value
                parts = line.split()
                label = parts[0][2:]
                x = float(parts[1])
                y = float(parts[2])
                z = float(parts[3])
                # Check if fixed type is CONSTRAINED
                if (type == PosType.ADJ_cov) or (type == PosType.MEAN_cov) or (type == PosType.MEAS_cov):
                    cov_string = line[line.find("@&"):]
                    cov_parts = cov_string.split()
                    m0 = float(cov_parts[1])
                    cxx = m0*m0*float(cov_parts[2])
                    cxy = m0*m0*float(cov_parts[3])
                    cxz = m0*m0*float(cov_parts[4])
                    cyy = m0*m0*float(cov_parts[5])
                    cyz = m0*m0*float(cov_parts[6])
                    czz = m0*m0*float(cov_parts[7])
                    self.addPoscWithCovarianceRecord(label, x, y, z, 'm', cxx, cxy, cxz, cyy, cyz, czz)
                elif type == PosType.REF:  # FIXED
                    self.addPoscRecord(label, x, y, z, 'm', 'FIX')
                else:  # FLOATING
                    self.addPoscRecord(label, x, y, z, 'm', 'FLT')
            return pos_exists

    def writeHeader(self,f):
        datestr = datetime.now().strftime("%H:%M:%S %b %d, %Y")
        #if infile_name is a path instead of a file name, pull out file name
        noPathInName = self.infile_name.split("/")
        f.write("Created by SALSA version {0}\n".format(self.SALSA_VERSION_STRING))
        f.write("# Converted with LeicaGNSSToLSA.py converter from {0} on {1}\n".format(noPathInName[-1],datestr))
        f.write("\n")

    def writeOutLSA(self):
        ''' Writes the parent file for the POSC and DXYZ record files
        '''
        try:
            gps_exists = self.parseDXYZ()
            pos_exists = self.parsePOSC()

            #create a list of all UNCRRecords from output_records
            uncrRecordList = filter(lambda x: x.get_lsa_string()[0:5] == "UNCR ", self.output_records)
            with open(self.outfile_name,'w') as f:
                self.writeHeader(f)
                for uncertainty in uncrRecordList:
                    f.write(uncertainty.get_lsa_string() + "\n")
                f.write("\n")
                hghtRecordList = filter(lambda x: x.get_lsa_string()[0:5] == "HGHT ", self.output_records)
                for height in hghtRecordList:
                    f.write(height.get_lsa_string() + "\n")
                f.write("\n")
                #if outfile_name is a path instead of a file name, pull out file name
                noPathInPOS = self.outfile_name.split("/")
                noPathInGPS = self.outfile_name.split("/")
                posOut_name = noPathInPOS[-1].replace(".lsa", ".pos.lsa")
                gpsOut_name = noPathInGPS[-1].replace(".lsa", ".gps.lsa")
                if ' ' in posOut_name:
                    posOut_name = "\"" + posOut_name + "\""
                if ' ' in gpsOut_name:
                    gpsOut_name = "\"" + gpsOut_name + "\""
                if pos_exists:
                    f.write("#--include {0}\n".format(posOut_name))
                if gps_exists:
                    f.write("--include {0}\n".format(gpsOut_name))

        except EnvironmentError:
            print("Error - unable to find/write {0}. Exiting.\n".format(self.outfile_name), file=sys.stderr)
            exit(1)

    def writeOutGPS(self):
        '''Writes out the .gps.lsa file containing the DXYZ records.
        '''
        try:
            gps_exists = self.parseDXYZ()
            if not gps_exists:
                return
            for warning in self.warnings:
                self.write_warning(warning)
            with open(self.outfile_name,'w') as f:
                self.writeHeader(f)
                for record in self.output_records:
                    if record.get_lsa_string()[0:5] != "UNCR " and record.get_lsa_string()[0:5] != "HGHT ":
                        f.write(record.get_lsa_string() + "\n")
        except EnvironmentError:
            print("Error - unable to find/write {0}. Exiting.\n".format(self.outfile_name), file=sys.stderr)
            exit(1)
        except RuntimeError as error:
            print("Error - unexpected value found while parsing.\n{}".format(error), file=sys.stderr)
            exit(1)

    def writeOutPOS(self):
        '''Writes out the .pos.lsa file containing the POSC records.
        '''
        pos_exists = self.parsePOSC()
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

    def isValid(self):
        ''' Expect to find this at the top of the file:
            @%Unit:                m
            @%Coordinate type:     Cartesian
            @%Reference ellipsoid: WGS 1984
        '''
        found_unit = False
        found_coordinate_type = False
        found_ref_ellipsoid = False
        for line in self.input_lines:
            parts = line.strip().split()
            if parts[0] == "@%Unit:":
                found_unit = True
                units = parts[1]
                if units != 'm':
                    return False
            elif parts[0] == "@%Coordinate":
                found_coordinate_type = True
                coordinate_type = parts[2]
                if coordinate_type != 'Cartesian':
                    return False
            elif parts[0] == "@%Reference":
                found_ref_ellipsoid = True
                ref_ellipsoid = parts[2] + " " + parts[3]
                if ref_ellipsoid != 'WGS 1984':
                    return False
            if found_unit and found_coordinate_type and found_ref_ellipsoid:
                return True
        return False

    def getVersion(self,input_lines=None):
        # The Leica GNSS version does not appear to be stored in the .asc file
        return 'Version 1'  # This is what we did for TrimbleToLSA.py

    def writeOutputFile(self):
        if self.outFileType == "LSA":
            self.writeOutLSA()
        elif self.outFileType == "GPS":
            self.writeOutGPS()
        elif self.outFileType == "POS":
            self.writeOutPOS()
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
                print("Error - Not a Cartesian WGS84 meters file.  {0} is not a valid .asc file.  Exiting.\n".format(self.infile_name), file=sys.stderr)
                exit(1)
        except:
            print('Error - unable to determine validity of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)

        try:
            version = self.getVersion()
        except:
            print('Error - unable to determine version of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)

        self.OutputFileType(self.outfile_name)  # the converter is called once for each output file type (include, gps, and pos)
        if not self.validOutputFile():
            print("Error - output file must have either .gps.lsa, .pos.lsa, or .lsa as file extension", file=sys.stderr)
            exit(1)
        self.writeOutputFile()
        if self.warnings_generated:
            print("Warning - warnings generated when parsing {}. Refer to {} for details.".format(os.path.basename(self.infile_name), self.warnfile_name))
