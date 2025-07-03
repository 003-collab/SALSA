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
from lsaconverter import Config

class GSIConverter(LSAConverter):
    uncrLabel = ""
    sigma = 0.0
    bitVersion = 0

    #Implementation of abstract methods
    def writeOutputFile(self):
        datestr = datetime.now().strftime("%H:%M:%S %b %d, %Y")
        try:
            with open(self.outfile_name,'w') as f:
                #write out header
                f.write("Created by SALSA version {0}\n".format(self.SALSA_VERSION_STRING))
                #if infile_name is a path instead of a file name, pull out file name
                noPathInName = self.infile_name.split("/")        
                f.write("# Converted with the {0} {1} on {2}\n\n".format(self.converter_string,noPathInName[-1],datestr))
                
                #write out UNCR records
                #create a list of all UNCRRecords from output_records                    
                uncrRecordList = filter(lambda x: x.get_lsa_string()[0:5] == "UNCR ", self.output_records)
                for uncertainty in uncrRecordList:
                    f.write(uncertainty.get_lsa_string() + "\n")
                f.write("\n")

                #write out HDIF records
                for record in self.output_records:
                    out_string = record.get_lsa_string()
                    if out_string[0:5] != "UNCR ":
                        f.write("{0}\n".format(out_string))
        except EnvironmentError:
            print("Error - unable to find/write {0}. Exiting.\n".format(self.outfile_name), file=sys.stderr)
            exit(1)

    def getBitVersion(self):
        first_block = self.input_lines[0].split(" ")[0]
        len_first_block = len(first_block)
        len_first_block += 1 #add one for the space at the end
        if len_first_block == 16:
            self.bitVersion = 8
        elif len_first_block == 24:
            self.bitVersion = 16
        elif len_first_block ==25 and first_block[0] == '*':#Fix to Bug #1543
            self.stripLeadingAsterisks()
            self.bitVersion = 16
        else:
            raise Exception("bit version indeterminable")

    def stripLeadingAsterisks(self):
        try:
            with open(self.infile_name, 'r') as f:
                asterisk_input_lines = f.readlines()
            self.input_lines = [line[1:] for line in asterisk_input_lines]
            self.num_lines = len(self.input_lines)
        except EnvironmentError:
            print("Error - unable to find/read {0}. Exiting.\n".format(self.infile_name), file=sys.stderr)
            exit(1)

    def getVersion(self):
        offset = self.bitVersion - 8
        for line in self.input_lines:
            if line[0] == "4" and line[1] == "1" and line[7+offset] == "?":
                if line[14+offset] == "1":
                    return "BF"
                elif line[14+offset] == "2":
                    return "BFFB"
                elif line[14+offset] == "3":
                    return "aBF"
                elif line[14+offset] == "4":
                    return "aBFFB"
                elif line[14+offset] == "10":
                    return "Check & Adjust"
                else:
                    raise Exception("no version found")
        raise

    def isValid(self):
        if (".GSI" in self.infile_name):
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
                print("Error - .GSI file not found.  {0} is not a valid LeicaGSI file.  Exiting.\n".format(self.infile_name), file=sys.stderr)
                exit(1)
        except:
            print('Error - unable to determine validity of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        try:
            self.getBitVersion()
        except:
            print('Error - unable to determine if data format of {0} is GSI-8 or GSI-16. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        try:
            version = self.getVersion()
        except:
            print('Error - unable to determine version of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)
        self.converter_string = "LeicaGSI converter from"
        self.uncrLabel = self.addUNCRRecord("LeicaGSI", "HDIF")
        if version == 'BFFB':
            self.parseBFFBOutput()
        elif version == 'BF':
            self.parseBFOutput()
        else:
            #currently "BFFB" and "BF" are the only versions supported by this converter
            print('Error - level line method {0} in file {0} is currenlty unsupported. Exiting.'.format(version,self.infile_name), file=sys.stderr)
            exit(1)       
        self.writeOutputFile()
        if self.warnings_generated:
            print("Warning - warnings generated when parsing {}. Refer to {} for details.".format(os.path.basename(self.infile_name), self.warnfile_name))

    #non-abstract methods
    def getValFromWord(self, word):
        valAsString = 0
        for index in range(7, len(word)):
            symbol = word[index]
            if symbol != "0":
                valAsString = word[index:len(word)]
                break

        val = float(valAsString)

        unit = word[5]
        if unit == "0":
            val = val/1000
        elif unit == "6":
            val = val/10000
        elif unit == "8":
            val = val/100000
        else:
            raise Exception("no unit found for value")

        if "-" in word:
            val = -val

        return val

    def getNameFromWord(self, word):
        for index in range(7, len(word)):
            symbol = word[index]
            if symbol != "0":
                stationName = word[index:len(word)]
                break
        return stationName

    def parseBFFBOutput(self):
        fromStation = "fromPoint"
        toStation = "toPoint"
        #set defualt values
        B1, F1, F2, B2, heightDiff = 0.0, 0.0, 0.0, 0.0, 0.0
        needToApplyCorr = False
        unit = "m"
        #search entire file untill you find the beginning of the stations list        
        lineItr = iter(self.input_lines)        
        for line in lineItr:
            lineItems = line.split(" ")
            firstWord = lineItems[0]            
            if firstWord[0:2] == "41":
                #create a HDIF record from each station pair in list
                currentLine = next(lineItr)
                for currentLine in lineItr:
                    WordList = currentLine.strip().split(" ")
                    if len(WordList) == 5:
                        thirdWord = WordList[2]
                        if thirdWord[0:3] == "331":
                            B1 = self.getValFromWord(thirdWord)
                            fromStation = self.getNameFromWord(WordList[0])
                        if thirdWord[0:3] == "332":
                            F1 = self.getValFromWord(thirdWord)
                            toStation = self.getNameFromWord(WordList[0])
                        if thirdWord[0:3] == "336":
                            F2 = self.getValFromWord(thirdWord)
                        if thirdWord[0:3] == "335":
                            B2 = self.getValFromWord(thirdWord)
                        if thirdWord[4] == "0" or thirdWord[4] == "1":
                            needToApplyCorr = True
                    elif len(WordList) == 6:
                        heightDiff = (B1 + B2) / 2.0 - (F1 + F2)/2.0
                        #no option for instrument determined sigma???
                        self.addHDIFRecord(fromStation, toStation, heightDiff, self.DIFF_LEVELS.sigma, unit, self.uncrLabel, needToApplyCorr) 
                        #reset defualt values
                        B1, F1, F2, B2, heightDiff = 0.0, 0.0, 0.0, 0.0, 0.0
                        needToApplyCorr = False

    def parseBFOutput(self):
        fromStation = "fromPoint"
        toStation = "toPoint"
        #set defualt values
        B1, F1, sigmaB1, sigmaF1, heightDiff = 0.0, 0.0, 0.0, 0.0, 0.0
        needToApplyCorr = False
        unit = "m"
        is_sideshot = False
        #search entire file untill you find the beginning of the stations list        
        lineItr = iter(self.input_lines)        
        for line in lineItr:
            lineItems = line.split(" ")
            firstWord = lineItems[0]            
            if firstWord[0:2] == "41":
                #create a HDIF record from each station pair in list
                self.addComment("--- New line ---")
                currentLine = next(lineItr)
                for currentLine in lineItr:
                    line = currentLine.rstrip()
                    WordList = line.split(" ")
                    if len(WordList) > 0 and len(WordList[0]) >= 2 and WordList[0][0:2] == "41":
                        self.addComment("--- New line ---")
                    if len(WordList) >= 5:
                        thirdWord = WordList[2]
                        fifthWord = WordList[4]
                        if thirdWord[4] == "0" or thirdWord[4] == "1":
                            needToApplyCorr = True
                        else:
                            needToApplyCorr = False
                        if thirdWord[0:3] == "331":
                            if is_sideshot:
                                is_sideshot = False
                                continue
                            B1 = self.getValFromWord(thirdWord)
                            fromStation = self.getNameFromWord(WordList[0])
                            if fifthWord[0:3] == "391":
                                sigmaB1 = self.getValFromWord(fifthWord)
                        if thirdWord[0:3] == "332":
                            F1 = self.getValFromWord(thirdWord)
                            toStation = self.getNameFromWord(WordList[0])
                            if fifthWord[0:3] == "391":
                                sigmaF1 = self.getValFromWord(fifthWord)
                            heightDiff = B1 - F1
                            if self.DIFF_LEVELS.use_inst:
                                sig = math.sqrt(sigmaB1*sigmaB1 + sigmaF1*sigmaF1)
                            else:
                                sig = self.DIFF_LEVELS.sigma
                            self.addHDIFRecord(fromStation, toStation, heightDiff, sig, unit, self.uncrLabel, needToApplyCorr) 							
                        if thirdWord[0:3] == "333":  # side shot
                            is_sideshot = True
                            S = self.getValFromWord(thirdWord)
                            toStation = self.getNameFromWord(WordList[0])
                            if fifthWord[0:3] == "391":
                                sigmaS = self.getValFromWord(fifthWord)
                            heightDiff = B1 - S
                            if self.DIFF_LEVELS.use_inst:
                                sig = math.sqrt(sigmaB1*sigmaB1 + sigmaS*sigmaS)
                            else:
                                sig = self.DIFF_LEVELS.sigma
                            self.addHDIFRecord(fromStation, toStation, heightDiff, sig, unit, self.uncrLabel, needToApplyCorr) 
