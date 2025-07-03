/*
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
*/
/// @file lsah5.cpp
/// Implement HDF5 output file for lsapost, including input and output.

#include "lsah5.hpp"
#include "Point.hpp"
#include "lsaUtils.hpp"
#include "Position.hpp"
#include "logstream.hpp"
#include "QFileInfo"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;
using namespace H5;

// 1.0.2: Added number of measurements column to Problem Description
const std::string LSAH5File::HDF5_FILE_VERSION = std::string("1.1.0");

void ProjCfgDataSet::addDataPoint(double converge, int niterMax, std::string geoInt, std::string geoidFile, std::string confidence, double alpha, bool calcExtRelVect)
{
    dataContainer.converge = converge;
    dataContainer.iterMax = niterMax;
    dataContainer.geoint = geoInt;
    dataContainer.geoFileName = geoidFile;
    dataContainer.confidence = confidence;
    dataContainer.alpha = alpha;
    if(calcExtRelVect==true)
    {
      dataContainer.calcExtRelVect = enum_bool_type::BOOL_TRUE;
    }
    else
    {
       dataContainer.calcExtRelVect = enum_bool_type::BOOL_FALSE;
    }
}

void ProjCfgDataSet::addDataPoint(projConfigExternal dataPoint)
{
    dataContainer = dataPoint;
}

int ProjCfgDataSet::writeToFile(H5::H5File writeFile)
{
    //convert external struct to internal struct before writing
    projConfigInternal internalStruct = ProjConfigExternalToInternal();

    //write iternalStruct to .h5 file
    const int numOfRowsInTable = 1;
    const int numOfStructsPerRow = 1;
    projConfigInternal projCfgBuffer[numOfRowsInTable];
    projCfgBuffer[0] = internalStruct;
    hsize_t numOfStructsInRow[] = {numOfStructsPerRow};
    H5::DataSpace projCfgSpace = H5::DataSpace( numOfRowsInTable, numOfStructsInRow );
    H5::DataSet dataset = H5::DataSet(writeFile.createDataSet("Project Configuration", projCfgFileType, projCfgSpace));
    dataset.write( projCfgBuffer, projCfgMemoryType );

    //manually close all projCfg related hdf5 objects
    projCfgSpace.close();
    dataset.close();
    return 0;
}

int ProjCfgDataSet::readDataSetFromFile(H5::H5File readFile)
{
    //open dataset
    H5::DataSet dataset = H5::DataSet(readFile.openDataSet("Project Configuration"));
    //create read in buffer
    projConfigInternal readBuffer[1];
    //read file into Buffer
    dataset.read(readBuffer, projCfgMemoryType);
    //fill dataContainer with buffer contents
    readToDataContainer(readBuffer[0]);
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
projConfigExternal ProjCfgDataSet::getDataPoint()
{
    return dataContainer;
}

projConfigInternal ProjCfgDataSet::ProjConfigExternalToInternal()
{
    //convert external struct to internal(H5) struct
    projConfigInternal internalStruct;
    internalStruct.alpha = dataContainer.alpha;
    internalStruct.converge = dataContainer.converge;
    internalStruct.geoint = dataContainer.geoint.c_str();
    internalStruct.geoFileName = dataContainer.geoFileName.c_str();
    internalStruct.confidence = dataContainer.confidence.c_str();
    internalStruct.iterMax = dataContainer.iterMax;
    internalStruct.calcExtRelVect = dataContainer.calcExtRelVect;
    return internalStruct;
}

void ProjCfgDataSet::readToDataContainer(projConfigInternal dataPoint)
{
    //convert internal to external
    dataContainer.alpha = dataPoint.alpha;
    dataContainer.iterMax = dataPoint.iterMax;
    dataContainer.geoint = dataPoint.geoint;
    dataContainer.geoFileName = dataPoint.geoFileName;
    dataContainer.confidence = dataPoint.confidence;
    dataContainer.converge = dataPoint.converge;
    dataContainer.calcExtRelVect = dataPoint.calcExtRelVect;
}

//init the compound datatype for xyz dataset
void ProjCfgDataSet::InitprojCfgDataType(){

   //this will be repeated a lot, put this code somewhere all classes can reach
   enum_bool_type enumBool;
   LSAFixedState enumFixed;
   enum_source_type enumSource;

   hid_t strType = H5Tcopy(H5T_C_S1);
   H5Tset_size(strType, H5T_VARIABLE);

   hid_t ENUMBOOLMEMORY = H5Tenum_create(H5T_NATIVE_INT);
   H5Tenum_insert(ENUMBOOLMEMORY,            "FALSE",                    CPTR(enumBool, BOOL_FALSE));
   H5Tenum_insert(ENUMBOOLMEMORY,            "TRUE",                     CPTR(enumBool, BOOL_TRUE));

   hid_t ENUMFIXEDMEMORY = H5Tenum_create(H5T_NATIVE_INT);
   H5Tenum_insert(ENUMFIXEDMEMORY,           "FLOAT",                    CPTR(enumFixed, LSAFixedState::FLOATING));
   H5Tenum_insert(ENUMFIXEDMEMORY,           "FIXED",                    CPTR(enumFixed, LSAFixedState::FIXED));
   H5Tenum_insert(ENUMFIXEDMEMORY,           "CONSTRAINED",              CPTR(enumFixed, LSAFixedState::CONSTRAINED));

   hid_t ENUMSOURCEMEMORY = H5Tenum_create(H5T_NATIVE_INT);
   H5Tenum_insert(ENUMSOURCEMEMORY,          "PROVIDED",                 CPTR(enumSource, PROVIDED));
   H5Tenum_insert(ENUMSOURCEMEMORY,          "DERIVED_MEAN",             CPTR(enumSource, DERIVED_MEAN));
   H5Tenum_insert(ENUMSOURCEMEMORY,          "DERIVED_ENUO",             CPTR(enumSource, DERIVED_ENUO));
   H5Tenum_insert(ENUMSOURCEMEMORY,          "AUTOGENERATED",            CPTR(enumSource, AUTOGENERATED));
   //end of repeated code

    //create compound data type for memory
    projCfgMemoryType = H5Tcreate(H5T_COMPOUND, sizeof(projConfigInternal));
    H5Tinsert(projCfgMemoryType, "Convergence Criterion",               HOFFSET(projConfigInternal, converge),       H5T_NATIVE_DOUBLE);
    H5Tinsert(projCfgMemoryType, "Iteration Max",                       HOFFSET(projConfigInternal, iterMax),        H5T_NATIVE_INT);
    H5Tinsert(projCfgMemoryType, "Geoid Interpolator",                  HOFFSET(projConfigInternal, geoint),         strType);
    H5Tinsert(projCfgMemoryType, "Geoid File Name",                     HOFFSET(projConfigInternal, geoFileName),    strType);
    H5Tinsert(projCfgMemoryType, "Confidence Region",                   HOFFSET(projConfigInternal, confidence),     strType);
    H5Tinsert(projCfgMemoryType, "Chi Squared Significance",            HOFFSET(projConfigInternal, alpha),          H5T_NATIVE_DOUBLE);
    H5Tinsert(projCfgMemoryType, "External Reliability Calculation",    HOFFSET(projConfigInternal, calcExtRelVect), ENUMBOOLMEMORY);


    projCfgFileType = H5Tcopy(projCfgMemoryType);
}

void PostProcInputDataSet::addDataPoint(std::string binFileName, int binVersion, std::string datFileName, std::string outFileName, std::string hashString, enum_binary_file_status binFileStatus)
{
    dataContainer.binFileName = binFileName;
    dataContainer.binVersion = binVersion;
    dataContainer.datFileName = datFileName;
    dataContainer.outFileName = outFileName;
    dataContainer.hashString = hashString;
    dataContainer.binFileStatus = binFileStatus;
}

void PostProcInputDataSet::addDataPoint(postProcInputExternal dataPoint)
{
    dataContainer = dataPoint;
}

int PostProcInputDataSet::writeToFile(H5::H5File writeFile)
{
    //convert external struct to internal struct before writing
    postProcInputInternal internalStruct = postProcInputExternalToInternal();

    //write iternalStruct to .h5 file
    const int numOfRowsInTable = 1;
    const int numOfStructsPerRow = 1;
    postProcInputInternal projCfgBuffer[numOfRowsInTable];
    projCfgBuffer[0] = internalStruct;
    hsize_t numOfStructsInRow[] = {numOfStructsPerRow};
    H5::DataSpace projCfgSpace = H5::DataSpace( numOfRowsInTable, numOfStructsInRow );
    H5::DataSet dataset = H5::DataSet(writeFile.createDataSet("Post Processor Input", postProcInputFileType, projCfgSpace));
    dataset.write( projCfgBuffer,postProcInputMemoryType );

    //manually close all projCfg related hdf5 objects
    projCfgSpace.close();
    dataset.close();
    return 0;
}

int PostProcInputDataSet::readDataSetFromFile(H5::H5File readFile)
{
    //open dataset
    H5::DataSet dataset = H5::DataSet(readFile.openDataSet("Post Processor Input"));
    //create read in buffer
    postProcInputInternal readBuffer[1];
    //read file into Buffer
    dataset.read(readBuffer, postProcInputMemoryType);
    //fill dataContainer with buffer contents
    readToDataContainer(readBuffer[0]);
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
postProcInputExternal PostProcInputDataSet::getDataPoint()
{
    return dataContainer;
}

postProcInputInternal PostProcInputDataSet::postProcInputExternalToInternal()
{
    //convert external struct to internal(H5) struct
    postProcInputInternal internalStruct;
    internalStruct.binFileName = dataContainer.binFileName.c_str();
    internalStruct.binFileStatus = dataContainer.binFileStatus;
    internalStruct.binVersion = dataContainer.binVersion;
    internalStruct.datFileName = dataContainer.datFileName.c_str();
    internalStruct.hashString = dataContainer.hashString.c_str();
    internalStruct.outFileName = dataContainer.outFileName.c_str();
    return internalStruct;
}

void PostProcInputDataSet::readToDataContainer(postProcInputInternal dataPoint)
{
    //convert internal to external
    dataContainer.binFileName = dataPoint.binFileName;
    dataContainer.binVersion = dataPoint.binVersion;
    dataContainer.datFileName = dataPoint.datFileName;
    dataContainer.hashString = dataPoint.hashString;
    dataContainer.outFileName = dataPoint.outFileName;
    dataContainer.binFileStatus = dataPoint.binFileStatus;
}

//init the compound datatype for xyz dataset
void PostProcInputDataSet::InitpostProcInputDataType(){

    //this will be repeated a lot, put this code somewhere all classes can reach
    enum_binary_file_status enumBinFileStat;
    hid_t ENUMBINFILESTATUSMEMORY = H5Tenum_create(H5T_NATIVE_INT);
    H5Tenum_insert(ENUMBINFILESTATUSMEMORY,   "BIN_FILE_PARSING_FAILURE", CPTR(enumBinFileStat, BIN_FILE_PARSING_FAILURE));
    H5Tenum_insert(ENUMBINFILESTATUSMEMORY,   "BIN_FILE_STALE",           CPTR(enumBinFileStat, BIN_FILE_STALE));
    H5Tenum_insert(ENUMBINFILESTATUSMEMORY,   "BIN_FILE_FORMAT_OBSOLETE", CPTR(enumBinFileStat, BIN_FILE_FORMAT_OBSOLETE));
    H5Tenum_insert(ENUMBINFILESTATUSMEMORY,   "BIN_FILE_UNABLE_TO_OPEN",  CPTR(enumBinFileStat, BIN_FILE_UNABLE_TO_OPEN));
    H5Tenum_insert(ENUMBINFILESTATUSMEMORY,   "BIN_FILE_NOT_PARSED",      CPTR(enumBinFileStat, BIN_FILE_NOT_PARSED));
    H5Tenum_insert(ENUMBINFILESTATUSMEMORY,   "BIN_FILE_NOT_FOUND",       CPTR(enumBinFileStat, BIN_FILE_NOT_FOUND));
    H5Tenum_insert(ENUMBINFILESTATUSMEMORY,   "BIN_FILE_NO_SOLUTION",     CPTR(enumBinFileStat, BIN_FILE_NO_SOLUTION));
    H5Tenum_insert(ENUMBINFILESTATUSMEMORY,   "BIN_FILE_FOUND_SOLUTION",  CPTR(enumBinFileStat, BIN_FILE_FOUND_SOLUTION));
    H5Tenum_insert(ENUMBINFILESTATUSMEMORY,   "BIN_FILE_VALID",           CPTR(enumBinFileStat, BIN_FILE_VALID));

    hid_t strType = H5Tcopy(H5T_C_S1);
    H5Tset_size(strType, H5T_VARIABLE);
    //end repeated code

    //create compound data type for memory
    postProcInputMemoryType = H5Tcreate(H5T_COMPOUND, sizeof(postProcInputInternal));
    H5Tinsert(postProcInputMemoryType, "Bin File Name",         HOFFSET(postProcInputInternal, binFileName),    strType);
    H5Tinsert(postProcInputMemoryType, "Bin File Version",      HOFFSET(postProcInputInternal, binVersion),     H5T_NATIVE_INT);
    H5Tinsert(postProcInputMemoryType, "Dat File Name",         HOFFSET(postProcInputInternal, datFileName),    strType);
    H5Tinsert(postProcInputMemoryType, "Out File Name",         HOFFSET(postProcInputInternal, outFileName),    strType);
    H5Tinsert(postProcInputMemoryType, "Hash String",           HOFFSET(postProcInputInternal, hashString),     strType);
    H5Tinsert(postProcInputMemoryType, "Binary File Status",    HOFFSET(postProcInputInternal, binFileStatus),  ENUMBINFILESTATUSMEMORY);

    postProcInputFileType = H5Tcopy(postProcInputMemoryType);
}

void PostProcInputDataSet::initDataContainer(){
    dataContainer.binFileName = "";
    dataContainer.binFileStatus = BIN_FILE_NO_SOLUTION;//what should the empty value for this variable be?
    dataContainer.binVersion = 0;
    dataContainer.datFileName = "";
    dataContainer.hashString = "";
    dataContainer.outFileName = "";
}

void ProbDescriptDataSet::addDataPoint(int degOfFreed, int unknownCount, int dataCount, int NMeas, int constraintCount, int dims)
{
    dataContainer.degOfFreed = degOfFreed;
    dataContainer.unknownCount = unknownCount;
    dataContainer.dataCount = dataCount;
    dataContainer.NMeas = NMeas;
    dataContainer.constraintCount = constraintCount;
    dataContainer.dims = dims;
    size++;
}

void ProbDescriptDataSet::addDataPoint(probDescriptExternal dataPoint)
{
    dataContainer = dataPoint;
    size++;
}

int ProbDescriptDataSet::writeToGroup(H5::Group adjustmentResultsGroup)
{
    const int numOfRowsInTable = 1;
    const int numOfStructsPerRow = 1;
    probDescriptExternal probDescriptBuffer[numOfRowsInTable];
    probDescriptBuffer[0] = dataContainer;
    hsize_t numOfStructsInRow[] = {numOfStructsPerRow};
    H5::DataSpace probDescriptSpace = H5::DataSpace( numOfRowsInTable, numOfStructsInRow );
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.createDataSet("Problem Description", probDescriptFileType, probDescriptSpace));
    dataset.write( probDescriptBuffer, probDescriptMemoryType );
    probDescriptSpace.close();
    dataset.close();
    return 0;
}

int ProbDescriptDataSet::readDataSetFromGroup(H5::Group adjustmentResultsGroup)
{
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.openDataSet("Problem Description"));
    probDescriptExternal readBuffer[1];
    //read file
    // reference to probDescriptContainer
    dataset.read( readBuffer, probDescriptMemoryType);
    dataContainer = readBuffer[0];
    if(dataContainer.constraintCount != 0 ||
       dataContainer.dataCount != 0 ||
       dataContainer.NMeas != 0 ||
       dataContainer.degOfFreed != 0 ||
       dataContainer.dims != 0 ||
       dataContainer.unknownCount != 0)
        size = 1;
    dataset.close();
    return 0;
}

