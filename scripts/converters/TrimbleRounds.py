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

'''
Warning - this converter is still in active development and is meant to be utilized only for the purposes of
testing and troubleshooting the conversion of jxl file data.
'''

#! python3

#standary library imports
from datetime import datetime, timedelta
#python imports
import sys
import os
import re
import math
import numpy as np
import collections
import xml
import xml.etree.ElementTree as ET
#LSA imports
from lsaconverter import LSAConverter
from wrappers.hght import LSAHeight
from lsaconverter import Config


class TrimbleRounds(LSAConverter):

    noEDMList = ('Track', '5mm', '10mm', '20mm', 'Default', 'AveragedTrack', 'Null')
    unitsMap = {'Metres':"m", 'InternationalFeet':"ft", 'USSurveyFeet':"ft"}

    def __init__(self, infile,outfile,configfile,warnfile):
        # Call lsaconverter init method
        super().__init__(infile, outfile, configfile, warnfile)
        self.fieldBookElement = None # Parent element for all rounds related data. Most of the parsed elements
        # have this as an ancestor (parent, grandparent, etc.)
        self.isParsed = False
        self.stationRecords = None # List of all StationRecord elements
        self.stationRecordMap = None # StationRecord ID -> StationRecord element
        self.stationLabelMap = None # StationRecord ID -> corresponding HGHT label

        self.targetRecords = None # List of all TargetRecord elements
        self.targetRecordMap = None # TargetRecord ID -> TargetRecord element
        self.targetLabelMap = None # TargetRecord ID -> corresponding HGHT label

        self.pointRecords = None # List of all PointRecord elements
        self.instrumentRecordMap = None # InstrumentRecord ID -> InstrumentRecord element

        self.organizedRecords = None # 2D list of PointRecord elements. len(self.organizedRecords) should be
        # equivalent to the number of child include elements in the final project.
        self.topoRecords = None
        self.version = None
        self.dateTimeList = None # Time stamps for trimble rounds
        self.endRoundsTimeList = None # Time stamps for EndRoundsRecords
        self.hdirUNCRlabel = None
        self.zangUNCRlabel = None
        self.dist_IR_UNCRlabel = None
        self.dist_RED_UNCRlabel = None
        self.dist_NON_UNCRlabel = None
        self.dist_IR_utilized = False
        self.dist_RED_utilized = False
        self.dist_NON_utilized = False

        self.fromStationMapCounter = {}
        self.rawHeightsMap = None

        self.DIST_UNIT = None
        self.HEIGHT_UNIT = None

    def getVersion(self,input_lines=None):
        pass

    def parseForMeasUnits(self, rootElem):

        distUnitVal = rootElem.findtext('Environment/DisplaySettings/DistanceUnits')
        heightUnitVal = rootElem.findtext('Environment/DisplaySettings/HeightUnits')
        try:
            if distUnitVal is None:
                raise Exception('Distance unit element was not available in {0}'.format(self.infile_name))
            lsaDistUnit = TrimbleRounds.unitsMap.get(distUnitVal)
            if lsaDistUnit is None:
                raise Exception('Distance Units value of {0} not supported for input file {1}.'.format(distUnitVal, self.infile_name))
            self.DIST_UNIT = lsaDistUnit

            if heightUnitVal is None:
                raise Exception('Distance unit element was not available in {0}'.format(self.infile_name))
            lsaHeightUnit = TrimbleRounds.unitsMap.get(heightUnitVal)
            if lsaHeightUnit is None:
                raise Exception('Distance Units value of {0} not supported for input file {1}.'.format(heightUnitVal, self.infile_name))
            self.HEIGHT_UNIT = lsaHeightUnit

        except Exception as E:
            print("Error - {}".format(E.args[0]), file=sys.stderr)
            exit(1)
        except:
            print('Error - an unspecified exception occurred while trying to determine units in Trimble Rounds file', file=sys.stderr)
            exit(1)

    def handleAngularSet(self, measSet):
        """Method utilized to map measurements in a manner that robustly handles angle sets near range boundaries

        :param measSet: population of angles that will be translated. Array of (face1, face2) measurements
        :return: phase - measurement value through which all angles will be translated
        measSet - translated measurement set

        Exception raised in event that all face1 and face2 values are equivalent to None.
        """
        if len(measSet) == 0:
            return (0.0, measSet)

        offsetVal, firstFace1, firstFace2 = None, None, None
        for measValue in measSet:
            if measValue[0] is not None:
                firstFace1 = measValue[0]
                break
            if measValue[1] is not None and firstFace2 is None:
                firstFace2 = measValue[1]

        try:
            if firstFace1 is not None:
                offsetVal = firstFace1
            elif firstFace2 is not None:
                offsetVal = firstFace2
            else:
                raise Exception('Measurement set exists without having a non-null measurement.')

        except Exception as E:
            print("Error - {}".format(E.args[0]), file=sys.stderr)
            exit(1)

        for outerIndex in range(len(measSet)):
            for innerIndex in range(len(measSet[outerIndex])):
                if measSet[outerIndex][innerIndex] is None:
                    continue
                measSet[outerIndex][innerIndex] -= offsetVal
                measSet[outerIndex][innerIndex] = TrimbleRounds.enforceAngleRange(measSet[outerIndex][innerIndex], -180.0, 180.0)

        return (offsetVal, measSet)

    def readInputFile(self):
        """ Method which parses all of the data in the jxl/xml file needed to form lsa record

        Parses the jxl file if it exists and sets all appropriate data members. Should be the only method
        necessary for data extraction from the jxl file.
        :return:

        Will raise an exception if the jxl file does not exist or if the FieldBook element (child of root element)
        does not exist.
        """
        try:
            if not os.path.isfile(self.infile_name):
                raise Exception('File not found {0}'.format(self.infile_name))
            tree = ET.parse(self.infile_name)
            root = tree.getroot()

            self.version = root.get('version')

            self.fieldBookElement = root.find('FieldBook')
            if self.fieldBookElement is None:
                raise Exception('Could not find FieldBook element in {0}'.format(self.infile_name))

            self.parseForMeasUnits(root)

        except Exception as E:
            print("Error - {}".format(E.args[0]), file=sys.stderr)
            exit(1)
        except:
            print('Error - an unspecified exception occurred in parsing Trimble Rounds file.', file=sys.stderr)
            exit(1)

        # Ask if we should throw an exception for any of the elements below being empty and exit
        self.stationRecords = self.fieldBookElement.findall('StationRecord')
        self.stationRecordMap = {}
        for stationRecord in self.stationRecords:
            stationRecID = stationRecord.get('ID')
            self.stationRecordMap[stationRecID] = stationRecord

        self.targetRecords = self.fieldBookElement.findall('TargetRecord')
        self.targetRecordMap = {}
        for targetRecord in self.targetRecords:
            targetRecID = targetRecord.get('ID')
            self.targetRecordMap[targetRecID] = targetRecord

        self.pointRecords = self.fieldBookElement.findall('PointRecord')

        instrumentRecords = self.fieldBookElement.findall('InstrumentRecord')
        self.instrumentRecordMap = {}
        for instrumentRecord in instrumentRecords:
            instrumentID = instrumentRecord.get('ID')
            self.instrumentRecordMap[instrumentID] = instrumentRecord

        self.isParsed = True

    # Very basic for now. self.isParsed will be true if we were able to successfully open a jxl file
    # and grab a FieldBook element.
    def isValid(self):
        if self.isParsed:
            return True
        else:
            return False

    def parseForUNCRrecords(self):
        # Find the first instrument record and grab its type
        # Currently the first parameter in addUNCRRecord is not utilized.
        instrVal = ''
        for instrumentRecord in self.instrumentRecordMap.values():
            type = instrumentRecord.findtext('Type')
            if type is not None:
                instrVal = type
                break

        self.hdirUNCRlabel = self.addUNCRRecord(instrVal, 'HDIR')
        self.zangUNCRlabel = self.addUNCRRecord(instrVal, 'ZANG')
        self.dist_IR_UNCRlabel = self.addUNCRRecord(instrVal, 'DIST_IR')
        self.dist_RED_UNCRlabel = self.addUNCRRecord(instrVal, 'DIST_RED')
        self.dist_NON_UNCRlabel = self.addUNCRRecord(instrVal, 'DIST_NON')

    def getNextHeightLabel(self,station, correctedHeight, rawHeight):
        lastEntryNum = 0
        for key in self.height_dict:#height label is of the form HGHT#<number>_<station_name>
            end = key.find('_')
            begin = key.find('#')
            stored_station = key[end+1:]
            if station == stored_station:#previous entry for this station
                if self.height_dict[key] == (correctedHeight, rawHeight): #duplicate value, don't create a new record
                    return key
                else:#different height value, keep looking for duplicates
                    entryNum = int(key[begin+1:end])
                    if entryNum > lastEntryNum:
                        lastEntryNum = entryNum
        #no duplicates found, add new entry to the dict
        labelPrefix = "HGHT#{0}_".format(lastEntryNum+1)
        heightLabel = labelPrefix + station
        return heightLabel

    def addHGHTRecord(self,heightLabel,height, rawHeight, heightUnit="m"):
        """adds LSAHGHT record to output_records"""
        if heightLabel in self.height_dict:
            return
        else:
            self.height_dict[heightLabel] = (height, rawHeight)
            if self.TOTSTA_INSTR_HT is not None:
                lsaHeight = LSAHeight(heightLabel,height,heightUnit,self.TOTSTA_INSTR_HT.sigma,"m")
            else:
                lsaHeight = LSAHeight(heightLabel,height,heightUnit,0.0,"m")
            self.output_records.append(lsaHeight)

    def parseForHGHTrecords(self):
        """ Method responsible for formation of HGHT records and extraction of raw heights

        Searches StationRecord and TargetRecord elements for height values in raw and corrected
        heights, then creates HGHT records. An HGHT record should not be created if

        1. An HGHT record with the same station label, raw height, and corrected height already exists
        2. The HGHT record has a corrected height equivalent to zero (rounded to the last decimal place)
        :return:
        """
        self.stationLabelMap = {}
        self.targetLabelMap = {}
        self.rawHeightsMap = {}
        for stationRecord in self.stationRecords:

            stationName = stationRecord.findtext('StationName')
            heightVal = float(stationRecord.findtext('TheodoliteHeight'))
            rawHeight = round(float(stationRecord.findtext('RawTheodoliteHeight/MeasuredHeight')), 4)
            measMethod = stationRecord.findtext('RawTheodoliteHeight/MeasurementMethod')
            id = stationRecord.get('ID')

            if stationName is None or heightVal is None:
                continue

            if round(heightVal, 4) >= 0.0001:
                hghtLabel = self.getNextHeightLabel(stationName, heightVal, rawHeight)
                self.addHGHTRecord(hghtLabel, heightVal, rawHeight, self.HEIGHT_UNIT)
                self.stationLabelMap[id] = hghtLabel
                self.rawHeightsMap[hghtLabel] = (rawHeight, measMethod)


        for targetRecord in self.targetRecords:
            targetName = self.findNameForTargetRecord(targetRecord)
            heightVal = float(targetRecord.findtext('TargetHeight'))
            rawHeight = round(float(targetRecord.findtext('RawTargetHeight/MeasuredHeight')), 4)
            measMethod = targetRecord.findtext('RawTargetHeight/MeasurementMethod')
            id = targetRecord.get('ID')

            if targetName is None or heightVal is None:
                continue

            if round(heightVal, 4) >= 0.0001:
                hghtLabel = self.getNextHeightLabel(targetName, heightVal, rawHeight)
                self.addHGHTRecord(hghtLabel, heightVal, rawHeight, self.HEIGHT_UNIT)
                self.targetLabelMap[id] = hghtLabel
                self.rawHeightsMap[hghtLabel] = (rawHeight, measMethod)


    # Grabs name of station for a given TargetRecord element.
    def findNameForTargetRecord(self, targetRecord):
        targetName = None
        id = targetRecord.get('ID')
        for pointRecord in self.pointRecords:
            if pointRecord.findtext('TargetID') == id:
                targetName = pointRecord.findtext('Name')
                break
        return targetName

    # Partitions the original self.pointRecords list into groups that will be used to formulate the individual
    # include child elements.
    def organizeConventionalRecords(self):

        startRoundsList = self.fieldBookElement.findall('StartRoundsRecord')
        endRoundsList = self.fieldBookElement.findall('EndRoundsRecord')

        self.dateTimeList = []
        self.endRoundsTimeList = []
        try:
            if len(self.pointRecords) == 0:
                raise Exception('No PointRecord elements were found in {0}'.format(self.infile_name))

            #Rounds survey
            if len(startRoundsList) > 0 or len(endRoundsList) > 0:
                if len(startRoundsList) != len(endRoundsList):
                    raise Exception('Number of StartRoundsRecords is not equivalent to number of EndRoundsRecords.')
                for index in range(len(startRoundsList)):
                    self.dateTimeList.append(TrimbleRounds.parseForDateTime(startRoundsList[index]))
                    self.endRoundsTimeList.append(TrimbleRounds.parseForDateTime(endRoundsList[index]))

        except Exception as E:
            print("Error - {}".format(E.args[0]), file=sys.stderr)
            exit(1)

        self.topoRecords = []
        self.organizedRecords = [[] for y in range(len(self.dateTimeList))]

        for pointRecord in self.pointRecords:
            convTimeStamp = TrimbleRounds.parseForDateTime(pointRecord)
            if convTimeStamp is None:
                continue
            method = pointRecord.findtext('Method')
            if method != 'DirectReading' and method != 'AngleOnly':
                continue
            if pointRecord.find('Circle') is None:
                continue

            index = TrimbleRounds.grabIndex(self.dateTimeList, self.endRoundsTimeList, convTimeStamp)
            if index is None:
                self.topoRecords.append(pointRecord)
            else:
                self.organizedRecords[index].append(pointRecord)

    # Input a corresponding record for face1, face2, the measurement type and correction.
    # Returns a floating value corresponding to the measurement value.
    def giveMeasurementValue(self, face1Record, face2Record, measType):

        if measType == 'HDIR':
            searchElem = 'Circle/HorizontalCircle'
        elif measType == 'ZANG':
            searchElem = 'Circle/VerticalCircle'
        elif measType == 'DIST':
            searchElem = 'Circle/EDMDistance'
        else:
            return None

        face1Value = face1Record.findtext(searchElem)
        if face2Record is not None:
            face2Value = face2Record.findtext(searchElem)
        else:
            face2Value = ''

        if face1Value != '':
            face1Value = float(face1Value)

        if face2Value != '':
            if measType == 'ZANG':
                face2ValueCast = 360.0 - float(face2Value)
            elif measType == 'HDIR':
                face2ValueCast = float(face2Value) - 180.0
            elif measType == 'DIST':
                face2ValueCast = float(face2Value)

        if face1Value == '' and face2Value == '':
            return (None, None)
        elif face1Value == '':
            return (None, face2ValueCast)
        elif face2Value == '':
            return (face1Value, None)
        else:
            return (face1Value, face2ValueCast)

    def meanFaces(self, anglesList):

        angleValues = []
        for anglePair in anglesList:
            if anglePair[0] is not None and anglePair[1] is not None:
                angleValues.append((anglePair[0] + anglePair[1]) / 2.0)
            elif anglePair[0] is not None:
                angleValues.append(anglePair[0])
            elif anglePair[1] is not None:
                angleValues.append(anglePair[1])

        return angleValues

    def listCombine(self, facesList):
        separatedList = []
        for facePair in facesList:
            for faceAngle in facePair:
                if faceAngle is not None:
                    separatedList.append(faceAngle)

        return separatedList

    # Returns a sigma value for the corresponding measurement. Value is instrument calculated
    # if appropriate Config.use_inst value is set to true. Otherwise returns the sigma value
    # appropriate Config object.
    def giveSigmaValue(self, measType, fromStation, toStation, face1PointRecord, face2PointRecord, values, measMode=None):

        valuesSeparated = self.listCombine(values)

        if measType == 'HDIR':
            if self.TOTSTA_HORZ_DIR.use_inst:
                if len(values) == 1:#topo point
                    sigmaDecDeg = self.getSingleMeasurementSigma(face1PointRecord, face2PointRecord, measType)
                else:
                    sigmaDecDeg = np.std(valuesSeparated)
                if sigmaDecDeg is not None:
                    return 3600.0 * sigmaDecDeg # Convert to SOA
                else:
                    self.write_warning('Warning - a {0} measurement between {1} and {2} has a zero sigma.'.format(measType, fromStation, toStation))
                    return 0.0
            else:
                return self.TOTSTA_HORZ_DIR.sigma

        elif measType == 'ZANG':
            if self.TOTSTA_VERT.use_inst:
                if len(values) == 1:#topo point
                    sigmaDecDeg = self.getSingleMeasurementSigma(face1PointRecord, face2PointRecord, measType)
                else:
                    sigmaDecDeg = np.std(valuesSeparated)
                if sigmaDecDeg is not None:
                    return 3600.0 * sigmaDecDeg # Convert to SOA
                else:
                    self.write_warning('Warning - a {0} measurement between {1} and {2} has a zero sigma.'.format(measType, fromStation, toStation))
                    return 0.0
            else:
                return self.TOTSTA_VERT.sigma

        elif measType == 'DIST':
            if self.TOTSTA_DIST_IR.use_inst:
                if len(values) == 1:#topo point
                    sigmaDist = self.getSingleMeasurementSigma(face1PointRecord, face2PointRecord, measType)
                else:
                    sigmaDist = np.std(valuesSeparated)
                if sigmaDist is not None:
                    return sigmaDist
                else:
                    self.write_warning('Warning - a {0} measurement between {1} and {2} has a zero sigma.'.format(measType, fromStation, toStation))
                    return 0.0
            elif measMode=='Fine':
                return self.TOTSTA_DIST_IR.sigma
            elif measMode=='Coarse':
                return self.TOTSTA_DIST_RED.sigma
            elif measMode in TrimbleRounds.noEDMList:
                return self.TOTSTA_DIST_NON.sigma
            else:
                self.write_warning('Warning - could not find configuration sigma for {0} record between stations {1} and {2}.'.format(measType, fromStation, toStation))
                return 0.0

    # Extract the instrument reported sigma on a single measurement
    def getSingleMeasurementSigma(self, face1PointRecord, face2PointRecord, measType):
        if measType == 'HDIR':
            face1Sigma = face1PointRecord.findtext('Circle/HorizontalCircleStandardError')
            if face2PointRecord is not None:
                face2Sigma = face2PointRecord.findtext('Circle/HorizontalCircleStandardError')
            else:
                face2Sigma = None
        elif measType == 'ZANG':
            face1Sigma = face1PointRecord.findtext('Circle/VerticalCircleStandardError')
            if face2PointRecord is not None:
                face2Sigma = face2PointRecord.findtext('Circle/VerticalCircleStandardError')
            else:
                face2Sigma = None
        elif measType =="DIST":
            face1Sigma = face1PointRecord.findtext('Circle/EDMDistanceStandardError')
            if face2PointRecord is not None:
                face2Sigma = face2PointRecord.findtext('Circle/EDMDistanceStandardError')
            else:
                face2Sigma = None

        if face1Sigma is None:
            return float(face2Sigma)
        elif face2Sigma is None:
            return float(face1Sigma)
        else:
            return np.sqrt(float(face1Sigma)*float(face1Sigma) + float(face2Sigma)*float(face2Sigma))/2.0


    # Grab an HGHT label for a corresponding to station record.
    def grabToHGHTlabel(self, xmlPointRecord):

        targetID = xmlPointRecord.findtext('TargetID')
        if targetID is None:
            return ''
        else:
            return self.targetLabelMap.get(targetID, '')

    def correlateFaceMeasurements(self, face1List, face2List, measType):
        """ Method that pairs together face 1 and face 2 measurements based on their time tag attributes.

        :param face1List: Collection of all face 1 measurements for a given data set
        :param face2List: Collection of all face 2 measurements fora  given data set
        :param measType: Measurement type (valid options are HDIR, ZANG, and DIST)
        :return: List of (face1, face2) tuples. If a face1 element does not have a valid face2 corresponding element, face2 is set
        to None. Likewise, if a face2 element does not have a valid face1 measurement then face1 is set to None
        """

        valuesList = []
        if face2List is None:
            for face1Record in face1List:
                (face1measurement, face2measurement) = self.giveMeasurementValue(face1Record, None, measType)
                if face1measurement is not None or face2measurement is not None:
                    valuesList.append([face1measurement, face2measurement])
            return valuesList


        if len(face1List) <= len(face2List):
            shortLength = len(face1List)
            face1Limiting = True
        else:
            shortLength = len(face2List)
            face1Limiting = False

        faceTimeTags = []

        for index in range(shortLength):
            if face1Limiting:
                currentPointRecord = face1List[index]
                referenceTime = TrimbleRounds.parseForDateTime(currentPointRecord)
                for face2Record in face2List:
                    if TrimbleRounds.parseForDateTime(face2Record) not in faceTimeTags:
                        closestFace2Record = face2Record
                        break
                closestFace2Time = TrimbleRounds.parseForDateTime(closestFace2Record)
                for face2Record in face2List:
                    currentFace2Time = TrimbleRounds.parseForDateTime(face2Record)
                    if abs(currentFace2Time - referenceTime) < abs(closestFace2Time - referenceTime) and currentFace2Time not in faceTimeTags:
                        closestFace2Record = face2Record
                        closestFace2Time = currentFace2Time
                (face1measurement, face2measurement) = self.giveMeasurementValue(currentPointRecord, closestFace2Record, measType)
                if face1measurement is not None or face2measurement is not None:
                    valuesList.append([face1measurement, face2measurement])
                faceTimeTags.append(closestFace2Time)
            else:
                currentPointRecord = face2List[index]
                referenceTime = TrimbleRounds.parseForDateTime(currentPointRecord)
                for face1Record in face1List:
                    if TrimbleRounds.parseForDateTime(face1Record) not in faceTimeTags:
                        closestFace1Record = face1Record
                        break
                closestFace1Time = TrimbleRounds.parseForDateTime(closestFace1Record)
                for face1Record in face1List:
                    currentFace1Time = TrimbleRounds.parseForDateTime(face1Record)
                    if abs(currentFace1Time - referenceTime) < abs(closestFace1Time - referenceTime) and currentFace1Time not in faceTimeTags:
                        closestFace1Record = face1Record
                        closestFace1Time = currentFace1Time
                (face1measurement, face2measurement) = self.giveMeasurementValue(closestFace1Record, currentPointRecord, measType)
                if face1measurement is not None or face2measurement is not None:
                    valuesList.append([face1measurement, face2measurement])
                faceTimeTags.append(closestFace1Time)

        if face1Limiting:
            for face2Record in face2List:
                if TrimbleRounds.parseForDateTime(face2Record) not in faceTimeTags:
                    (face1measurement, face2measurement) = self.giveMeasurementValue(None, face2Record, measType)
                    if face1measurement is not None or face2measurement is not None:
                        valuesList.append([face1measurement, face2measurement])
        else:
            for face1Record in face1List:
                if TrimbleRounds.parseForDateTime(face1Record) not in faceTimeTags:
                    (face1measurement, face2measurement) = self.giveMeasurementValue(face1Record, None, measType)
                    if face1measurement is not None or face2measurement is not None:
                        valuesList.append([face1measurement, face2measurement])

        return valuesList

    # Loop over all measurements of a given measurement type for a particular station and add corresponding LSA records to output_records
    def addMeasurementRecords(self, face1Map, face2Map, nameValue, fromStationName, fromHGHTLabel, currentDGRPlabel, measType):
        for key in face1Map.keys():
            face2ValueList = face2Map.get(key)

            values = self.correlateFaceMeasurements(face1Map[key], face2ValueList, measType)
            if measType == 'DIST':
                measMode = face1Map[key][0].findtext('Circle/EDMMeasurementMode')

            if len(values) > 0:
                face1PointRecord = face1Map[key][-1]
                if face2ValueList is not None:
                    face2PointRecord = face2ValueList[-1]
                else:
                    face2PointRecord = None
                toHGHTLabel = self.grabToHGHTlabel(face1PointRecord)

                if measType == 'HDIR':
                    (phase, values) = self.handleAngularSet(values)
                    measSigma = self.giveSigmaValue(measType, fromStationName, key, face1PointRecord, face2PointRecord, values)
                    meanedFaces = self.meanFaces(values)
                    meanValue =  TrimbleRounds.enforceAngleRange(np.mean(meanedFaces) + phase, 0.0, 360.0)
                    degVal, minVal, secVal = TrimbleRounds.convertToDMS(meanValue)
                    self.addHDIRRecord(key, toHGHTLabel, degVal, minVal, secVal, measSigma, currentDGRPlabel)
                elif measType =='ZANG':
                    (phase, values) = self.handleAngularSet(values)
                    measSigma = self.giveSigmaValue(measType, fromStationName, key, face1PointRecord, face2PointRecord, values)
                    meanedFaces = self.meanFaces(values)
                    meanValue = TrimbleRounds.enforceAngleRange(np.mean(meanedFaces) + phase, 0.0, 360.0)
                    degVal, minVal, secVal = TrimbleRounds.convertToDMS(meanValue)
                    self.addZANGRecord(fromStationName, key, fromHGHTLabel, toHGHTLabel, degVal, minVal, secVal, measSigma, self.zangUNCRlabel)
                elif measType == 'DIST':
                    if measMode == 'Fine':
                        uncrLabel = self.dist_IR_UNCRlabel
                        self.dist_IR_utilized = True
                    elif measMode == 'Coarse':
                        uncrLabel = self.dist_RED_UNCRlabel
                        self.dist_RED_utilized = True
                    elif measMode in TrimbleRounds.noEDMList:
                        uncrLabel = self.dist_NON_UNCRlabel
                        self.dist_NON_utilized = True
                    else:
                        uncrLabel = ''
                    measSigma = self.giveSigmaValue('DIST', fromStationName, key, face1PointRecord, face2PointRecord, values, measMode)
                    meanedFaces = self.meanFaces(values)
                    self.addDISTRecord(fromStationName, key, fromHGHTLabel, toHGHTLabel, np.mean(meanedFaces), measSigma, uncrLabel, self.DIST_UNIT)


    def parseIndividualRecordList(self, recordList, provDateTime, isTopo=False):

        """ Method that is responsible for parsing together conventional measurement records from a provided set
        of PointRecord elements.

        :param recordList: A list of PointRecord elements (will form components of Include file if more than one set).
        All elements in this list will have the same From station.
        :param provDateTime: Starting record time for a set of rounds (or first PointRecord time for topo measurements)
        :param isTopo: Boolean that distinguishes if measurement set is topo (True) or rounds (False)
        :return:
        """

        fileBaseName = os.path.splitext(os.path.basename(self.outfile_name))[0]
        if len(self.topoRecords) > 0:
            topoCont = 1
        else:
            topoCont = 0
        numLists = len(self.organizedRecords) + topoCont

        if (len(recordList)) == 0:
            return

        stationID = None
        instrumentRecord = None

        # Dictionaries used to separate data for face1 and face2.
        # Key is to station label
        face1Map, face2Map = {}, {}

        for pointRecord in recordList:
            id = pointRecord.get('ID')
            tmpStationID = pointRecord.findtext('StationID')
            if stationID is None:
                if tmpStationID is not None:
                    stationID = tmpStationID
                else:
                    continue
            elif stationID != tmpStationID:
                self.write_warning('Warning - mismatch. pointRecord with id {0} has differing StationID value {1} than expected {2}'.format(id, tmpStationID, stationID))
                continue

            nameValue = pointRecord.findtext('Name')
            faceValue = pointRecord.findtext('Circle/Face')
            if faceValue == 'Face1':
                if face1Map.get(nameValue) is None:
                    face1Map[nameValue] = [pointRecord]
                else:
                    face1Map[nameValue].append(pointRecord)
            elif faceValue == 'Face2':
                if face2Map.get(nameValue) is None:
                    face2Map[nameValue] = [pointRecord]
                else:
                    face2Map[nameValue].append(pointRecord)

        # Grab from station name and instrument record information associated with stationID
        fromStationRecord = self.stationRecordMap.get(stationID)
        if fromStationRecord is None:
            fromStationName = 'Unknown'
        else:
            fromStationName = fromStationRecord.findtext('StationName', 'Unknown')
            instrumentRecordID = fromStationRecord.findtext('InstrumentID')
            if instrumentRecordID is not None:
                instrumentRecord = self.instrumentRecordMap.get(instrumentRecordID)

        # Formulate include records if we partitioned our PointRecord elements into more than one
        # list.
        if numLists > 1 and not isTopo:
            if self.fromStationMapCounter.get(fromStationName) is None:
                self.fromStationMapCounter[fromStationName] = 1
            else:
                self.fromStationMapCounter[fromStationName] += 1

            counterVal = self.fromStationMapCounter.get(fromStationName)
            if counterVal == 1:
                fileName = fileBaseName + '_' + fromStationName + '.lsa'
            else:
                fileName = fileBaseName + '_' + fromStationName + '.' + str(counterVal) + '.lsa'

            self.addInclude(fileName)

        elif numLists > 1 and isTopo:
            fileName = fileBaseName + '_' + 'TOPO.lsa'
            self.addInclude(fileName)

        instrumentTypeStr = 'Instrument Type\t\t\t: '
        instrumentSerialStr = 'Instrument Serial No.\t\t: '
        instrumentFirmwareStr = 'Instrument Firmware Ver.\t: '
        if instrumentRecord is not None:
            instrumentTypeStr += instrumentRecord.findtext('Type', '')
            instrumentSerialStr += instrumentRecord.findtext('Serial', '')
            instrumentFirmwareStr += instrumentRecord.findtext('FirmwareVersion', '')

        self.addComment(instrumentTypeStr)
        self.addComment(instrumentSerialStr)
        self.addComment(instrumentFirmwareStr)
        self.addComment('Rounds Collection Start\t\t: ' + provDateTime.strftime('%Y/%m/%d %H:%M:%S'))
        self.addComment('')

        currentDGRPlabel = self.getNextDGRPLabel()
        self.addDGRPRecord(currentDGRPlabel, fromStationName, self.hdirUNCRlabel)
        fromHGHTLabel = self.stationLabelMap.get(stationID, '')

        # Go through the dictionaries and add records.
        # Add HDIR records first, then ZANG, then DIST
        self.addMeasurementRecords(face1Map, face2Map, nameValue, fromStationName, fromHGHTLabel, currentDGRPlabel,
                                   'HDIR')
        self.addComment('')
        self.addMeasurementRecords(face1Map, face2Map, nameValue, fromStationName, fromHGHTLabel, currentDGRPlabel,
                                   'ZANG')
        self.addComment('')
        self.addMeasurementRecords(face1Map, face2Map, nameValue, fromStationName, fromHGHTLabel, currentDGRPlabel,
                                   'DIST')


    # Goes thorugh all of the records in self.organized records and adds include and conventional records
    # via the various lsaconverter add****Record methods.
    def parseForConventionalRecords(self):

        self.fromStationMapCounter = {}

        if len(self.topoRecords) > 0:
            topoDateTime = TrimbleRounds.parseForDateTime(self.topoRecords[0])
            self.parseIndividualRecordList(self.topoRecords, topoDateTime, True)

        for listIndex in range(len(self.organizedRecords)):
            self.parseIndividualRecordList(self.organizedRecords[listIndex], self.dateTimeList[listIndex])


    # When called assumes that all necessary data has been added to self.output_records
    def writeOutputFile(self):
        heights = []
        includes = []
        uncertainties = []
        unused_uncertainties = []

        datestr = datetime.now().strftime("%H:%M:%S %b %d, %Y")

        try:
            # Write out all of the appropriate information in the parent include
            with open(self.outfile_name, 'w') as outFile:
                outFile.write("Created by SALSA version {0}\n".format(self.SALSA_VERSION_STRING))
                outFile.write("# Converted with the {0} converter from {1} on {2}\n".format(self.converter_string, self.infile_name, datestr))

                for record in self.output_records:
                    out_string = record.get_lsa_string()
                    if out_string[:10] == "--include ":
                        includes.append(out_string)
                    elif out_string[0:5] == "HGHT ":
                        heights.append(out_string)
                    elif out_string[0:5] == "UNCR ":
                        if "DIST" in out_string:
                            if "IR" in out_string and self.dist_IR_utilized:
                                uncertainties.append(out_string)
                            elif "RED" in out_string and self.dist_RED_utilized:
                                uncertainties.append(out_string)
                            elif "NOEDM" in out_string and self.dist_NON_utilized:
                                uncertainties.append(out_string)
                            else:
                                unused_uncertainties.append(out_string)
                        else:
                            uncertainties.append(out_string)

                outFile.write("\n")
                for uncertainty in uncertainties:
                    outFile.write("{0}\n".format(uncertainty))
                outFile.write("\n")

                for height in heights:
                    outFile.write("{0}\n".format(height))
                    hghtLabel = height.split(' ')[1]
                    rawHeightInfo = self.rawHeightsMap.get(hghtLabel, ('Unknown', 'Unknown'))
                    outFile.write("# Raw height {0} {1} extracted from {2} method\n".format(rawHeightInfo[0], self.HEIGHT_UNIT, rawHeightInfo[1]))
                outFile.write("\n")

                if len(includes) > 1:
                    for include in includes:
                        outFile.write("{0}\n".format(include))
                else:
                    for record in self.output_records:
                        out_string = record.get_lsa_string()
                        if (out_string not in uncertainties and
                            out_string not in heights and
                            out_string not in includes and
                            out_string not in unused_uncertainties):
                            outFile.write("{0}\n".format(out_string))

        except EnvironmentError:
            print("Error - unable to find/write {0}. Exiting.\n".format(self.outfile_name), file=sys.stderr)
            exit(1)

        try:
            # Write all child include records as necessary
            if len(includes) > 1:
                recordsList = [[] for y in range(len(includes))]
                includeCounter = -1

                for record in self.output_records:

                    out_string = record.get_lsa_string()
                    if out_string[:10] == "--include ":
                        includeCounter += 1
                    elif includeCounter != -1:
                        recordsList[includeCounter].append(out_string)

                for includeIndex in range(len(includes)):
                    include_file_name = includes[includeIndex][10:]
                    if include_file_name.startswith("\"") and include_file_name.endswith("\""):
                        include_file_name = include_file_name[1:-1]
                    include_file_name = os.path.join(os.path.dirname(self.outfile_name), include_file_name)
                    with open(include_file_name, 'w') as outFile:
                        outFile.write("Created by SALSA version {0}\n".format(self.SALSA_VERSION_STRING))
                        outFile.write("# Converted with the {0} converter from {1} on {2}\n".format(self.converter_string,
                                                                                          self.infile_name, datestr))
                        for lsaString in recordsList[includeIndex]:
                            outFile.write("{0}\n".format(lsaString))
        except EnvironmentError:
            print("Error - unable to find/write a child include record. Exiting.\n", file=sys.stderr)
            exit(1)


    # Meta method that does everything necessary to parse and write out an output file (except for parsing
    # configuration file).
    def convert(self):
        try:
            self.readInputFile()
        except:
            print('Error - unable to read {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)

        try:
            if not self.isValid():
                raise Exception
                exit(1)
        except Exception:
            print('Error - unable to determine validity of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)

        try:
            if self.version is None:
                raise Exception
        except Exception:
            print('Error - unable to determine version of {0}. Exiting.'.format(self.infile_name), file=sys.stderr)
            exit(1)

        self.converter_string = "Trimble Rounds Converter"
        self.parseForUNCRrecords()
        self.parseForHGHTrecords()
        try:
            self.organizeConventionalRecords()
            self.parseForConventionalRecords()
        except Exception as e:
            if hasattr(e, 'message'):
                print('Error in forming conventional records:\n{}\nExiting.'.format(e.message), file=sys.stderr)
            else:
                print('Error in forming conventional records:\n{}\nExiting.'.format(e), file=sys.stderr)
            exit(1)

        self.writeOutputFile()
        if self.warnings_generated:
            print("Warning - warnings generated when parsing {}. Refer to {} for details.".format(os.path.basename(self.infile_name), self.warnfile_name))


    @staticmethod
    def enforceAngleRange(value, lowerBound, upperBound):
        while value < lowerBound:
            value += 360.0
        while value >= upperBound:
            value -= 360.0

        return value

    # Takes a time stamp from a given jxl / xml element and parses it into a datetime object.
    @staticmethod
    def parseForDateTime(xmlElement):
        dateTimeReg = re.compile(r'(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})')
        timeStampStr = xmlElement.get('TimeStamp')
        if timeStampStr is None:
            return None
        dateTimeRes = dateTimeReg.search(timeStampStr)
        if dateTimeRes is None:
            return None
        return datetime(int(dateTimeRes.group(1)), int(dateTimeRes.group(2)), int(dateTimeRes.group(3)),
                                 int(dateTimeRes.group(4)), int(dateTimeRes.group(5)), int(dateTimeRes.group(6)))

    # Returns an index indicating where an element should be dropped. Assumes first parameter is sorted.
    @staticmethod
    def grabIndex(beginTimeList, endTimeList, value):
        finalIndex = None
        for listIndex in range(len(beginTimeList)):
            if value >= beginTimeList[listIndex] and value < endTimeList[listIndex]:
                finalIndex = listIndex
                break
        return finalIndex

    # Converts a decimal degree parameter into DMS. All types returned are numerical.
    @staticmethod
    def convertToDMS(degreeVal):
        degInt = int(degreeVal)

        decimalMinute = degreeVal % 1
        minuteVal = decimalMinute * 60.0
        minInt = int(minuteVal)

        decimalSeconds = minuteVal % 1
        secondsVal = decimalSeconds * 60.0
        secondsRounded = round(secondsVal, 1)

        if secondsRounded >= 60.0:
            secondsRounded = round(secondsRounded - 60.0, 1)
            minInt += 1

        if minInt >= 60:
            minInt -= 60
            degInt += 1

        if degInt >= 360:
            degInt -= 360
        elif degInt <= -360:
            degInt += 360

        return degInt, minInt, secondsRounded
