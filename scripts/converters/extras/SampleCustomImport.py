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
#File: SampleCustomImport.py
#Purpose: Modify point names in Leica GSI file to match project convention, convert to lsa format and import.
#Structure:
#   Parse
#   Fix names
#   Call converterlauncher.py

import argparse as arg,re,os,subprocess,sys
import numpy as np

##############################
###         Parse          ###
##############################

#Parse the input file argument
parser=arg.ArgumentParser(description="Files to be Converted")
parser.add_argument("--in",action='store',dest='inFile',required=True,
    help="The GSI file to be converted.")
parser.add_argument("--out",action='store',dest='outFile',required=True,
    help="The desired output file name.")
parser.add_argument("--cfgfile",action='store',dest='cfgFile',required=True,
    help="The converter configuration file.")
parser.add_argument("--wrnfile",action='store',dest='wrnFile',required=True,
    help="The converter warnings file.")
parser.add_argument("--cvtpy",action='store',dest='cvtpy',required=True,
    help="The converter launcher script.")

args,unknown = parser.parse_known_args()

inFile=args.inFile
outFile=args.outFile
cfgFile=args.cfgFile
wrnFile=args.wrnFile
cvtpy=args.cvtpy

#Validate input and cfg file
if not os.path.isfile(inFile):
    sys.stderr.write("Unable to find the input file.")
    quit()
if not os.path.isfile(cfgFile):
    sys.stderr.write("Unable to find a converter configuration file.")
    quit()

##############################
###       Fix Names        ###
##############################

# Edit point names to comply with project-specific naming convention:
# Replace "CP" or "CP-" (control point) with "BM" (benchmark)
# Points will be renumbered such that [001, 002, 003, 004...] becomes [001A, 001B, 002A, 002B...]
# Example: "CP-123" becomes "BM061B"
# The revised input file will be renamed with a .fixed pre-extension.

tmpFile = re.sub('\.GSI','.fixed.GSI',inFile)
with open(inFile,'r') as inF:
    inFlines = inF.readlines()
    with open(tmpFile,'w') as tmpF:
        search_string=re.compile('.*(BM(\d{3})).*')#BM followed by 3 digits
        for line in inFlines:
            try:
                #look for CP followed by 0 or 1 '-', replace with BM
                new_line = re.sub("CP-?","BM",line)
                #look for BM followed by 3 digits
                m=search_string.match(new_line)
                if m is not None:
                    #convert the first group (the search string portion within parentheses) into a number
                    BMnum=int(m.group(2))
                    if BMnum%2 == 0:
                        newBMnum = "BM{:03}A".format(int(BMnum/2))
                    else:
                        newBMnum = "BM{:03}B".format(int((BMnum-1)/2))
                    #replace the previous benchmark number with half the number, zero-padded, and A or B.
                    new_line = re.sub(m.group(1),newBMnum,new_line)
            except Exception as e:
                if hasattr(e,'message'):
                    print("ERROR - Failure to fix benchmark name in line: {}\n{}".format(line,e.message))
                else:
                    print("ERROR - Failure to fix benchmark name in line: {}\n{}".format(line, e))
            tmpF.write(new_line)

##############################
###  converterlauncher.py  ###
##############################

#After having modified the raw instrument output, call the SALSA GSI converter on it
try:
    cmd = []
    cmd.append(sys.executable)
    cmd.append(cvtpy)
    cmd.append("--in")
    cmd.append(tmpFile)
    cmd.append("--out")
    cmd.append(outFile)
    cmd.append("--type")
    cmd.append("GSI")
    cmd.append("--cfgfile")
    cmd.append(cfgFile)
    cmd.append("--wrnfile")
    cmd.append(wrnFile)
    process = subprocess.Popen(cmd,stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    sys.stderr.write((stderr).decode('utf-8'))
    sys.stdout.write((stdout).decode('utf-8'))
except IOError:
    sys.stderr.write("An error occurred converting the data to LSA format.\n")

#clean up temporary file
try:
    os.remove(tmpFile)
except FileNotFoundError:
    pass