int ProbDescriptDataSet::getSize(){
    return size;
}

//readDataSetFromGroup must be called before this function to work
probDescriptExternal ProbDescriptDataSet::getDataPoint()
{
    return dataContainer;
}

void ProbDescriptDataSet::InitpostProcInputDataType(){
    //create compound data type for memory
    probDescriptMemoryType = H5Tcreate(H5T_COMPOUND, sizeof(probDescriptExternal));
    H5Tinsert(probDescriptMemoryType, "Degrees Of Freedom",     HOFFSET(probDescriptExternal, degOfFreed),      H5T_NATIVE_INT);
    H5Tinsert(probDescriptMemoryType, "Number Of Unknowns",     HOFFSET(probDescriptExternal, unknownCount),    H5T_NATIVE_INT);
    H5Tinsert(probDescriptMemoryType, "Number Of Data",         HOFFSET(probDescriptExternal, dataCount),       H5T_NATIVE_INT);
    H5Tinsert(probDescriptMemoryType, "Number Of Measurements", HOFFSET(probDescriptExternal, NMeas),           H5T_NATIVE_INT);
    H5Tinsert(probDescriptMemoryType, "Number Of Constraints",  HOFFSET(probDescriptExternal, constraintCount), H5T_NATIVE_INT);
    H5Tinsert(probDescriptMemoryType, "Dimensions",             HOFFSET(probDescriptExternal, dims),            H5T_NATIVE_INT);

    probDescriptFileType = H5Tcopy(probDescriptMemoryType);
}

void ProbDescriptDataSet::initContainer(){
    dataContainer.constraintCount = 0;
    dataContainer.dataCount = 0;
    dataContainer.NMeas = 0;
    dataContainer.degOfFreed = 0;
    dataContainer.dims = 0;
    dataContainer.unknownCount = 0;
    size = 0;
}

void LlhDataSet::setGroup(H5::Group assignedGroup)
{
    group = assignedGroup;
}

int LlhDataSet::getSize()
{
    return dataContainer.size();
}

void LlhDataSet::addDataPoint(std::string label, double lat, double lon, double ht, enum_source_type sourceType, bool is_adjusted,
                              LSAFixedState fixedType, std::string constraint, double oht, double dovN,double dovE, double adjN, double adjE, double adjU, double sigN,
                              double sigE, double sigU, double Cnn, double Cne, double Cnu, double Cee, double Ceu, double Cuu,
                              double maj3D, double maj2D, double min2D, double azim, double vert, double maj2Drr, double min2Drr, double maj3Drr,
                              double vertrr, double azrr)
{
    llhExternal dataStruct;
    dataStruct.label = label.c_str();
    dataStruct.lat = lat;
    dataStruct.lon = lon;
    dataStruct.ht = ht;
    dataStruct.sourceType = sourceType;
    dataStruct.utilized = is_adjusted;
    dataStruct.fixedType = fixedType;
    dataStruct.constraints = constraint.c_str();
    dataStruct.oht = oht;
    dataStruct.dovN = dovN;
    dataStruct.dovE = dovE;
    dataStruct.adjN = adjN;
    dataStruct.adjE = adjE;
    dataStruct.adjU = adjU;
    dataStruct.sigN = sigN;
    dataStruct.sigE = sigE;
    dataStruct.sigU = sigU;
    dataStruct.Cnn = Cnn;
    dataStruct.Cne = Cne;
    dataStruct.Cnu = Cnu;
    dataStruct.Cee = Cee;
    dataStruct.Ceu = Ceu;
    dataStruct.Cuu = Cuu;
    dataStruct.maj2D = maj2D;
    dataStruct.min2D = min2D;
    dataStruct.maj3D = maj3D;
    dataStruct.azim = azim;
    dataStruct.vert = vert;
    dataStruct.maj2Drr = maj2Drr;
    dataStruct.min2Drr = min2Drr;
    dataStruct.maj3Drr = maj3Drr;
    dataStruct.vertrr = vertrr;
    dataStruct.azrr = azrr;
    dataContainer.push_back(dataStruct);
}

void LlhDataSet::addDataPoint(llhExternal dataPoint)
{
    dataContainer.push_back(dataPoint);
}

int LlhDataSet::writeToGroup(H5::Group assignedGroup)
{
    const int numOfDataPointsInSet = dataContainer.size();
    llhInternal *h5Buffer = new llhInternal[numOfDataPointsInSet];
    writeToBuffer(h5Buffer);
    const int numOfPointsPerRow = 1; //this will be repeated alot, make Macro?
    const int numOfDimensions = 2; //this will be repeated alot, make Macro?
    hsize_t dimensionSizes[numOfDimensions];
    dimensionSizes[0] = numOfDataPointsInSet;
    dimensionSizes[1] = numOfPointsPerRow; //this will be repeated alot, make Macro?
    H5::DataSet dataset;
    H5::DataSpace llhSpace = H5::DataSpace( numOfDimensions, dimensionSizes );
    dataset = H5::DataSet(assignedGroup.createDataSet("LLH", llhFileType, llhSpace));
    dataset.write( h5Buffer, llhMemoryType );
    delete[] h5Buffer;
    llhSpace.close();
    dataset.close();
    return 0;
}

