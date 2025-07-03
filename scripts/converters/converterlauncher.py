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
#!/usr/bin/env python3

#standard library modules
import argparse as arg
import os, sys
#SALSA modules
from LeicaTotalStation import SetsOfAngles as Sets
from TrimbleToLSA import TrimbleConverter as Trimble
from PPPToLSA import LSAPoint as Point
from GSIToLSA import GSIConverter as GSI
from OPUStoLSA import OpusPoint as Opus
from TrimbleRounds import TrimbleRounds as Rounds
from LeicaGNSSToLSA import LeicaGNSSConverter as Leica

VALID_FILE_TYPES = ['SETS_OF_ANGLES', 'TRIMBLE', 'TRIMBLE_ROUNDS', 'PPP', 'GSI', 'OPUS', 'LEICA_GNSS']#TODO add more file types

def determineFileType(filename):    
    '''Attempt to find type of input file if --type not given as an argument'''
    basename, extension = os.path.splitext(filename)
    if extension.upper() == ".LOG":
        return "SETS_OF_ANGLES"
    elif extension.upper() == ".ASC":
        this_type = "TRIMBLE"
        with open(filename,'rt') as f:
            lines = f.readlines()
            for line in lines:
                if line[0] == "@":
                    this_type = "LEICA_GNSS"
                    break
        return this_type
    elif extension.upper() == ".GSI":
        return "GSI"
    elif extension.upper() == ".OPUS":
        return "OPUS"
    elif extension.upper() == ".JXL":
        return "TRIMBLE_ROUNDS"
    else:#TODO - add more checks and file types
        this_type = None
        with open(args.infile,'r') as f:
            lines = f.readlines()
            for line in lines:
                if ("MERGEDGDP" in line) or ("FINALFKFGDP" in line):
                    this_type = "PPP"
                    break
                elif ("OPUS solution" in line) or ("OPUS-RS solution" in line) or ("OPUS aborting" in line):
                    this_type = "OPUS"
                    break
        return this_type