int LlhDataSet::readDataSetFromGroup(H5::Group pointsGroup)
{
    //open dataset
    H5::DataSet dataset = H5::DataSet(pointsGroup.openDataSet("LLH"));
    //get dataspace from dataset
    H5::DataSpace dataspace = dataset.getSpace();
    hssize_t numberOfRowsInDataSet = dataspace.getSimpleExtentNpoints();
    //create read in buffer
    llhInternal *h5Buffer = new llhInternal[numberOfRowsInDataSet];
    //read file into Buffer
    dataset.read(h5Buffer, llhMemoryType);
    //fill dataContainer with buffer contents
    readToDataContainer(h5Buffer, numberOfRowsInDataSet);
    delete[] h5Buffer;
    dataspace.close();
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
llhExternal LlhDataSet::getDataPoint(int index)
{
    return dataContainer[index];
}

void LlhDataSet::writeToBuffer(llhInternal *buffer)
{
    int index = 0;
    for(std::vector<llhExternal>::iterator it = dataContainer.begin(); it != dataContainer.end(); ++it)
    {
        llhInternal h5Struct;

        h5Struct.adjE = it->adjE;
        h5Struct.adjN = it->adjN;
        h5Struct.adjU = it->adjU;
        h5Struct.azim = it->azim;
        h5Struct.Cee = it->Cee;
        h5Struct.Ceu = it->Ceu;
        h5Struct.Cne = it->Cne;
        h5Struct.Cnn = it->Cnn;
        h5Struct.Cnu = it->Cnu;
        h5Struct.constraints = it->constraints.c_str();
        h5Struct.Cuu = it->Cuu;
        h5Struct.dovE = it->dovE;
        h5Struct.dovN = it->dovN;
        h5Struct.fixedType = it->fixedType;
        h5Struct.ht = it->ht;
        if(it->utilized == true)
        {
            h5Struct.utilized = BOOL_TRUE;
        }
        else
        {
            h5Struct.utilized = BOOL_FALSE;
        }
        h5Struct.label = it->label.c_str();
        h5Struct.lat = it->lat;
        h5Struct.lon = it->lon;
        h5Struct.maj2D = it->maj2D;
        h5Struct.maj3D = it->maj3D;
        h5Struct.min2D = it->min2D;
        h5Struct.oht = it->oht;
        h5Struct.sigE = it->sigE;
        h5Struct.sigN = it->sigN;
        h5Struct.sigU = it->sigU;
        h5Struct.sourceType = it->sourceType;
        h5Struct.vert = it->vert;
        h5Struct.maj2Drr = it->maj2Drr;
        h5Struct.maj3Drr = it->maj3Drr;
        h5Struct.min2Drr = it->min2Drr;
        h5Struct.vertrr = it->vertrr;
        h5Struct.azrr = it->azrr;



        buffer[index] = h5Struct;
        ++index;
    }
}

void LlhDataSet::readToDataContainer(llhInternal *buffer, hssize_t numberOfDataPointsInSet)
{
    //convert internal to external
    for(int i = 0; i < numberOfDataPointsInSet; i++)
    {
        llhExternal externalStruct;
        externalStruct.adjE = buffer[i].adjE;
        externalStruct.adjN = buffer[i].adjN;
        externalStruct.adjU = buffer[i].adjU;
        externalStruct.azim = buffer[i].azim;
        externalStruct.Cee = buffer[i].Cee;
        externalStruct.Ceu = buffer[i].Ceu;
        externalStruct.Cne = buffer[i].Cne;
        externalStruct.Cnn = buffer[i].Cnn;
        externalStruct.Cnu = buffer[i].Cnu;
        externalStruct.constraints = buffer[i].constraints;
        externalStruct.Cuu = buffer[i].Cuu;
        externalStruct.dovE = buffer[i].dovE;
        externalStruct.dovN = buffer[i].dovN;
        externalStruct.fixedType = buffer[i].fixedType;
        externalStruct.ht = buffer[i].ht;
        if(buffer[i].utilized == BOOL_TRUE)
        {
            externalStruct.utilized = true;
        }
        else
        {
            externalStruct.utilized = false;
        }
        externalStruct.label = buffer[i].label;
        externalStruct.lat = buffer[i].lat;
        externalStruct.lon = buffer[i].lon;
        externalStruct.maj2D = buffer[i].maj2D;
        externalStruct.min2D = buffer[i].min2D;
        externalStruct.maj3D = buffer[i].maj3D;
        externalStruct.oht = buffer[i].oht;
        externalStruct.sigE = buffer[i].sigE;
        externalStruct.sigN = buffer[i].sigN;
        externalStruct.sigU = buffer[i].sigU;
        externalStruct.sourceType = buffer[i].sourceType;
        externalStruct.vert = buffer[i].vert;
        externalStruct.maj2Drr = buffer[i].maj2Drr;
        externalStruct.min2Drr = buffer[i].min2Drr;
        externalStruct.maj3Drr = buffer[i].maj3Drr;
        externalStruct.vertrr = buffer[i].vertrr;
        externalStruct.azrr = buffer[i].azrr;


        dataContainer.push_back(externalStruct);
    }
}

void LlhDataSet::InitLlhDataType()
{
        //this will be repeated a lot, put this code somewhere all classes can reach
        enum_bool_type enumBool;
        LSAFixedState enumFixed;
        enum_source_type enumSource;

        hid_t strType = H5Tcopy(H5T_C_S1);
        H5Tset_size(strType, H5T_VARIABLE);

        hid_t ENUMBOOLMEMORY = H5Tenum_create(H5T_NATIVE_INT);
        H5Tenum_insert(ENUMBOOLMEMORY,            "FALSE",                    CPTR(enumBool, BOOL_FALSE));
        H5Tenum_insert(ENUMBOOLMEMORY,            "TRUE",                     CPTR(enumBool, BOOL_TRUE));

        hid_t ENUMFIXEDMEMORY = H5Tenum_create(H5T_NATIVE_INT);
        H5Tenum_insert(ENUMFIXEDMEMORY,           "FLOAT",                    CPTR(enumFixed, LSAFixedState::FLOATING));
        H5Tenum_insert(ENUMFIXEDMEMORY,           "FIXED",                    CPTR(enumFixed, LSAFixedState::FIXED));
        H5Tenum_insert(ENUMFIXEDMEMORY,           "CONSTRAINED",              CPTR(enumFixed, LSAFixedState::CONSTRAINED));

        hid_t ENUMSOURCEMEMORY = H5Tenum_create(H5T_NATIVE_INT);
        H5Tenum_insert(ENUMSOURCEMEMORY,          "PROVIDED",                 CPTR(enumSource, PROVIDED));
        H5Tenum_insert(ENUMSOURCEMEMORY,          "DERIVED_MEAN",             CPTR(enumSource, DERIVED_MEAN));
        H5Tenum_insert(ENUMSOURCEMEMORY,          "DERIVED_ENUO",             CPTR(enumSource, DERIVED_ENUO));
        H5Tenum_insert(ENUMSOURCEMEMORY,          "AUTOGENERATED",            CPTR(enumSource, AUTOGENERATED));
        //end of repeated code

        //create compound data type for memory
        llhMemoryType = H5Tcreate(H5T_COMPOUND, sizeof(llhInternal));
        H5Tinsert(llhMemoryType,  "Point Label",    HOFFSET(llhInternal, label),        strType);
        H5Tinsert(llhMemoryType,  "Latitude",       HOFFSET(llhInternal, lat),          H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Longitude",      HOFFSET(llhInternal, lon),          H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Height",         HOFFSET(llhInternal, ht),           H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Oht",            HOFFSET(llhInternal, oht),          H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Delta North",    HOFFSET(llhInternal, adjN),         H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Delta East",     HOFFSET(llhInternal, adjE),         H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "delta Up",       HOFFSET(llhInternal, adjU),         H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "sigma North",    HOFFSET(llhInternal, sigN),         H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "sigma East",     HOFFSET(llhInternal, sigE),         H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "sigma Up",       HOFFSET(llhInternal, sigU),         H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Cnn",            HOFFSET(llhInternal, Cnn),          H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Cne",            HOFFSET(llhInternal, Cne),          H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Cnu",            HOFFSET(llhInternal, Cnu),          H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Cee",            HOFFSET(llhInternal, Cee),          H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Ceu",            HOFFSET(llhInternal, Ceu),          H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Cuu",            HOFFSET(llhInternal, Cuu),          H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "DoV North",      HOFFSET(llhInternal, dovN),         H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "DoV East",       HOFFSET(llhInternal, dovE),         H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "2D Major",       HOFFSET(llhInternal, maj2D),        H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "2D Minor",       HOFFSET(llhInternal, min2D),        H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "3D Major",       HOFFSET(llhInternal, maj3D),        H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Azimuth",        HOFFSET(llhInternal, azim),         H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Vertical",       HOFFSET(llhInternal, vert),         H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "2D Major RR",    HOFFSET(llhInternal, maj2Drr),      H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "2D Minor RR",    HOFFSET(llhInternal, min2Drr),      H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "3D Major RR",    HOFFSET(llhInternal, maj3Drr),      H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Azimuth RR",     HOFFSET(llhInternal, azrr),         H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Vertical RR",    HOFFSET(llhInternal, vertrr),       H5T_NATIVE_DOUBLE);
        H5Tinsert(llhMemoryType,  "Source Type",    HOFFSET(llhInternal, sourceType),   ENUMSOURCEMEMORY);
        H5Tinsert(llhMemoryType,  "Utilized",       HOFFSET(llhInternal, utilized),     ENUMBOOLMEMORY);
        H5Tinsert(llhMemoryType,  "Fixed Type",     HOFFSET(llhInternal, fixedType),    ENUMFIXEDMEMORY);
        H5Tinsert(llhMemoryType,  "Constraints",    HOFFSET(llhInternal, constraints),  strType);

        //create compound data type for file
        llhFileType = H5Tcopy(llhMemoryType);
}

int InitialLlhDataSet::writeToGroup(H5::Group assignedGroup)
{
    const int numOfDataPointsInSet = dataContainer.size();
    llhInternal *h5Buffer = new llhInternal[numOfDataPointsInSet];
    writeToBuffer(h5Buffer);
    const int numOfPointsPerRow = 1; //this will be repeated alot, make Macro?
    const int numOfDimensions = 2; //this will be repeated alot, make Macro?
    hsize_t dimensionSizes[numOfDimensions];
    dimensionSizes[0] = numOfDataPointsInSet;
    dimensionSizes[1] = numOfPointsPerRow; //this will be repeated alot, make Macro?
    H5::DataSet dataset;
    H5::DataSpace llhSpace = H5::DataSpace( numOfDimensions, dimensionSizes );
    dataset = H5::DataSet(assignedGroup.createDataSet("Initial LLH", llhFileType, llhSpace));
    dataset.write( h5Buffer, llhMemoryType );
    delete[] h5Buffer;
    llhSpace.close();
    dataset.close();
    return 0;
}

int InitialLlhDataSet::readDataSetFromGroup(H5::Group pointsGroup)
{
    //open dataset
    H5::DataSet dataset = H5::DataSet(pointsGroup.openDataSet("Initial LLH"));
    //get dataspace from dataset
    H5::DataSpace dataspace = dataset.getSpace();
    hssize_t numberOfRowsInDataSet = dataspace.getSimpleExtentNpoints();
    //create read in buffer
    llhInternal *h5Buffer = new llhInternal[numberOfRowsInDataSet];
    //read file into Buffer
    dataset.read(h5Buffer, llhMemoryType);
    //fill dataContainer with buffer contents
    readToDataContainer(h5Buffer, numberOfRowsInDataSet);
    delete[] h5Buffer;
    dataspace.close();
    dataset.close();
    return 0;
}

void XyxDataSet::setGroup(H5::Group assignedGroup)
{
    group = assignedGroup;
}

int XyxDataSet::getSize()
{
    return dataContainer.size();
}

void XyxDataSet::addDataPoint(std::string label, double x, double y, double z, enum_source_type sourceType, bool utilized,
                  LSAFixedState fixedType, std::string constraint, double adjX, double adjY, double adjZ, double sigX,
                 double sigY, double sigZ, double Cxx, double Cxy, double Cxz, double Cyy, double Cyz, double Czz)
{
    xyzExternal dataStruct;
    dataStruct.adjX = adjX;
    dataStruct.adjY = adjY;
    dataStruct.adjZ = adjZ;
    dataStruct.constraints = constraint;
    dataStruct.Cxx = Cxx;
    dataStruct.Cxy = Cxy;
    dataStruct.Cxz = Cxz;
    dataStruct.Cyy = Cyy;
    dataStruct.Cyz = Cyz;
    dataStruct.Czz = Czz;
    dataStruct.fixedType = fixedType;
    dataStruct.utilized = utilized;
    dataStruct.label = label;
    dataStruct.sigX = sigX;
    dataStruct.sigY = sigY;
    dataStruct.sigZ = sigZ;
    dataStruct.sourceType = sourceType;
    dataStruct.x = x;
    dataStruct.y = y;
    dataStruct.z = z;
    dataContainer.push_back(dataStruct);
}

void XyxDataSet::addDataPoint(xyzExternal dataStruct)
{
    dataContainer.push_back(dataStruct);
}

int XyxDataSet::writeToGroup(H5::Group assignedGroup)
{
    const int numOfDataPointsInSet = dataContainer.size();
    xyzInternal *h5Buffer = new xyzInternal[numOfDataPointsInSet];
    writeToBuffer(h5Buffer);
    const int numOfPointsPerRow = 1; //this will be repeated alot, make Macro?
    const int numOfDimensions = 2; //this will be repeated alot, make Macro?
    hsize_t dimensionSizes[numOfDimensions];
    dimensionSizes[0] = numOfDataPointsInSet;
    dimensionSizes[1] = numOfPointsPerRow; //this will be repeated alot, make Macro?
    H5::DataSet dataset;
    H5::DataSpace xyzSpace = H5::DataSpace( numOfDimensions, dimensionSizes );
    dataset = H5::DataSet(assignedGroup.createDataSet("XYZ", xyzFileType, xyzSpace));
    dataset.write( h5Buffer, xyzMemoryType );
    delete[] h5Buffer;
    xyzSpace.close();
    dataset.close();
    return 0;
}

int XyxDataSet::readDataSetFromGroup(H5::Group pointsGroup)
{
    //open dataset
    H5::DataSet dataset = H5::DataSet(pointsGroup.openDataSet("XYZ"));
    //get dataspace from dataset
    H5::DataSpace dataspace = dataset.getSpace();
    hssize_t numberOfRowsInDataSet = dataspace.getSimpleExtentNpoints();
    //create read in buffer
    xyzInternal *h5Buffer = new xyzInternal[numberOfRowsInDataSet];
    //read file into Buffer
    dataset.read(h5Buffer, xyzMemoryType);
    //fill dataContainer with buffer contents
    readToDataContainer(h5Buffer, numberOfRowsInDataSet);
    delete[] h5Buffer;
    dataspace.close();
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
xyzExternal XyxDataSet::getDataPoint(int index)
{
    return dataContainer[index];
}

void XyxDataSet::writeToBuffer(xyzInternal *buffer)
{
    int index = 0;
    for(std::vector<xyzExternal>::iterator it = dataContainer.begin(); it != dataContainer.end(); ++it)
    {
        xyzInternal h5Struct;

        h5Struct.adjX = it->adjX;
        h5Struct.adjY = it->adjY;
        h5Struct.adjZ = it->adjZ;
        h5Struct.constraints = it->constraints.c_str();
        h5Struct.Cxx = it->Cxx;
        h5Struct.Cxy = it->Cxy;
        h5Struct.Cxz = it->Cxz;
        h5Struct.Cyy = it->Cyy;
        h5Struct.Cyz = it->Cyz;
        h5Struct.Czz = it->Czz;
        h5Struct.fixedType = it->fixedType;
        if(it->utilized == true)
        {
            h5Struct.utilized = BOOL_TRUE;
        }
        else
        {
            h5Struct.utilized = BOOL_FALSE;
        }
        h5Struct.label = it->label.c_str();
        h5Struct.sigX = it->sigX;
        h5Struct.sigY = it->sigY;
        h5Struct.sigZ = it->sigZ;
        h5Struct.sourceType = it->sourceType;
        h5Struct.x = it->x;
        h5Struct.y = it->y;
        h5Struct.z = it->z;

        buffer[index] = h5Struct;
        ++index;
    }
}

void XyxDataSet::readToDataContainer(xyzInternal *buffer, hssize_t numberOfDataPointsInSet)
{
    //convert internal to external
    for(int i = 0; i < numberOfDataPointsInSet; i++)
    {
        xyzExternal externalStruct;
        externalStruct.adjX = buffer[i].adjX;
        externalStruct.adjY = buffer[i].adjY;
        externalStruct.adjZ = buffer[i].adjZ;
        externalStruct.constraints = buffer[i].constraints;
        externalStruct.Cxx = buffer[i].Cxx;
        externalStruct.Cxy = buffer[i].Cxy;
        externalStruct.Cxz = buffer[i].Cxz;
        externalStruct.Cyy = buffer[i].Cyy;
        externalStruct.Cyz = buffer[i].Cyz;
        externalStruct.Czz = buffer[i].Czz;
        externalStruct.fixedType = buffer[i].fixedType;
        if(buffer[i].utilized == BOOL_TRUE)
        {
            externalStruct.utilized = true;
        }
        else
        {
            externalStruct.utilized = false;
        }
        externalStruct.label = buffer[i].label;
        externalStruct.sigX = buffer[i].sigX;
        externalStruct.sigY = buffer[i].sigY;
        externalStruct.sigZ = buffer[i].sigZ;
        externalStruct.sourceType = buffer[i].sourceType;
        externalStruct.x = buffer[i].x;
        externalStruct.y = buffer[i].y;
        externalStruct.z = buffer[i].z;
        dataContainer.push_back(externalStruct);
    }
}

//init the compound datatype for xyz dataset
void XyxDataSet::InitXyzDataType(){

    //this will be repeated a lot, put this code somewhere all classes can reach
    enum_bool_type enumBool;
    LSAFixedState enumFixed;
    enum_source_type enumSource;

    hid_t strType = H5Tcopy(H5T_C_S1);
    H5Tset_size(strType, H5T_VARIABLE);

    hid_t ENUMBOOLMEMORY = H5Tenum_create(H5T_NATIVE_INT);
    H5Tenum_insert(ENUMBOOLMEMORY,            "FALSE",                    CPTR(enumBool, BOOL_FALSE));
    H5Tenum_insert(ENUMBOOLMEMORY,            "TRUE",                     CPTR(enumBool, BOOL_TRUE));

    hid_t ENUMFIXEDMEMORY = H5Tenum_create(H5T_NATIVE_INT);
    H5Tenum_insert(ENUMFIXEDMEMORY,           "FLOAT",                    CPTR(enumFixed, LSAFixedState::FLOATING));
    H5Tenum_insert(ENUMFIXEDMEMORY,           "FIXED",                    CPTR(enumFixed, LSAFixedState::FIXED));
    H5Tenum_insert(ENUMFIXEDMEMORY,           "CONSTRAINED",              CPTR(enumFixed, LSAFixedState::CONSTRAINED));

    hid_t ENUMSOURCEMEMORY = H5Tenum_create(H5T_NATIVE_INT);
    H5Tenum_insert(ENUMSOURCEMEMORY,          "PROVIDED",                 CPTR(enumSource, PROVIDED));
    H5Tenum_insert(ENUMSOURCEMEMORY,          "DERIVED_MEAN",             CPTR(enumSource, DERIVED_MEAN));
    H5Tenum_insert(ENUMSOURCEMEMORY,          "DERIVED_ENUO",             CPTR(enumSource, DERIVED_ENUO));
    H5Tenum_insert(ENUMSOURCEMEMORY,          "AUTOGENERATED",            CPTR(enumSource, AUTOGENERATED));
    //end of repeated code

    xyzMemoryType = H5Tcreate(H5T_COMPOUND, sizeof(xyzInternal));
    H5Tinsert(xyzMemoryType, "Point Label", HOFFSET(xyzInternal, label), strType);
    H5Tinsert(xyzMemoryType, "X",           HOFFSET(xyzInternal, x),            H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Y",           HOFFSET(xyzInternal, y),            H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Z",           HOFFSET(xyzInternal, z),            H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Delta X",     HOFFSET(xyzInternal, adjX),         H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Delta Y",     HOFFSET(xyzInternal, adjY),         H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Delta Z",     HOFFSET(xyzInternal, adjZ),         H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Sigma X",     HOFFSET(xyzInternal, sigX),         H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Sigma Y",     HOFFSET(xyzInternal, sigY),         H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Sigma Z",     HOFFSET(xyzInternal, sigZ),         H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Cxx",         HOFFSET(xyzInternal, Cxx),          H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Cxy",         HOFFSET(xyzInternal, Cxy),          H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Cxz",         HOFFSET(xyzInternal, Cxz),          H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Cyy",         HOFFSET(xyzInternal, Cyy),          H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Cyz",         HOFFSET(xyzInternal, Cyz),          H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Czz",         HOFFSET(xyzInternal, Czz),          H5T_NATIVE_DOUBLE);
    H5Tinsert(xyzMemoryType, "Source Type", HOFFSET(xyzInternal, sourceType),   ENUMSOURCEMEMORY);
    H5Tinsert(xyzMemoryType, "Utilized",    HOFFSET(xyzInternal, utilized),     ENUMBOOLMEMORY);
    H5Tinsert(xyzMemoryType, "Fixed Type",  HOFFSET(xyzInternal, fixedType),    ENUMFIXEDMEMORY);
    H5Tinsert(xyzMemoryType, "Constraints", HOFFSET(xyzInternal, constraints),  strType);

    xyzFileType = H5Tcopy(xyzMemoryType);
}

const int MeasurementsDataSet::getSize()
{
    return dataContainer.size();
}

void MeasurementsDataSet::addDataPoint(std::string measType, std::string from, std::string to, std::string tag,
                  double initMeas, double adjMeas, double rawResid, double relResid, double stdResid,
                  double redund, double minDectBias, double extVectMag, bool isBlunder, bool redundancyZero, bool isDerived, std::string at)
{
    measurementsExternal dataStruct;
    dataStruct.measType = measType.c_str();
    dataStruct.from = from.c_str();
    dataStruct.to = to.c_str();
    dataStruct.tag = tag.c_str();
    dataStruct.initMeas = initMeas;
    dataStruct.adjMeas = adjMeas;
    dataStruct.rawResid = rawResid;
    dataStruct.relResid = relResid;
    dataStruct.stdResid = stdResid;
    dataStruct.redund = redund;
    dataStruct.minDectBias = minDectBias;
    dataStruct.extVectMag = extVectMag;
    dataStruct.isBlunder = isBlunder;
    dataStruct.redundancyZero = redundancyZero;
    dataStruct.isDerived = isDerived;
    dataStruct.at = at.c_str();
    dataContainer.push_back(dataStruct);
}

void MeasurementsDataSet::addDataPoint(measurementsExternal dataStruct)
{
    dataContainer.push_back(dataStruct);
}

int MeasurementsDataSet::writeToGroup(H5::Group measurementsGroup)
{
    const int numOfDataPointsInSet = dataContainer.size();
    measurementsInternal *h5Buffer = new measurementsInternal[numOfDataPointsInSet];
    writeToBuffer(h5Buffer);
    const int numOfPointsPerRow = 1; //this will be repeated alot, make Macro?
    const int numOfDimensions = 2; //this will be repeated alot, make Macro?
    hsize_t dimensionSizes[numOfDimensions];
    dimensionSizes[0] = numOfDataPointsInSet;
    dimensionSizes[1] = numOfPointsPerRow; //this will be repeated alot, make Macro?
    H5::DataSet dataset;
    H5::DataSpace measurementsSpace = H5::DataSpace( numOfDimensions, dimensionSizes );
    dataset = H5::DataSet(measurementsGroup.createDataSet("Measurements", measurementsFileType, measurementsSpace));
    dataset.write( h5Buffer, measurementsMemoryType );
    delete[] h5Buffer;
    measurementsSpace.close();
    dataset.close();
    return 0;
}

int MeasurementsDataSet::readDataSetFromGroup(H5::Group measurementsGroup)
{
    //open dataset
    H5::DataSet dataset = H5::DataSet(measurementsGroup.openDataSet("Measurements"));
    //get dataspace from dataset
    H5::DataSpace measurementsSpace = dataset.getSpace();
    hssize_t numberOfRowsInDataSet = measurementsSpace.getSimpleExtentNpoints();
    //create read in buffer
    measurementsInternal *h5Buffer = new measurementsInternal[numberOfRowsInDataSet];
    //read file into Buffer
    dataset.read(h5Buffer, measurementsMemoryType);
    //fill dataContainer with buffer contents
    readToDataContainer(h5Buffer, numberOfRowsInDataSet);
    delete[] h5Buffer;
    measurementsSpace.close();
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
const measurementsExternal MeasurementsDataSet::getDataPoint(int index)
{
    return dataContainer[index];
}

void MeasurementsDataSet::writeToBuffer(measurementsInternal *buffer)
{
    int index = 0;
    for(std::vector<measurementsExternal>::iterator it = dataContainer.begin(); it != dataContainer.end(); ++it)
    {
        measurementsInternal internalStruct;

        internalStruct.adjMeas = it->adjMeas;
        internalStruct.at = it->at.c_str();
        internalStruct.from = it->from.c_str();
        internalStruct.initMeas = it->initMeas;
        if(it->isBlunder == true)
        {
            internalStruct.isBlunder = BOOL_TRUE;
        }
        else
        {
            internalStruct.isBlunder = BOOL_FALSE;
        }
        if(it->isDerived == true)
        {
            internalStruct.isDerived = BOOL_TRUE;
        }
        else
        {
            internalStruct.isDerived = BOOL_FALSE;
        }
        if(it->redundancyZero == true)
        {
            internalStruct.redundancyZero = BOOL_TRUE;
        }
        else
        {
            internalStruct.redundancyZero = BOOL_FALSE;
        }
        internalStruct.measType = it->measType.c_str();
        internalStruct.rawResid = it->rawResid;
        internalStruct.redund = it->redund;
        internalStruct.minDectBias = it->minDectBias;
        internalStruct.extVectMag = it->extVectMag;
        internalStruct.relResid = it->relResid;
        internalStruct.stdResid = it->stdResid;
        internalStruct.tag = it->tag.c_str();
        internalStruct.to = it->to.c_str();

        buffer[index] = internalStruct;
        ++index;
    }
}

void MeasurementsDataSet::readToDataContainer(measurementsInternal *buffer, hssize_t numberOfDataPointsInSet)
{
    //convert internal to external
    for(int i = 0; i < numberOfDataPointsInSet; i++)
    {
        measurementsExternal externalStruct;
        externalStruct.adjMeas = buffer[i].adjMeas;
        externalStruct.at = buffer[i].at;
        externalStruct.from = buffer[i].from;
        externalStruct.initMeas = buffer[i].initMeas;
        if(buffer[i].isBlunder == BOOL_TRUE)
        {
            externalStruct.isBlunder = true;
        }
        else
        {
            externalStruct.isBlunder = false;
        }
        if(buffer[i].isDerived == BOOL_TRUE)
        {
            externalStruct.isDerived = true;
        }
        else
        {
            externalStruct.isDerived = false;
        }
        if(buffer[i].redundancyZero == BOOL_TRUE)
        {
            externalStruct.redundancyZero = true;
        }
        else
        {
            externalStruct.redundancyZero = false;
        }
        externalStruct.measType = buffer[i].measType;
        externalStruct.rawResid = buffer[i].rawResid;
        externalStruct.redund = buffer[i].redund;
        externalStruct.minDectBias = buffer[i].minDectBias;
        externalStruct.extVectMag = buffer[i].extVectMag;
        externalStruct.relResid = buffer[i].relResid;
        externalStruct.stdResid = buffer[i].stdResid;
        externalStruct.tag = buffer[i].tag;
        externalStruct.to = buffer[i].to;
        dataContainer.push_back(externalStruct);
    }
}

//init the compound datatype for xyz dataset
void MeasurementsDataSet::InitMeasurementDataType(){
    //this will be repeated a lot, put this code somewhere all classes can reach
    enum_bool_type enumBool;

    hid_t strType = H5Tcopy(H5T_C_S1);
    H5Tset_size(strType, H5T_VARIABLE);

    hid_t ENUMBOOLMEMORY = H5Tenum_create(H5T_NATIVE_INT);
    H5Tenum_insert(ENUMBOOLMEMORY,            "FALSE",                    CPTR(enumBool, BOOL_FALSE));
    H5Tenum_insert(ENUMBOOLMEMORY,            "TRUE",                     CPTR(enumBool, BOOL_TRUE));
    //end of repeated code

    measurementsMemoryType = H5Tcreate(H5T_COMPOUND, sizeof(measurementsInternal));
    H5Tinsert(measurementsMemoryType, "Measurement Type",               HOFFSET(measurementsInternal, measType),    strType);
    H5Tinsert(measurementsMemoryType, "From Point Name",                HOFFSET(measurementsInternal, from),        strType);
    H5Tinsert(measurementsMemoryType, "At Point Name",                  HOFFSET(measurementsInternal, at),          strType);
    H5Tinsert(measurementsMemoryType, "To Point Name",                  HOFFSET(measurementsInternal, to),          strType);
    H5Tinsert(measurementsMemoryType, "Tag",                            HOFFSET(measurementsInternal, tag),         strType);
    H5Tinsert(measurementsMemoryType, "Initial Measurement",            HOFFSET(measurementsInternal, initMeas),    H5T_NATIVE_DOUBLE);
    H5Tinsert(measurementsMemoryType, "Adjusted Measurement",           HOFFSET(measurementsInternal, adjMeas),     H5T_NATIVE_DOUBLE);
    H5Tinsert(measurementsMemoryType, "Raw Residual",                   HOFFSET(measurementsInternal, rawResid),    H5T_NATIVE_DOUBLE);
    H5Tinsert(measurementsMemoryType, "Relative Residual",              HOFFSET(measurementsInternal, relResid),    H5T_NATIVE_DOUBLE);
    H5Tinsert(measurementsMemoryType, "Standard Residual",              HOFFSET(measurementsInternal, stdResid),    H5T_NATIVE_DOUBLE);
    H5Tinsert(measurementsMemoryType, "Redundancy",                     HOFFSET(measurementsInternal, redund),      H5T_NATIVE_DOUBLE);
    H5Tinsert(measurementsMemoryType, "Minimum Detectable Bias",        HOFFSET(measurementsInternal, minDectBias), H5T_NATIVE_DOUBLE);
    H5Tinsert(measurementsMemoryType, "External Reliability Magnitude", HOFFSET(measurementsInternal, extVectMag),  H5T_NATIVE_DOUBLE);

    H5Tinsert(measurementsMemoryType, "Blunders Present",               HOFFSET(measurementsInternal, isBlunder),        ENUMBOOLMEMORY);
    H5Tinsert(measurementsMemoryType, "Redundancy equals zero",         HOFFSET(measurementsInternal, redundancyZero),   ENUMBOOLMEMORY);
    H5Tinsert(measurementsMemoryType, "Is Measurement Derived",         HOFFSET(measurementsInternal, isDerived),        ENUMBOOLMEMORY);

    measurementsFileType = H5Tcopy(measurementsMemoryType);
}

void LowestRedundDataSet::addDataPoint(double lowestRedundancy)
{
    dataContainer = lowestRedundancy;
}

int LowestRedundDataSet::writeToGroup(H5::Group measurementsGroup)
{
    const int numOfDimensions = 1;
    const int numOfRowsInTable = 1;
    const int numOfStructsPerRow = 1; //repeated a lot
    double lowestRedundBuffer[numOfRowsInTable];
    lowestRedundBuffer[0] = dataContainer;
    hsize_t dimensionSizes[numOfDimensions];
    dimensionSizes[0] = numOfStructsPerRow;
    H5::DataSpace lowestRedundSpace = H5::DataSpace( numOfDimensions, dimensionSizes );
    H5::DataSet dataset = H5::DataSet(measurementsGroup.createDataSet("Lowest Redundancy", H5::PredType::NATIVE_DOUBLE, lowestRedundSpace));
    dataset.write( lowestRedundBuffer, H5::PredType::NATIVE_DOUBLE );
    lowestRedundSpace.close();
    dataset.close();
    return 0;
}

int LowestRedundDataSet::readDataSetFromGroup(H5::Group measurementsGroup)
{
    H5::DataSet dataset = H5::DataSet(measurementsGroup.openDataSet("Lowest Redundancy"));
    //create read in buffer
    double readBuffer[1];
    //read file
    dataset.read( readBuffer, H5::PredType::NATIVE_DOUBLE);
    dataContainer = readBuffer[0];
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
const double LowestRedundDataSet::getDataPoint()
{
    return dataContainer;
}

void BlundersExistDataSet::addDataPoint(bool blundersExist)
{
    dataContainer = blundersExist;
}

int BlundersExistDataSet::writeToGroup(H5::Group measurementsGroup)
{
    enum_bool_type output;
    if(dataContainer == true){
        output = BOOL_TRUE;
    }
    else{
        output = BOOL_FALSE;
    }
    const int numOfDimensions = 1;
    const int numOfRowsInTable = 1;
    const int numOfStructsPerRow = 1; //repeated code
    enum_bool_type blunderExistsBuffer[numOfRowsInTable];
    blunderExistsBuffer[0] = output;
    hsize_t dimensionSizes[numOfDimensions];
    dimensionSizes[0] = numOfStructsPerRow;
    H5::DataSpace blunderExistsSpace = H5::DataSpace( numOfDimensions, dimensionSizes );
    H5::DataSet dataset = H5::DataSet(measurementsGroup.createDataSet("Blunder Exists", ENUMBOOLFILE, blunderExistsSpace));
    dataset.write( blunderExistsBuffer, ENUMBOOLMEMORY );
    blunderExistsSpace.close();
    dataset.close();
    return 0;
}

int BlundersExistDataSet::readDataSetFromGroup(H5::Group measurementsGroup)
{
    H5::DataSet dataset = H5::DataSet(measurementsGroup.openDataSet("Blunder Exists"));
    //create read in buffer
    enum_bool_type readBuffer[1];
    //read file
    dataset.read( readBuffer, ENUMBOOLMEMORY);
    bool enumBoolToBool;
    if(readBuffer[0] == BOOL_TRUE)
    {
        enumBoolToBool = true;
    }
    else
    {
        enumBoolToBool = false;
    }
    dataContainer = enumBoolToBool;
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
const bool BlundersExistDataSet::getDataPoint()
{
    return dataContainer;
}

void BlundersExistDataSet::InitBlunderDataType(){
    //this will be repeated a lot, put this code somewhere all classes can reach
    enum_bool_type enumBool;
    ENUMBOOLMEMORY = H5Tenum_create(H5T_NATIVE_INT);
    H5Tenum_insert(ENUMBOOLMEMORY,            "FALSE",                    CPTR(enumBool, BOOL_FALSE));
    H5Tenum_insert(ENUMBOOLMEMORY,            "TRUE",                     CPTR(enumBool, BOOL_TRUE));

    ENUMBOOLFILE = H5Tcopy(ENUMBOOLMEMORY);
    //end of repeated code
}

const int ConvergenceHistDataSet::getSize()
{
    return dataContainer.size();
}

void ConvergenceHistDataSet::addDataPoint(int iterationNum, double RMSadj, double delta, double RMSrelResid, std::string status)
{
    convergenceHistExternal dataStruct;

    dataStruct.iterationNum = iterationNum;
    dataStruct.RMSadj = RMSadj;
    dataStruct.delta = delta;
    dataStruct.RMSrelResid = RMSrelResid;
    dataStruct.status = status.c_str();

    dataContainer.push_back(dataStruct);
}

void ConvergenceHistDataSet::addDataPoint(convergenceHistExternal dataStruct)
{
    dataContainer.push_back(dataStruct);
}

int ConvergenceHistDataSet::writeToGroup(H5::Group adjustmentResultsGroup)
{
    const int numOfDataPointsInSet = dataContainer.size();
    convergenceHistInternal *h5Buffer = new convergenceHistInternal[numOfDataPointsInSet];
    writeToBuffer(h5Buffer);
    const int numOfPointsPerRow = 1; //this will be repeated alot, make Macro?
    const int numOfDimensions = 2; //this will be repeated alot, make Macro?
    hsize_t dimensionSizes[numOfDimensions];
    dimensionSizes[0] = numOfDataPointsInSet;
    dimensionSizes[1] = numOfPointsPerRow; //this will be repeated alot, make Macro?
    H5::DataSet dataset;
    H5::DataSpace dataSpace = H5::DataSpace( numOfDimensions, dimensionSizes );
    dataset = H5::DataSet(adjustmentResultsGroup.createDataSet("Convergence History", convergenceHistFileType, dataSpace));
    dataset.write( h5Buffer, convergenceHistMemoryType );
    delete[] h5Buffer;
    dataSpace.close();
    dataset.close();
    return 0;
}

int ConvergenceHistDataSet::readDataSetFromGroup(H5::Group adjustmentResultsGroup)
{
    //open dataset
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.openDataSet("Convergence History"));
    //get dataspace from dataset
    H5::DataSpace dataSpace = dataset.getSpace();
    hssize_t numberOfRowsInDataSet = dataSpace.getSimpleExtentNpoints();
    //create read in buffer
    convergenceHistInternal *h5Buffer = new convergenceHistInternal[numberOfRowsInDataSet];
    //read file into Buffer
    dataset.read(h5Buffer, convergenceHistMemoryType);
    //fill dataContainer with buffer contents
    readToDataContainer(h5Buffer, numberOfRowsInDataSet);
    delete[] h5Buffer;
    dataSpace.close();
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
const convergenceHistExternal ConvergenceHistDataSet::getDataPoint(int index)
{
    return dataContainer[index];
}

const convergenceHistExternal ConvergenceHistDataSet::back(){
    return dataContainer.back();
}

void ConvergenceHistDataSet::writeToBuffer(convergenceHistInternal *buffer)
{
    int index = 0;
    for(std::vector<convergenceHistExternal>::iterator it = dataContainer.begin(); it != dataContainer.end(); ++it)
    {
        convergenceHistInternal internalStruct;

        internalStruct.delta = it->delta;
        internalStruct.iterationNum = it->iterationNum;
        internalStruct.RMSadj = it->RMSadj;
        internalStruct.RMSrelResid = it->RMSrelResid;
        internalStruct.status = it->status.c_str();

        buffer[index] = internalStruct;
        ++index;
    }
}

void ConvergenceHistDataSet::readToDataContainer(convergenceHistInternal *buffer, hssize_t numberOfDataPointsInSet)
{
    //convert internal to external
    for(int i = 0; i < numberOfDataPointsInSet; i++)
    {
        convergenceHistExternal externalStruct;
        externalStruct.delta = buffer[i].delta;
        externalStruct.iterationNum = buffer[i].iterationNum;
        externalStruct.RMSadj = buffer[i].RMSadj;
        externalStruct.RMSrelResid = buffer[i].RMSrelResid;
        externalStruct.status = buffer[i].status;
        dataContainer.push_back(externalStruct);
    }
}

//init the compound datatype for xyz dataset
void ConvergenceHistDataSet::InitconvergenceHistDataType(){
    //this will be repeated a lot, put this code somewhere all classes can reach
    hid_t strType = H5Tcopy(H5T_C_S1);
    H5Tset_size(strType, H5T_VARIABLE);
    //end repeated code

    convergenceHistMemoryType = H5Tcreate(H5T_COMPOUND, sizeof(convergenceHistInternal) );
    H5Tinsert(convergenceHistMemoryType, "Number Of Iterations",   HOFFSET(convergenceHistInternal, iterationNum), H5T_NATIVE_INT);
    H5Tinsert(convergenceHistMemoryType, "RMS Adjustment",         HOFFSET(convergenceHistInternal, RMSadj),       H5T_NATIVE_DOUBLE);
    H5Tinsert(convergenceHistMemoryType, "Delta",                  HOFFSET(convergenceHistInternal, delta),        H5T_NATIVE_DOUBLE);
    H5Tinsert(convergenceHistMemoryType, "RMS Relative Residual",  HOFFSET(convergenceHistInternal, RMSrelResid),  H5T_NATIVE_DOUBLE);
    H5Tinsert(convergenceHistMemoryType, "Status",                 HOFFSET(convergenceHistInternal, status),       strType);

    convergenceHistFileType = H5Tcopy(convergenceHistMemoryType);
}

const int SolverWariningsDataSet::getSize()
{
    return dataContainer.size();
}

void SolverWariningsDataSet::addDataPoint(std::string warningString)
{
    dataContainer.push_back(warningString);
}

int SolverWariningsDataSet::writeToGroup(H5::Group adjustmentResultsGroup)
{

    const int numOfDimensions = 1;
    const int numOfRowsInTable = dataContainer.size();
    char **solverWarningsBuffer = new char*[numOfRowsInTable];
    writeToBuffer(solverWarningsBuffer);
    hsize_t dimensionSizes[numOfDimensions];
    dimensionSizes[0] = numOfRowsInTable;
    H5::DataSpace solverWarningsSpace = H5::DataSpace(numOfDimensions, dimensionSizes );
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.createDataSet("Solver Warnings", solverWarningFileType, solverWarningsSpace));
    dataset.write( solverWarningsBuffer, solverWarningMemoryType );
    solverWarningsSpace.close();
    dataset.close();
    //free memory reserved from solverWarningsBuffer
    for(int i=0; i<numOfRowsInTable; ++i)
    {
       delete[] solverWarningsBuffer[i];
    }
    delete[] solverWarningsBuffer;
    return 0;
}

int SolverWariningsDataSet::readDataSetFromGroup(H5::Group adjustmentResultsGroup)
{
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.openDataSet("Solver Warnings"));
    //get dataspace from dataset
    H5::DataSpace dataspace = dataset.getSpace();
    hssize_t numberOfRowsInDataSet = dataspace.getSimpleExtentNpoints();
    //create read in buffer
    solverWarningInternal *readBuffer;
    readBuffer = new solverWarningInternal[numberOfRowsInDataSet];
    //read file
    dataset.read(readBuffer, solverWarningMemoryType);
    //convert internal to external
    readToDataContainer(readBuffer, numberOfRowsInDataSet);
    delete[] readBuffer;
    dataspace.close();
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
const std::string SolverWariningsDataSet::getDataPoint(int index)
{
    return dataContainer[index];
}

const std::string SolverWariningsDataSet::back(){
    return dataContainer.back();
}

void SolverWariningsDataSet::writeToBuffer(char **buffer)
{
    //convert external to internal
    const int numOfRowsInTable = dataContainer.size();
    int index = 0;
    for(int i = 0; i < numOfRowsInTable; ++i)
    {
        buffer[index] = new char[dataContainer[index].size() + 1];
        std::strcpy(buffer[index], dataContainer[index].c_str());
        index++;
    }
}

void SolverWariningsDataSet::readToDataContainer(solverWarningInternal *buffer, hssize_t numberOfDataPointsInSet)
{
    //convert internal to external
    for(int i = 0; i < numberOfDataPointsInSet; i++)
    {
        std::string warningCharToString = buffer[i].warningString;
        dataContainer.push_back(warningCharToString);
    }
}

//init the compound datatype for xyz dataset
void SolverWariningsDataSet::initSolverWariningsDataType(){
    solverWarningMemoryType = H5Tcopy(H5T_C_S1);
    H5Tset_size(solverWarningMemoryType, H5T_VARIABLE);

    solverWarningFileType = H5Tcopy(solverWarningMemoryType);
}

int SolverErrorsDataSet::getSize() const
{
    return dataContainer.size();
}

void SolverErrorsDataSet::addDataPoint(std::string warningString)
{
    dataContainer.push_back(warningString);
}

int SolverErrorsDataSet::writeToGroup(H5::Group adjustmentResultsGroup)
{

    const int numOfDimensions = 1;
    const int numOfRowsInTable = dataContainer.size();
    char **solverErrorsBuffer = new char*[numOfRowsInTable];
    writeToBuffer(solverErrorsBuffer);
    hsize_t dimensionSizes[numOfDimensions];
    dimensionSizes[0] = numOfRowsInTable;
    H5::DataSpace SolverErrorsSpace = H5::DataSpace(numOfDimensions, dimensionSizes );
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.createDataSet("Solver Errors", solverErrorFileType, SolverErrorsSpace));
    dataset.write( solverErrorsBuffer, solverErrorMemoryType );
    SolverErrorsSpace.close();
    dataset.close();
    //free memory reserved from solverWarningsBuffer
    for(int i=0; i<numOfRowsInTable; ++i)
    {
       delete[] solverErrorsBuffer[i];
    }
    delete[] solverErrorsBuffer;
    return 0;
}

int SolverErrorsDataSet::readDataSetFromGroup(H5::Group adjustmentResultsGroup)
{
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.openDataSet("Solver Errors"));
    //get dataspace from dataset
    H5::DataSpace dataspace = dataset.getSpace();
    hssize_t numberOfRowsInDataSet = dataspace.getSimpleExtentNpoints();
    //create read in buffer
    solverErrorsInternal *readBuffer;
    readBuffer = new solverErrorsInternal[numberOfRowsInDataSet];
    //read file
    dataset.read(readBuffer, solverErrorMemoryType);
    //convert internal to external
    readToDataContainer(readBuffer, numberOfRowsInDataSet);
    delete[] readBuffer;
    dataspace.close();
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
const std::string SolverErrorsDataSet::getDataPoint(int index)
{
    return dataContainer[index];
}

const std::string SolverErrorsDataSet::back(){
    return dataContainer.back();
}

void SolverErrorsDataSet::writeToBuffer(char **buffer)
{
    //convert external to internal
    const int numOfRowsInTable = dataContainer.size();
    int index = 0;
    for(int i = 0; i < numOfRowsInTable; ++i)
    {
        buffer[index] = new char[dataContainer[index].size() + 1];
        std::strcpy(buffer[index], dataContainer[index].c_str());
        index++;
    }
}

void SolverErrorsDataSet::readToDataContainer(solverErrorsInternal *buffer, hssize_t numberOfDataPointsInSet)
{
    //convert internal to external
    for(int i = 0; i < numberOfDataPointsInSet; i++)
    {
        std::string errorCharToString = buffer[i].errorString;
        dataContainer.push_back(errorCharToString);
    }
}

//init the compound datatype for xyz dataset
void SolverErrorsDataSet::initSolverErrorsDataType(){
    solverErrorMemoryType = H5Tcopy(H5T_C_S1);
    H5Tset_size(solverErrorMemoryType, H5T_VARIABLE);

    solverErrorFileType = H5Tcopy(solverErrorMemoryType);
}

void SolverRunTypeDataSet::addDataPoint(enum_solver_run_type runType)
{
    dataContainer = runType;
}

int SolverRunTypeDataSet::writeToGroup(H5::Group adjustmentResultsGroup)
{
    const int numOfDimensions = 1;
    const int numOfRowsInTable = 1;
    const int numOfStructsPerRow = 1; //repeated code
    enum_solver_run_type SolverRunTypeBuffer[numOfRowsInTable];
    SolverRunTypeBuffer[0] = dataContainer;
    hsize_t dimensionSizes[numOfDimensions];
    dimensionSizes[0] = numOfStructsPerRow;
    H5::DataSpace SolverRunTypeSpace = H5::DataSpace( numOfDimensions, dimensionSizes );
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.createDataSet("Solver Run Type", ENUMSOLVERFILE, SolverRunTypeSpace));
    dataset.write( SolverRunTypeBuffer, ENUMSOLVERMEMORY );
    SolverRunTypeSpace.close();
    dataset.close();
    return 0;
}

int SolverRunTypeDataSet::readDataSetFromGroup(H5::Group adjustmentResultsGroup)
{
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.openDataSet("Solver Run Type"));
    //create read in buffer
    enum_solver_run_type readBuffer[1];
    //read file
    dataset.read( readBuffer, ENUMSOLVERMEMORY);
    dataContainer = readBuffer[0];
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
enum_solver_run_type SolverRunTypeDataSet::getDataPoint() const
{
    return dataContainer;
}

void SolverRunTypeDataSet::InitSolverRunType(){
    enum_solver_run_type enumSolverRun;

    ENUMSOLVERMEMORY = H5Tenum_create(H5T_NATIVE_INT);
    H5Tenum_insert(ENUMSOLVERMEMORY,       "FAST",                     CPTR(enumSolverRun, FAST));
    H5Tenum_insert(ENUMSOLVERMEMORY,       "STABLE",                   CPTR(enumSolverRun, STABLE));
    H5Tenum_insert(ENUMSOLVERMEMORY,       "FALLBACK_TO_STABLE",       CPTR(enumSolverRun, FALLBACK_TO_STABLE));

    ENUMSOLVERFILE = H5Tcopy(ENUMSOLVERMEMORY);
}

void SolverExitTypeDataSet::addDataPoint(lsa::enum_solver_return_code exitType)
{
    dataContainer = exitType;
}

int SolverExitTypeDataSet::writeToGroup(H5::Group adjustmentResultsGroup)
{
    const int numOfDimensions = 1;
    const int numOfRowsInTable = 1;
    const int numOfStructsPerRow = 1; //repeated code
    lsa::enum_solver_return_code SolverExitTypeBuffer[numOfRowsInTable];
    SolverExitTypeBuffer[0] = dataContainer;
    hsize_t dimensionSizes[numOfDimensions];
    dimensionSizes[0] = numOfStructsPerRow;
    H5::DataSpace SolverExitTypeSpace = H5::DataSpace( numOfDimensions, dimensionSizes );
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.createDataSet("Solver Exit Type", ENUMSOLVEREXITFILE, SolverExitTypeSpace));
    dataset.write( SolverExitTypeBuffer, ENUMSOLVEREXITMEMORY );
    SolverExitTypeSpace.close();
    dataset.close();
    return 0;
}

int SolverExitTypeDataSet::readDataSetFromGroup(H5::Group adjustmentResultsGroup)
{
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.openDataSet("Solver Exit Type"));
    //create read in buffer
    lsa::enum_solver_return_code readBuffer[1];
    //read file
    dataset.read( readBuffer, ENUMSOLVEREXITMEMORY);
    dataContainer = readBuffer[0];
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
lsa::enum_solver_return_code SolverExitTypeDataSet::getDataPoint() const
{
    return dataContainer;
}

void SolverExitTypeDataSet::InitSolverRunType(){
    lsa::enum_solver_return_code enumSolverExit;

    ENUMSOLVEREXITMEMORY = H5Tenum_create(H5T_NATIVE_INT);
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "CMDINVALID",               CPTR(enumSolverExit, lsa::CMDINVALID));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "UNKNOWN",                  CPTR(enumSolverExit, lsa::UNKNOWN));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "NOOUTFILE",                CPTR(enumSolverExit, lsa::NOOUTFILE));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "NORMAL",                   CPTR(enumSolverExit, lsa::NORMAL));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "CMDLINEUSAGE",             CPTR(enumSolverExit, lsa::CMDLINEUSAGE));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "CMDLINERRRORS",            CPTR(enumSolverExit, lsa::CMDLINERRRORS));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "USERVALIDATION",           CPTR(enumSolverExit, lsa::USERVALIDATION));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "INPUTINVALID",             CPTR(enumSolverExit, lsa::INPUTINVALID));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "OUTPUTFILEFAIL",           CPTR(enumSolverExit, lsa::OUTPUTFILEFAIL));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "INPUTFILEFAIL",            CPTR(enumSolverExit, lsa::INPUTFILEFAIL));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "EARLYEXIT",                CPTR(enumSolverExit, lsa::EARLYEXIT));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "PROBLEMUNDERDETERMINED",   CPTR(enumSolverExit, lsa::PROBLEMUNDERDETERMINED));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "PROBLEMSINGULAR",          CPTR(enumSolverExit, lsa::PROBLEMSINGULAR));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "UNABLEAPRIORI",            CPTR(enumSolverExit, lsa::UNABLEAPRIORI));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "NODATA",                   CPTR(enumSolverExit, lsa::NODATA));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "NOSTATE",                  CPTR(enumSolverExit, lsa::NOSTATE));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "SINGULARCOV",              CPTR(enumSolverExit, lsa::SINGULARCOV));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "SINGULARMEASCOV",          CPTR(enumSolverExit, lsa::SINGULARMEASCOV));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "SOLUTIONSINGULAR",         CPTR(enumSolverExit, lsa::SOLUTIONSINGULAR));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "INVALIDDIRSET",            CPTR(enumSolverExit, lsa::INVALIDDIRSET));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "GEOIDFILEFAIL",            CPTR(enumSolverExit, lsa::GEOIDFILEFAIL));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "EIGENFAIL",                CPTR(enumSolverExit, lsa::EIGENFAIL));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "MEMORYALLOCATEFAIL",       CPTR(enumSolverExit, lsa::MEMORYALLOCATEFAIL));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "COMPFILEFAIL",             CPTR(enumSolverExit, lsa::COMPFILEFAIL));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "DATFILEFAIL",              CPTR(enumSolverExit, lsa::DATFILEFAIL));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "BINFILEFAIL",              CPTR(enumSolverExit, lsa::BINFILEFAIL));
    H5Tenum_insert(ENUMSOLVEREXITMEMORY,      "BINFILEOBSOLELE",          CPTR(enumSolverExit, lsa::BINFILEOBSOLELE));


    ENUMSOLVEREXITFILE = H5Tcopy(ENUMSOLVEREXITMEMORY);
}

void StatisticsDataSet::addDataPoint(double relRMSresid, double stdDevUnitWieght, double APV, double angRMSresidSOA, double lenRMSresid, double lowerChiSq, double upperChiSq)
{
    dataContainer.relRMSresid = relRMSresid;
    dataContainer.stdDevUnitWieght = stdDevUnitWieght;
    dataContainer.APV = APV;
    dataContainer.angRMSresidSOA = angRMSresidSOA;
    dataContainer.lenRMSresid = lenRMSresid;
    dataContainer.lowerChiSq = lowerChiSq;
    dataContainer.upperChiSq = upperChiSq;

}

void StatisticsDataSet::addDataPoint(statisticsExternal dataPoint)
{
    dataContainer = dataPoint;
}

int StatisticsDataSet::writeToGroup(H5::Group adjustmentResultsGroup)
{
    const int numOfRowsInTable = 1;
    const int numOfStructsPerRow = 1;
    statisticsExternal probDescriptBuffer[numOfRowsInTable];
    probDescriptBuffer[0] = dataContainer;
    hsize_t numOfStructsInRow[] = {numOfStructsPerRow};
    H5::DataSpace statisticsSpace = H5::DataSpace( numOfRowsInTable, numOfStructsInRow );
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.createDataSet("Statistics", statFileType, statisticsSpace));
    dataset.write( probDescriptBuffer, statMemoryType );
    statisticsSpace.close();
    dataset.close();
    return 0;
}