if __name__ == '__main__':

    #parse input arguments and compose help screen
    usage = """converterlauncher.py [options]
    Invoke the appropriate instrument output to LSA format converter for a specified input and output file
    Example: 'python converterlauncher.py --in C:\data\160120BB.log --out C:\projects\survey1\160120BB.lsa --type SETS_OF_ANGLES --cfgfile C:\projects\survey1\converter.cfg' --wrnfile C:\projects\survey1\160120BB.wrn"""
    parser = arg.ArgumentParser(usage=usage)
    parser.add_argument('--in', action='store', dest='infile', help='Full path to file to be converted.', required=True)
    parser.add_argument('--out', action='store', dest='outfile', help='Full path to resulting LSA file', required=True)
    parser.add_argument('--type', action='store', dest='file_type', help='Type of file to be converted. Valid types are: {0}'.format(str(VALID_FILE_TYPES)))
    parser.add_argument('--cfgfile',action='store', dest='config_file', help='Full path to configuration file',required=True)
    parser.add_argument('--wrnfile', action='store', dest='warning_file', help='Full path to warning file',
                        required=True)
    args = parser.parse_args()

    #check for valid input arguments
    if args.infile == None:
        print('Error - no input file specified.\n', file=sys.stderr)
        parser.print_help()
        exit(1)
    if args.outfile == None:
        print('Error - no output file specified.\n', file=sys.stderr)
        parser.print_help()
        exit(1)
    if not os.path.exists(args.infile):
        print('Error - could not locate file %s. Exiting. ' % args.infile, file=sys.stderr)
        exit(1)
    if args.config_file == None:
        print('Error - no configuration file specified.\n', file=sys.stderr)
        parser.print_help()
        exit(1)
    if not os.path.exists(args.config_file):
        print('Error - could not locate file %s. Exiting. ' % args.config_file, file=sys.stderr)
        exit(1)
    if args.warning_file == None:
        print('Error - no warning file specified.\n', file=sys.stderr)
        parser.print_help()
        exit(1)
    if os.path.exists(args.warning_file): #remove .wrn files from prior conversion attempts
        os.remove(args.warning_file)

    #call the appropriate converter
    if args.file_type == None:
        args.file_type = determineFileType(args.infile)
    if args.file_type == None:
        print("Error - unable to determine file type of {0}.  "
              "Please try again with the --type argument.  Exiting.\n".format(args.infile), file=sys.stderr)
        exit(1)
    elif args.file_type not in VALID_FILE_TYPES:
        print("Error - {0} is not a valid file type.  "
              "The following are currently supported file types: {1}.  Exiting.\n".format(args.file_type,str(VALID_FILE_TYPES)),
              file=sys.stderr)
        exit(1)
    elif args.file_type == "SETS_OF_ANGLES":
        converter = Sets(args.infile,args.outfile,args.config_file,args.warning_file)
        #set values for measurement sigmas and centering errors
        converter.parse_config_file()
        #run the converter
        converter.convert()
    elif args.file_type == "TRIMBLE":    
        #derive gps and pos names here
        gpsoutfile = args.outfile.replace(".lsa", ".gps.lsa")
        posoutfile = args.outfile.replace(".lsa", ".pos.lsa")
        lvloutfile = args.outfile.replace(".lsa", ".lvl.lsa")
        rndoutfile = args.outfile.replace(".lsa", ".rnd.lsa")
        lsaConverter = Trimble(args.infile,args.outfile,args.config_file,args.warning_file)
        lsaConverter.parse_config_file()
        lsaConverter.convert(True)
        gpsConverter = Trimble(args.infile,gpsoutfile,args.config_file,args.warning_file)
        gpsConverter.parse_config_file()
        gpsConverter.convert()
        posConverter = Trimble(args.infile,posoutfile,args.config_file,args.warning_file)
        posConverter.parse_config_file()
        posConverter.convert()
        lvlConverter = Trimble(args.infile,lvloutfile,args.config_file,args.warning_file)
        lvlConverter.parse_config_file()
        lvlConverter.convert()
        rndConverter = Trimble(args.infile,rndoutfile,args.config_file,args.warning_file)
        rndConverter.parse_config_file()
        rndConverter.convert()
    elif args.file_type == "LEICA_GNSS":
        #derive gps and pos names here
        gpsoutfile = args.outfile.replace(".lsa", ".gps.lsa")
        posoutfile = args.outfile.replace(".lsa", ".pos.lsa")
        lsaConverter = Leica(args.infile,args.outfile,args.config_file,args.warning_file)
        lsaConverter.parse_config_file()
        lsaConverter.convert(True)
        gpsConverter = Leica(args.infile,gpsoutfile,args.config_file,args.warning_file)
        gpsConverter.parse_config_file()
        gpsConverter.convert()
        posConverter = Leica(args.infile,posoutfile,args.config_file,args.warning_file)
        posConverter.parse_config_file()
        posConverter.convert()    
    elif args.file_type == "TRIMBLE_ROUNDS":
        converter = Rounds(args.infile,args.outfile,args.config_file,args.warning_file)
        converter.parse_config_file()
        converter.convert()
    elif args.file_type == "PPP":
        converter = Point(args.infile,args.outfile,args.config_file,args.warning_file)
        converter.parse_config_file()
        converter.convert()
    elif args.file_type == "OPUS":
        converter = Opus(args.infile,args.outfile,args.config_file,args.warning_file)
        converter.parse_config_file()
        converter.convert()
    elif args.file_type == "GSI":
        converter = GSI(args.infile,args.outfile,args.config_file,args.warning_file)
        converter.parse_config_file()
        converter.convert()
       
    #output for GUI
    if (args.file_type == "SETS_OF_ANGLES") or (args.file_type == "PPP") or (args.file_type == "GSI") \
            or (args.file_type == "OPUS") or (args.file_type == "TRIMBLE_ROUNDS"):
        print("converterlauncher.py output directed to {0}.".format(args.outfile))
    elif args.file_type == "TRIMBLE":
        print("converterlauncher.py output directed to the following:\n{0}\n{1}\nand {2} or {3}".format
              (args.outfile, posoutfile, gpsoutfile, lvloutfile))
    elif args.file_type == "LEICA_GNSS":
        print("converterlauncher.py output directed to the following:\n{0}\n{1}\nand {2}".format
              (args.outfile, posoutfile, gpsoutfile))