int StatisticsDataSet::readDataSetFromGroup(H5::Group adjustmentResultsGroup)
{
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.openDataSet("Statistics"));
    statisticsExternal readBuffer[1];
    //read file
    dataset.read( readBuffer, statMemoryType);
    dataContainer = readBuffer[0];
    dataset.close();
    return 0;
}

//readDataSetFromGroup must be called before this function to work
statisticsExternal StatisticsDataSet::getDataPoint()
{
    return dataContainer;
}

void StatisticsDataSet::initStatisticsDataType(){
    //create compound data type for memory
    statMemoryType = H5Tcreate(H5T_COMPOUND, sizeof(statisticsExternal) );
    H5Tinsert(statMemoryType, "MS Post Fit Relative Residual",          HOFFSET(statisticsExternal, relRMSresid),       H5T_NATIVE_DOUBLE);
    H5Tinsert(statMemoryType, "Standard Deviation Of Unit Weight",      HOFFSET(statisticsExternal, stdDevUnitWieght),  H5T_NATIVE_DOUBLE);
    H5Tinsert(statMemoryType, "APV",                                    HOFFSET(statisticsExternal, APV),               H5T_NATIVE_DOUBLE);
    H5Tinsert(statMemoryType, "RMS Post Fit Raw Residual Angles",       HOFFSET(statisticsExternal, angRMSresidSOA),    H5T_NATIVE_DOUBLE);
    H5Tinsert(statMemoryType, "RMS Post Fit Raw Residual Length",       HOFFSET(statisticsExternal, lenRMSresid),       H5T_NATIVE_DOUBLE);
    H5Tinsert(statMemoryType, "Upper Chi Squared Test",                 HOFFSET(statisticsExternal, upperChiSq),        H5T_NATIVE_DOUBLE);
    H5Tinsert(statMemoryType, "Lower Chi Squared Test",                 HOFFSET(statisticsExternal, lowerChiSq),        H5T_NATIVE_DOUBLE);

    statFileType = H5Tcopy(statMemoryType);
}

void StatisticsDataSet::initContainer(){
    statisticsExternal emptyContainer;
    emptyContainer.angRMSresidSOA = 0.0;
    emptyContainer.APV = 0.0;
    emptyContainer.lenRMSresid = 0.0;
    emptyContainer.lowerChiSq = 0.0;
    emptyContainer.relRMSresid = 0.0;
    emptyContainer.stdDevUnitWieght = 0.0;
    emptyContainer.upperChiSq = 0.0;
    dataContainer = emptyContainer;
}

int SolutionCovarDataSet::getCovarRowCount() const
{
    return covarianceMatrix.size();
}

int SolutionCovarDataSet::getNumOfLabels() const
{
    return axisLabels.size();
}

void SolutionCovarDataSet::addRowToMatrix(std::vector<double> newRow)
{
    covarianceMatrix.push_back(newRow);
}

void SolutionCovarDataSet::setMatrix(std::vector<std::vector<double> > matrix)
{
    covarianceMatrix = matrix;
}

void SolutionCovarDataSet::addAxisLabel(std::string newLabel)
{
    axisLabels.push_back(newLabel);
}

void SolutionCovarDataSet::setAllLabels(std::vector<std::string> labels)
{
    axisLabels = labels;
}

int SolutionCovarDataSet::writeToGroup(H5::Group adjustmentResultsGroup)
{
    const int numOfDimensions = 2;
    const int numOfLabels = this->getNumOfLabels();
    const int CovarYLength = this->getCovarRowCount();
    double *solCovarBuffer = new double[CovarYLength*CovarYLength];
    writeToBuffer(solCovarBuffer);
    hsize_t dimensionSizes[numOfDimensions];
    dimensionSizes[0] = CovarYLength;
    dimensionSizes[1] = CovarYLength;
    H5::DataSpace solCovarSpace = H5::DataSpace( numOfDimensions, dimensionSizes );
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.createDataSet("Solution Covariance", H5::PredType::NATIVE_DOUBLE, solCovarSpace));
    //create Axis labels attribute
    H5::DataSpace AttSpace(H5S_SCALAR);
    for(int i = 0; i < numOfLabels; i++)
    {
        std::ostringstream convert;
        convert << (i);
        std::string columnNum = convert.str();
        const char *attributeLabel = columnNum.c_str();
        H5::Attribute Att = dataset.createAttribute(attributeLabel, STRING, AttSpace);
        std::string labelString = axisLabels[i];
        const char *labelChars = labelString.c_str();
        Att.write(STRING, &labelChars);
        Att.close();
    }
    dataset.write( solCovarBuffer, H5::PredType::NATIVE_DOUBLE );

    delete[] solCovarBuffer;
    solCovarSpace.close();
    dataset.close();
    return 0;
}

int SolutionCovarDataSet::readDataSetFromGroup(H5::Group adjustmentResultsGroup)
{
    H5::DataSet dataset = H5::DataSet(adjustmentResultsGroup.openDataSet("Solution Covariance"));

    //get dataspace from dataset
    H5::DataSpace dataspace = dataset.getSpace();
    hssize_t numberOfRowsInDataSet = sqrt(dataspace.getSimpleExtentNpoints());

    //create read in buffer
    double *readBuffer = new double[numberOfRowsInDataSet*numberOfRowsInDataSet];

    //read file into buffer
    dataset.read( readBuffer, H5::PredType::NATIVE_DOUBLE);

    //read buffer into Matrix
    readToMatrix(readBuffer, numberOfRowsInDataSet);

    //read file attributes
    const char *labelBuffer[1];

    // Preallocating may save some time for axisLabels but probably not much
    for(int i = 0; i < numberOfRowsInDataSet; i++)
    {
        H5::Attribute attribute = dataset.openAttribute(i);
        attribute.read(STRING, labelBuffer);
        axisLabels.push_back(labelBuffer[0]);
        attribute.close();
    }

    delete[] readBuffer;
    dataspace.close();
    dataset.close();

    return 0;
}

//readDataSetFromGroup must be called before this function to work
const std::vector<double> SolutionCovarDataSet::getMatrixRow(int index)
{
    return covarianceMatrix[index];
}

const std::string SolutionCovarDataSet::getLabel(int index)
{
    return axisLabels[index];
}


void SolutionCovarDataSet::writeToBuffer(double *buffer)
{
    //write 2D matrix to 1D buffer
    int CovarYLength = this->getCovarRowCount();
    for(int yIndex = 0; yIndex < CovarYLength; yIndex++)
    {
        for(int xIndex = 0; xIndex < CovarYLength; xIndex++){
            buffer[yIndex*CovarYLength+xIndex] = covarianceMatrix[yIndex][xIndex];
        }
    }
}

void SolutionCovarDataSet::readToMatrix(double *buffer, hssize_t numberOfRowsInMaxtrix)
{
    //read 1D buffer to 2d matrix

    for(int yIndex = 0; yIndex < numberOfRowsInMaxtrix; yIndex++)
    {
        std::vector<double> tempVec;
        for(int xIndex = 0; xIndex < numberOfRowsInMaxtrix; xIndex++)
        {
            tempVec.push_back(buffer[yIndex*numberOfRowsInMaxtrix+xIndex]);
        }
        covarianceMatrix.push_back(tempVec);
    }
}

void LSAH5File::createFile(std::string inputFileName)
{
    //create file
    fileName = inputFileName.c_str();//can I keep fileName as std::string?
    initDataTypes();
}

// ------------------------------------------------------------------------
// Write the H5 file using this object. This routine must strictly parallel Read()
// param filename name of H5 file to be written
// throw on stream error
int LSAH5File::WriteH5File()
{
    try
    {
        /*
         * Turn off the auto-printing when failure occurs so that we can
         * handle the errors appropriately
         */
        H5::Exception::dontPrint();
        writeFile = H5::H5File( fileName.c_str(), H5F_ACC_TRUNC );
        //create file heirarchy
        adjustmentResultsGroup = writeFile.createGroup("/Adjustment Results");
        pointsGroup = writeFile.createGroup("/Adjustment Results/Points");
        measurementsGroup = writeFile.createGroup("/Adjustment Results/Measurements");
        //write File attributes
        int returnStatus = WriteFileAttributes();
        //write Project_Configuration data to H5 file
        projCfg->writeToFile(writeFile);

        //write Post_Processor_Input data to H5 file
        postProcInput->writeToFile(writeFile);

        //write problem_description data to H5 file
        probDescript->writeToGroup(adjustmentResultsGroup);

        //write LLH data to H5 file
        //returnStatus = WriteLlhToH5();
        llh->writeToGroup(pointsGroup);

        //write initialLLH data to H5 file
        initialLlh->writeToGroup(pointsGroup);

        //write XYZ data to H5 file
        xyz->writeToGroup(pointsGroup);

        //write Measurements data to H5 file
        measurements->writeToGroup(measurementsGroup);

        //write Lowest_Redundancy data to H5 file
        lowestRedundancy->writeToGroup(measurementsGroup);

        //write Blunder_Exists data to H5 file
        blundersExist->writeToGroup(measurementsGroup);

        //write Convergence_History data to H5 file
        convergenceHist->writeToGroup(adjustmentResultsGroup);

        //write Solver_Warnings data to H5 file
        solverWarnings->writeToGroup(adjustmentResultsGroup);

        //write Solver_Errors data to H5 file
        solverErrors->writeToGroup(adjustmentResultsGroup);

        //write Solver_Run_Type data to H5 file
        solverRunType->writeToGroup(adjustmentResultsGroup);

        //write Solver_Exit_Type data to H5 file
        solverExitType->writeToGroup(adjustmentResultsGroup);

        //write statistics data to H5 file
        statistics->writeToGroup(adjustmentResultsGroup);

        //write solution_covariance data to H5 file
        solutionCovariance->writeToGroup(adjustmentResultsGroup);
    }
    // catch failure caused by the H5 operations
    catch( H5::Exception error )
    {
        QFileInfo check( QString::fromStdString(fileName) );
        if(!check.exists())
        {
            return -1;
        }
        //is file .h5?
        if( !writeFile.isHdf5(fileName) )
        {
            return -1;
        }
        writeFile.close();
        adjustmentResultsGroup.close();
        pointsGroup.close();
        measurementsGroup.close();
        return -1;
    }
    writeFile.close();
    adjustmentResultsGroup.close();
    pointsGroup.close();
    measurementsGroup.close();
    return 0;
}  // end void LSAH5File::WriteH5File(string& fn)

int LSAH5File::ReadH5File(std::string filename)
{
    try
    {
        /*
         * Turn off the auto-printing when failure occurs so that we can
         * handle the errors appropriately
         */
        H5::Exception::dontPrint();
        //open file
        fileName = filename;//make this a more clean solution before pushing
        //does file exist with name?
        QFileInfo check( QString::fromStdString(fileName) );
        if(!check.exists())
        {
            return -1;
        }
        readFile = H5::H5File(filename.c_str(), H5F_ACC_RDONLY);
    }
    catch(exception)
    {
        return -1;
    }

    try
    {
        /*
         * Turn off the auto-printing when failure occurs so that we can
         * handle the errors appropriately
         */
        H5::Exception::dontPrint();

        //open groups
        adjustmentResultsGroup = readFile.openGroup("/Adjustment Results");
        pointsGroup = readFile.openGroup("/Adjustment Results/Points");
        measurementsGroup = readFile.openGroup("/Adjustment Results/Measurements");
        initDataTypes();
        initContainers();//clear contents from a previous read
        ReadFileAttributes();
        projCfg->readDataSetFromFile(readFile);

        postProcInput->readDataSetFromFile(readFile);

        probDescript->readDataSetFromGroup(adjustmentResultsGroup);

        llh->readDataSetFromGroup(pointsGroup);

        initialLlh->readDataSetFromGroup(pointsGroup);

        xyz->readDataSetFromGroup(pointsGroup);

        measurements->readDataSetFromGroup(measurementsGroup);

        lowestRedundancy->readDataSetFromGroup(measurementsGroup);

        blundersExist->readDataSetFromGroup(measurementsGroup);

        convergenceHist->readDataSetFromGroup(adjustmentResultsGroup);

        solverWarnings->readDataSetFromGroup(adjustmentResultsGroup);

        solverErrors->readDataSetFromGroup(adjustmentResultsGroup);

        statistics->readDataSetFromGroup(adjustmentResultsGroup);

        solverRunType->readDataSetFromGroup(adjustmentResultsGroup);

        solverExitType->readDataSetFromGroup(adjustmentResultsGroup);

        solutionCovariance->readDataSetFromGroup(adjustmentResultsGroup);
    }
    // catch failure caused by the H5 operations
    catch( H5::Exception error )
    {
        //std::string errormsg = error.getDetailMsg();
        readFile.close();
        adjustmentResultsGroup.close();
        pointsGroup.close();
        measurementsGroup.close();
        return -1;
    }
    readFile.close();
    adjustmentResultsGroup.close();
    pointsGroup.close();
    measurementsGroup.close();
    return 0;
}

void LSAH5File::closeH5File()
{
    readFile.close();
    initContainers();
}

int LSAH5File::WriteFileAttributes()
{
    const char* h5VersionWriteBuffer = h5VersionContainer.c_str();
    const char* lsaVersionWriteBuffer = LSAVersionContainer.c_str();
    const char* dateWriteBuffer = dateCreatedContainer.c_str();
    H5::DataSpace attSpace(H5S_SCALAR);

    H5::Attribute h5VersionAttribute = writeFile.createAttribute("H5 Version", STRING, attSpace);
    H5::Attribute lsaVersionAttribute = writeFile.createAttribute("LSA Version", STRING, attSpace);
    H5::Attribute dateCreatedAttribute = writeFile.createAttribute("Date Created", STRING, attSpace);

    h5VersionAttribute.write(STRING, &h5VersionWriteBuffer);
    lsaVersionAttribute.write(STRING, &lsaVersionWriteBuffer);
    dateCreatedAttribute.write(STRING, &dateWriteBuffer);

    h5VersionAttribute.close();
    lsaVersionAttribute.close();
    dateCreatedAttribute.close();
    return 0;
}

int LSAH5File::ReadFileAttributes()
{
    try
    {
        readFile = H5::H5File(fileName.c_str(), H5F_ACC_RDONLY);
        const char* readBuffer[1];
        H5::Attribute h5Attribute = readFile.openAttribute("H5 Version");
        H5::Attribute lsaAttribute = readFile.openAttribute("LSA Version");
        H5::Attribute dateAttribute = readFile.openAttribute("Date Created");

        h5Attribute.read(STRING, readBuffer);
        h5VersionContainer = readBuffer[0];
        lsaAttribute.read(STRING, readBuffer);
        LSAVersionContainer = readBuffer[0];
        dateAttribute.read(STRING, readBuffer);
        dateCreatedContainer = readBuffer[0];

        h5Attribute.close();
        lsaAttribute.close();
        dateAttribute.close();
        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

// ------------------------------------------------------------------------
void LSAH5File::initContainers()
{
    //initialize non-structs and vectors
    LSAVersionContainer = "";
    h5VersionContainer = "";
    dateCreatedContainer = "";
    
    // reset the smart pointers with new objects
    projCfg.reset(new ProjCfgDataSet());
    postProcInput.reset(new PostProcInputDataSet());
    probDescript.reset(new ProbDescriptDataSet());
    llh.reset(new LlhDataSet());
    initialLlh.reset(new InitialLlhDataSet());
    xyz.reset(new XyxDataSet());
    measurements.reset(new MeasurementsDataSet());
    lowestRedundancy.reset(new LowestRedundDataSet());
    blundersExist.reset(new BlundersExistDataSet());
    convergenceHist.reset(new ConvergenceHistDataSet());
    solverWarnings.reset(new SolverWariningsDataSet());
    solverErrors.reset(new SolverErrorsDataSet());
    solverRunType.reset(new SolverRunTypeDataSet());
    solverExitType.reset(new SolverExitTypeDataSet());
    statistics.reset(new StatisticsDataSet());
    solutionCovariance.reset(new SolutionCovarDataSet());
}
