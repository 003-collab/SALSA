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
#include <lsainverse.hpp>
#include "LSAVersion.hpp"

#include <algorithm>

// pragma commands used to squelch useless warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <Eigen/Dense>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

#include "SigPtTransform.hpp"

#include <QString>

const string GlobalDataLsainverse::Version(string("1.0.0 10/17/16")
                          + string(", SALSA Ver. ") + LSAVERSION);

//------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
// 2-digit exponent on windows
#ifdef _WIN32
   #if (_MSC_VER < 1900)//Visual Studio 2015
      _set_output_format(_TWO_DIGIT_EXPONENT);
   #endif
#endif

   string PrgmName;        // for catch
try {
   // begin counting time
   clock_t totaltime = clock();
   Epoch wallbegin,wallend;
   wallbegin.setLocalTime();

   // get (create) the global data object (a singleton);
   // since this is the first instance, this will also set default values
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();
   PrgmName = GD.PrgmName;

   // Build title
   Epoch ttag;
   ttag.setLocalTime();
   GD.Title = GD.PrgmName + ", Ver. "
      + GD.Version + ttag.printf(", Run %04Y/%02m/%02d at %02H:%02M:%02S");

   // display title on screen
   LOG(INFO) << GD.Title;

   // process : loop once --------------------------------------------
   int iret;
   for(bool go=true; go; go=false)  {

      // process the command line ------------------------------------
      iret = getCommandLine(argc,argv);
      if(iret) break;

      // do it
      iret = Process();
      if(iret) break;
   }  // end loop once

   if(iret != LSAINVERSE_OK) {
      if(iret != COMMAND_LINE_USAGE) {
         string msg;
         msg = GD.PrgmName + string(" is terminating with code ")
                           + StringUtils::asString(iret);
         pLOGstrm = &cerr;
         LOG(ERROR) << msg;
      }

      if(iret == COMMAND_LINE_USAGE) { LOG(INFO) << GD.cmdlineUsage; }
      else if(iret == COMMAND_LINE_ERRORS) { LOG(INFO) << GD.cmdlineErrors; }
      else if(iret == USER_VALIDATION) { LOG(INFO) << "The user requested input validation."; }
      else if(iret == INVALID_INPUT) { LOG(INFO) << "The input is invalid."; }
      else if(iret == LOG_FILE_UNOPENED) { LOG(INFO) << "The log file could not be opened."; }
      else if(iret == HDF5_FILE_UNOPENED) { LOG(INFO) << "The hdf5 file could not be opened."; }
      else if(iret == HDF5_READ_FAILURE) { LOG(INFO) << "Unable to read .h5 file."; }
      else if(iret == HDF5_FILE_VERSION) { LOG(INFO) << "Incompatible .h5 file version."; }
      else if(iret == HDF5_NO_DATA) { LOG(INFO) << "No data in .h5 file."; }
      else if(iret == INV_FILE_ERROR) { LOG(INFO) << "Error getting info from .inv file."; }
      else if(iret == COMMAND_LINE_DEF) { // cmd line definition invalid
         LOG(INFO) << "The command line definition is invalid.\n" << GD.cmdlineErrors;
      }
      else if(iret == DATA_OBJECT_UNCREATED) { LOG(INFO) << "The data object could not be created."; }
      else                 // fix this
         LOG(INFO) << "temp - Some other return code..." << iret;
   }

   // compute and print run time ----------------------------------------------
   if(iret != COMMAND_LINE_USAGE) {
      wallend.setLocalTime();
      totaltime = clock()-totaltime;
      ostringstream oss;
      oss << PrgmName << " timing: " << fixed << setprecision(3)
         << double(totaltime)/double(CLOCKS_PER_SEC)
         << " seconds. (" << (wallend - wallbegin) << " sec)";
      if(!GD.logfile.empty())
          pLOGstrm = &GD.oflog;
      else
          pLOGstrm = &cout;
      if(pLOGstrm != &cout) LOG(INFO) << oss.str();
      cout << oss.str() << endl;
   }

   if(iret == LSAINVERSE_OK)
       return LSAINVERSE_OK;
   else
       return LSAINVERSE_ERROR;
}
catch(Exception& e) {
   pLOGstrm = &cerr;
   LOG(ERROR) << "Error - " + PrgmName << " caught Exception:\n" << e.what() << endl;
   // don't use LOG here - causes hangup - don't know why
}
catch (...) {
   pLOGstrm = &cerr;
   LOG(ERROR) <<"Error- unknown error in " << PrgmName << ".  Abort." << endl;
}
   return LSAINVERSE_ERROR;
}   // end main()

//------------------------------------------------------------------------------------
int getCommandLine(int argc, char **argv) throw(Exception)
{
try {
   int i;
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

   // create list of command line options, and fill it
   // put required options first - they will get listed first anyway
   CommandLine opts;

   // build the options list == syntax page
   string PrgmDesc =
      " Program " + GD.PrgmName + " will read a SALSA hdf5 output file (<hdf5>) and\n"
      " the labels of two stations (<from>,<to>).  Output are the relative geodetic\n"
      " quantities of interest (e.g. slant distance, ellipsoidal distance) between the\n"
      " two stations, along with the associated uncertainty values.\n"
      " Accepted options are shown below, followed by a description, with default\n"
      " value, if any, in ().";

   // opts.Add(char, opt, arg, repeat?, required?, &target, pre-descript, descript.);
   // required options
   bool req(true);
   opts.Add(0, "hdf5", "name", false, req, &GD.hdf5FileName, "",
            "Name of input hdf5 file (lsapost output), .h5 extension expected");
   opts.Add(0, "from", "name", false, req, &GD.fromStation, "",
            "Name of 'from' station");
   opts.Add(0, "to", "name", false, req, &GD.toStation, "",
            "Name of 'to' station");
   // optional args
   req = false;
   opts.Add('l', "log", "name", false, req, &GD.logfile, "",
            "Name of output log file");
   opts.Add(0, "logpath", "path", false, req, &GD.logpath, "",
            "Path for output log file");
   opts.Add(0, "hdf5path", "path", false, req, &GD.hdf5path, "",
            "Path for input hdf5 file");
   opts.Add(0, "scaleByAPV", "", false, req, &GD.scaleByAPV, "",
            "If present, scale the sigmas by the APV (default False)");
   opts.Add(0, "westLon", "", false, req, &GD.westLon, "",
            "If present, output longitudes in West direction (default False)");
   opts.Add(0,"GUIOutput", "", false, req, &GD.GUIOutput, "",
            "Forces output to be comma-delimited (default False)");
   //  help
   opts.Add(0, "verbose", "", false, req, &GD.verbose, "",
            "Print extended output information");
   opts.Add(0, "help", "", false, req, &GD.help, "",
            "Print this and quit");

   // --------------------------------------------------------------------------
   // declare it and parse it; write all errors to string GD.cmdlineErrors
   int iret = opts.ProcessCommandLine(argc,argv,PrgmDesc,
                         GD.cmdlineUsage,GD.cmdlineErrors,GD.cmdlineUnrecognized);
   if(iret == BAD_ALLOCATION) return iret;      // bad alloc
   if(iret == COMMAND_LINE_DEF) return iret;      // cmd line definition invalid

   // --------------------------------------------------------------------------
   // do extra parsing -- append errors to GD.cmdlineErrors

   // unrecognized arguments are an error
   if(GD.cmdlineUnrecognized.size() > 0) {
      LOG(WARNING) << " Warning - unrecognized arguments:";
      for(i=0; i<GD.cmdlineUnrecognized.size(); i++)
         LOG(WARNING) << "   " << GD.cmdlineUnrecognized[i];
      LOG(WARNING) << " End of unrecognized arguments.";
   }

   // --------------------------------------------------------------------------
   LOG(DEBUG) << GD.cmdlineUsage;  // this will contain list of args

   // dump it
   ostringstream oss;
   oss << "------ Summary of " << GD.PrgmName
      << " command line configuration --------" << endl;
   opts.DumpConfiguration(oss);
      // perhaps dump the 'extra parsing' things
   oss << "------ End configuration summary --------" << endl;
   GD.cmdlineDump = oss.str();

   if(opts.hasHelp()) return COMMAND_LINE_USAGE;
   if(opts.hasErrors()) return COMMAND_LINE_ERRORS;
   if(!GD.cmdlineErrors.empty()) {
      //LOG(INFO) << "RETURNING invalid b/c of errors:\n" << GD.cmdlineErrors;
      return COMMAND_LINE_ERRORS;
   }

   if(!GD.logfile.empty())
   {
       include_path(GD.logpath,GD.logfile);
       expand_filename(GD.logfile);
       LOG(VERBOSE) << " Found log file name " << GD.logfile;
   }

   if(!GD.hdf5FileName.empty())
   {
       include_path(GD.hdf5path,GD.hdf5FileName);
       expand_filename(GD.hdf5FileName);
       LOG(VERBOSE) << " Found lsapost .hf5 file name " << GD.hdf5FileName;
   }

   // open log file, if it exists
   if(!GD.logfile.empty()) {
      GD.oflog.open(GD.logfile.c_str(),ios_base::app);
      if(!GD.oflog.is_open()) {
         pLOGstrm = &cerr;
         LOG(ERROR) << "Error - failed to open log file " << GD.logfile << "." << endl;
         return LOG_FILE_UNOPENED;
      }
      LOG(INFO) << "Output directed to log file " << GD.logfile;
      pLOGstrm = &GD.oflog; // ConfigureLOG::Stream() = &GD.oflog;
   }

   // configure log stream
   ConfigureLOG::ReportLevels() = false;
   ConfigureLOG::ReportTimeTags() = false;
   if(GD.verbose)
       LOGlevel = ConfigureLOG::Level("VERBOSE");
   if(pLOGstrm != &cout)
       LOG(INFO) << GD.Title;

   LOG(VERBOSE) << GD.cmdlineDump;

   return 0;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int Process(void) throw(Exception)
{
try {
   int iret=0;
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

   //------------------------------------------------------------------------
   // main loop
   for(bool go=true; go; go=!go)
   {
       //Open and read the HDF5 file
       if(!GD.hdf5FileName.empty())
       {
           if((iret = readHDF5File()) != 0)
               break;
       }
       else
       {
           iret = COMMAND_LINE_ERRORS;
           break;
       }

       //Verify that the From/To points exist in the file
       if((iret = validateInput()) != 0)
           break;

       //Convert the solution covariance matrix into a form suitable for extraction
       if((iret = preProcess()) != 0)
           break;

       //Compute the extraction
       if((iret = computeExtraction()) != 0)
           break;

       //Output the extraction results to stdout (or the log file, if specified)
       if((iret = outputResults()) != 0)
       {
           break;
       }

       if(!GD.logpath.empty() && !GD.hdf5FileName.empty())
       {
            iret = writeOutputCSV();
       }

   }  // end main loop
   return iret;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int preProcess(void) throw(Exception)
{
try {
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

   int numPoints = GD.hdf5File.solutionCovariance->getNumOfLabels();
   GD.StateNames.clear();
   for(int i=0; i<numPoints; i++)
       GD.StateNames += GD.hdf5File.solutionCovariance->getLabel(i);

   int numRows = GD.hdf5File.solutionCovariance->getCovarRowCount();
   GD.Covariance.resize(numRows,numRows);
   for(int i=0; i<numRows; i++)
   {
       std::vector<double> row = GD.hdf5File.solutionCovariance->getMatrixRow(i);
       int numCols = row.size();
       for(int j=0; j<numCols; j++)
           GD.Covariance(i,j) = row.at(j);
   }

   int numAllPoints = GD.hdf5File.xyz->getSize();
   GD.Points.clear();
   for(int i=0; i<numAllPoints; i++)
   {
       xyzExternal xyzPoint = GD.hdf5File.xyz->getDataPoint(i);
       Point point(xyzPoint.label,xyzPoint.x,xyzPoint.y,xyzPoint.z,
                   xyzPoint.Cxx,xyzPoint.Cxy,xyzPoint.Cxz,
                   xyzPoint.Cyy,xyzPoint.Cyz,xyzPoint.Czz);
       GD.Points.insert(std::pair<std::string,Point>(xyzPoint.label,point));
   }

   GD.APV = GD.hdf5File.statistics->getDataPoint().APV;

   return LSAINVERSE_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int readHDF5File(void) throw(Exception)
{
try {
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

   QFileInfo check(QString::fromStdString(GD.hdf5FileName));
   if(!check.exists())
   {
       return HDF5_FILE_UNOPENED;
   }
   //check the file version
   GD.hdf5File.createFile(GD.hdf5FileName);
   if(GD.hdf5File.ReadFileAttributes() != 0)
   {
       GD.hdf5File.closeH5File();
       return HDF5_READ_FAILURE;
   }

   //bail if invalid version
   if(GD.hdf5File.h5VersionContainer != LSAH5File::HDF5_FILE_VERSION)
   {
       LOG(INFO) << "Unsupported HDF5 file version! " << GD.hdf5FileName <<
                    " is version " << GD.hdf5File.h5VersionContainer <<
                    " but the currently supported version is " <<
                    LSAH5File::HDF5_FILE_VERSION <<
                    ". Re-calculate adjustement to regenerate a current .h5 file.";
       GD.hdf5File.closeH5File();
       return HDF5_FILE_VERSION;
   }
   GD.hdf5File.closeH5File();

   //read the .h5 file
   if(GD.hdf5File.ReadH5File(GD.hdf5FileName) != 0)
   {
       GD.hdf5File.closeH5File();
       return HDF5_READ_FAILURE;
   }

   //verify that there is valid content in the .h5 file
   enum_binary_file_status binStat = GD.hdf5File.postProcInput->getDataPoint().binFileStatus;
   if((binStat == BIN_FILE_NOT_FOUND) || (binStat == BIN_FILE_NOT_PARSED) ||
      (binStat == BIN_FILE_UNABLE_TO_OPEN) || (binStat == BIN_FILE_FORMAT_OBSOLETE))
   {
       GD.hdf5File.closeH5File();
       return HDF5_NO_DATA;
   }

   bool foundFrom=false, foundTo=false;

//   int numXYZPoints = GD.hdf5File.xyz -> getSize();

   int numAllPoints = GD.hdf5File.llh->getSize();
   for(int i=0; i<numAllPoints; i++)
   {
       llhExternal llhPoint = GD.hdf5File.llh->getDataPoint(i);
       if(llhPoint.label == GD.fromStation)
       {
           GD.fromLLH.label = llhPoint.label;
           GD.fromLLH.lat = llhPoint.lat;
           GD.fromLLH.lon = llhPoint.lon;
           GD.fromLLH.ht = llhPoint.ht;
           GD.fromLLH.oht = llhPoint.oht;
           GD.fromLLH.dovN = llhPoint.dovN;
           GD.fromLLH.dovE = llhPoint.dovE;
           GD.fromLLH.fixedType = llhPoint.fixedType;
           foundFrom = true;
       }
       else if(llhPoint.label == GD.toStation)
       {
           GD.toLLH.label = llhPoint.label;
           GD.toLLH.lat = llhPoint.lat;
           GD.toLLH.lon = llhPoint.lon;
           GD.toLLH.ht = llhPoint.ht;
           GD.toLLH.oht = llhPoint.oht;
           GD.toLLH.dovN = llhPoint.dovN;
           GD.toLLH.dovE = llhPoint.dovE;
           GD.toLLH.fixedType = llhPoint.fixedType;
           foundTo = true;
       }
       if(foundFrom && foundTo)
           break;
   }

   return LSAINVERSE_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int validateInput(void) throw(Exception)
{
try {
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();
   bool foundFrom = false, foundTo = false, fromIsFloatUnused = false, toIsFloatUnused = false;

   if(GD.toStation == GD.fromStation)
   {
       pLOGstrm = &cerr;
       LOG(ERROR) << " Error - 'From' Station \"" << GD.fromStation << "\" and 'To' Station \"" << GD.toStation << "\" are the same.  No measurements extracted.";
       return INVALID_INPUT;
   }

   //verify fromStation and toStation are in the .h5 point list
   int numPoints = GD.hdf5File.llh->getSize();
   for(int i=0; i<numPoints; i++)
   {
       llhExternal point = GD.hdf5File.llh->getDataPoint(i);
       if(point.label == GD.fromStation)
       {
           foundFrom = true;
           if((point.fixedType.asString() == "Floating") && (point.utilized == false))
           {
               LOG(INFO) << " Warning - 'From' Station \"" << GD.fromStation << "\" is floating and unused.  No sigmas available for extracted quantities.";
               fromIsFloatUnused = true;
           }
       }
       if(point.label == GD.toStation)
       {
           foundTo = true;
           if((point.fixedType.asString() == "Floating") && (point.utilized == false))
           {
               LOG(INFO) << " Warning - 'To' Station \"" << GD.toStation << "\" is floating and unused.  No sigmas available for extracted quantities.";
               toIsFloatUnused = true;
           }
       }
   }
   if (!foundFrom)
   {
       pLOGstrm = &cerr;
       LOG(ERROR) << " Error - 'From' Station \"" << GD.fromStation << "\" not found in solution covariance in SALSA hdf5 file " << GD.hdf5FileName << ".";
       return INVALID_INPUT;
   }
   if (!foundTo)
   {
       pLOGstrm = &cerr;
       LOG(ERROR) << " Error - 'To' Station \"" << GD.toStation << "\" not found in solution covariance in SALSA hdf5 file " << GD.hdf5FileName << ".";
       return INVALID_INPUT;
   }

   if(fromIsFloatUnused || toIsFloatUnused)
       GD.stationIsFloatUnused = true;

   return LSAINVERSE_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int computeExtraction(void) throw(Exception)
{
try {
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

   //create extraction record strings for currently supported record types
   std::vector<std::string> extractRecordStrings;
   extractRecordStrings.push_back(std::string("EXTR T0 AZM ") + addQuotes(GD.fromStation) + std::string(" ") + addQuotes(GD.toStation));
   extractRecordStrings.push_back(std::string("EXTR T1 DEL ") + addQuotes(GD.fromStation) + std::string(" ") + addQuotes(GD.toStation));
   extractRecordStrings.push_back(std::string("EXTR T2 DIS ") + addQuotes(GD.fromStation) + std::string(" ") + addQuotes(GD.toStation));
   extractRecordStrings.push_back(std::string("EXTR T3 HGT ") + addQuotes(GD.fromStation) + std::string(" ") + addQuotes(GD.toStation));
   extractRecordStrings.push_back(std::string("EXTR T4 VAN ") + addQuotes(GD.fromStation) + std::string(" ") + addQuotes(GD.toStation));

   string str,blk,msg;
   int k,j;

   // build the Extraction objects, and add to vector
   for(int i=0; i<extractRecordStrings.size(); i++)
   {
      str = extractRecordStrings[i];
      blk = Extraction::getBlockFromString(str,msg);

      // find the Extraction object; add one if necessary
      for(k=-1,j=0; j<GD.Extrs.size(); j++)
         if(blk == GD.Extrs[j].getBlock()) { k=j; break; }

      if(k == -1) {
         Extraction e(blk);
         k = GD.Extrs.size();
         GD.Extrs.push_back(e);
      }

      // add the string
      if(!GD.Extrs[k].Add(str,msg,GD.StateNames)) {
         LOG(INFO) << " Warning - parsing failed for >" << str << "<: " << msg;
         continue;
      }
   }

   // compute for each extraction
   bool is2D = false;
   bool doSOA = true;
   for(int i=0; i<GD.Extrs.size(); i++)
       GD.Extrs[i].Compute(GD.StateNames, GD.Points, GD.Covariance, GD.APV, is2D, !GD.scaleByAPV);

   //extract lsainverse quantities from the extraction output
   ostringstream osstrm;
   for(int i=0; i<GD.Extrs.size(); i++)
       GD.Extrs[i].Output(osstrm,getLinearMeasurementPrecisionMeters(),is2D,doSOA);
   std::string extractStr = osstrm.str();

   double deltaCorrXY, deltaCorrXZ, deltaCorrYZ;
   bool foundDelX=false, foundDelY=false;
   std::vector<std::string> lines = gnsstk::StringUtils::splitWithQuotes(extractStr, '\n');
   for(int i=0; i<lines.size(); i++)
   {
       string line = lines.at(i);
       std::vector<std::string> pieces = gnsstk::StringUtils::splitWithQuotes(line, ' ');
       double numPieces = pieces.size();
       if(numPieces == 0)
           continue;
       string meas = pieces[0];

       if(meas == string("Azm"))
       {
           GD.azimuthDeg = gnsstk::StringUtils::asInt(pieces[numPieces-4]);
           if((GD.azimuthDeg == 0) && (pieces[numPieces-4].substr(0,1) == std::string("-")))
               GD.azimuthIsNeg = true;
           GD.azimuthMin = gnsstk::StringUtils::asInt(pieces[numPieces-3]);
           GD.azimuthSec = gnsstk::StringUtils::asDouble(pieces[numPieces-2]);
           GD.azimuthSigma = gnsstk::StringUtils::asDouble(pieces[numPieces-1]);
       }
       else if(meas == string("Del"))
       {
           if(pieces.size() >= 8 && !foundDelX)
           {
               GD.deltaX = gnsstk::StringUtils::asDouble(pieces[numPieces-6]);
               GD.deltaXSigma = gnsstk::StringUtils::asDouble(pieces[numPieces-4]);
               deltaCorrXY = gnsstk::StringUtils::asDouble(pieces[numPieces-2]);
               deltaCorrXZ = gnsstk::StringUtils::asDouble(pieces[numPieces-1]);
               foundDelX=true;
           }
           else if(pieces.size() >= 7 && foundDelX && !foundDelY)
           {
               GD.deltaY = gnsstk::StringUtils::asDouble(pieces[numPieces-5]);
               GD.deltaYSigma = gnsstk::StringUtils::asDouble(pieces[numPieces-3]);
               deltaCorrYZ = gnsstk::StringUtils::asDouble(pieces[numPieces-1]);
               foundDelY=true;
           }
           else if(pieces.size() >= 6 && foundDelX && foundDelY)
           {
               GD.deltaZ = gnsstk::StringUtils::asDouble(pieces[numPieces-4]);
               GD.deltaZSigma = gnsstk::StringUtils::asDouble(pieces[numPieces-2]);
           }
       }
       else if(meas == string("Dis"))
       {
           GD.slantDistance = gnsstk::StringUtils::asDouble(pieces[numPieces-3]);
           GD.slantDistanceSigma = gnsstk::StringUtils::asDouble(pieces[numPieces-1]);
       }
       else if(meas == string("Hgt"))
       {
           GD.heightDiff = gnsstk::StringUtils::asDouble(pieces[numPieces-2]);
           GD.heightDiffSigma = gnsstk::StringUtils::asDouble(pieces[numPieces-1]);
       }
       else if(meas == string("Van"))
       {
           GD.verticalAngleDeg = gnsstk::StringUtils::asInt(pieces[numPieces-4]);
           if((GD.verticalAngleDeg == 0) && (pieces[numPieces-4].substr(0,1) == std::string("-")))
               GD.verticalAngleIsNeg = true;
           GD.verticalAngleMin = gnsstk::StringUtils::asInt(pieces[numPieces-3]);
           GD.verticalAngleSec = gnsstk::StringUtils::asDouble(pieces[numPieces-2]);
           GD.verticalAngleSigma = gnsstk::StringUtils::asDouble(pieces[numPieces-1]);
       }
   }

   //get covariance matrix from sigmas and correlation matrix
   GD.deltaCovXX = GD.deltaXSigma * GD.deltaXSigma;
   GD.deltaCovXY = GD.deltaXSigma * GD.deltaYSigma * deltaCorrXY;
   GD.deltaCovXZ = GD.deltaXSigma * GD.deltaZSigma * deltaCorrXZ;
   GD.deltaCovYY = GD.deltaYSigma * GD.deltaYSigma;
   GD.deltaCovYZ = GD.deltaYSigma * GD.deltaZSigma * deltaCorrYZ;
   GD.deltaCovZZ = GD.deltaZSigma * GD.deltaZSigma;

   // New method to compute slant distance and associated uncertainty
   // Not employed for now, but likely useful in the future, especially if needed for
   // other types of station inverse quantities - good template
   bool UseSigPtTransformForSlantDistSigma = false;
   if (UseSigPtTransformForSlantDistSigma == true)
       computeSlantDistanceAndSigma();

   //Produce the derived values that do not have corresponding DAT record types
   //(geodesic distance and horizontal distance)
   computeHorizontalDistance();
   computeGeodesicDistance();

   return LSAINVERSE_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
void computeSlantDistanceAndSigma(void) throw(Exception)
{
try {
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

   // Extract the combined to and from station ECEF covariance (including cross cov terms)
   gnsstk::Matrix<double> CovXYZfromAndto(6,6);  //= {0.0};
   CovXYZfromAndto = ExtractFromAndToStationCov();

   // Extract to and from positions in ECEF XYZ coordinates
   Eigen::VectorXd fromAndtoXYZ(6);
   // Extract the from station ECEF XYZ coordinates - NOTE: GD.fromLLH.lat, lon in degrees!
   // Fix to Bug #1379 - want @ ht=0
   gnsstk::Position fromPosition(GD.fromLLH.lat, GD.fromLLH.lon, GD.fromLLH.ht, gnsstk::Position::Geodetic);
   // Extract the to station ECEF XYZ coordinates - NOTE: GD.fromLLH.lat, lon in degrees!
   // Fix to Bug #1379 - want @ ht=0
   gnsstk::Position toPosition(GD.toLLH.lat, GD.toLLH.lon, GD.toLLH.ht, gnsstk::Position::Geodetic);
   fromAndtoXYZ(0) = fromPosition.getX();
   fromAndtoXYZ(1) = fromPosition.getY();
   fromAndtoXYZ(2) = fromPosition.getZ();
   fromAndtoXYZ(3) = toPosition.getX();
   fromAndtoXYZ(4) = toPosition.getY();
   fromAndtoXYZ(5) = toPosition.getZ();

   // Compute the slant distance between station 1 and station 2
   Eigen::VectorXd slantDistance = computeSlantDistance(fromAndtoXYZ);
   GD.slantDistance = slantDistance(0);

   // Scale by the APV if needed
   if(GD.scaleByAPV)//Fix to Bug #1365
      CovXYZfromAndto *= GD.APV;

   // Convert CovXYZfromAndto to Eigen matrix
   Eigen::MatrixXd CovXYZfromAndtoEigen = castMatrixgnsstktoEigen(CovXYZfromAndto);

   // Map the combined covariance of both station positions into an uncertainty sigma
   // for the geodesic distance
   // Employ Sigma Point Transform function to map the CovXYZfromAndtoEigen covariance to the
   // variance of the geodesic distance, providing the mapping function computeGeodesic
   Eigen::MatrixXd MappedCov = SigmaPointTransformFunc(fromAndtoXYZ,CovXYZfromAndtoEigen,
                                                        computeSlantDistance);

   GD.slantDistanceSigma = std::sqrt(MappedCov(0,0));

}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

Eigen::VectorXd computeSlantDistance(Eigen::VectorXd Pos1and2) throw(Exception)
{
    // Calculate Delta Vector
    Eigen::VectorXd Delta(3);
    Delta(0) = Pos1and2(3) - Pos1and2(0);
    Delta(1) = Pos1and2(4) - Pos1and2(1);
    Delta(2) = Pos1and2(5) - Pos1and2(2);

    Eigen::VectorXd SlantDistOutput(1);
    SlantDistOutput(0) = std::sqrt(std::pow(Delta(0), 2.0) + std::pow(Delta(1), 2.0) +
                                   std::pow(Delta(2), 2.0));
    return SlantDistOutput;
}



//------------------------------------------------------------------------------------
void computeGeodesicDistance(void) throw(Exception)
{
try {
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

   // Declare needed variables
   gnsstk::Matrix<double> CovXYZfromAndto(6,6);  //= {0.0};
   gnsstk::Vector<double> fromXYZ(3), toXYZ(3);

   // Extract the from station ECEF XYZ coordinates - NOTE: GD.fromLLH.lat, lon in degrees!
   // Fix to Bug #1379 - want @ ht=0
   gnsstk::Position fromPosition(GD.fromLLH.lat, GD.fromLLH.lon, 0.0, gnsstk::Position::Geodetic);
   fromXYZ(0) = fromPosition.getX();
   fromXYZ(1) = fromPosition.getY();
   fromXYZ(2) = fromPosition.getZ();

   // Extract the to station ECEF XYZ coordinates - NOTE: GD.fromLLH.lat, lon in degrees!
   // Fix to Bug #1379 - want @ ht=0
   gnsstk::Position toPosition(GD.toLLH.lat, GD.toLLH.lon, 0.0, gnsstk::Position::Geodetic);
   toXYZ(0) = toPosition.getX();
   toXYZ(1) = toPosition.getY();
   toXYZ(2) = toPosition.getZ();

   // Extract the combined to and from station ECEF covariance (including cross cov terms)
   CovXYZfromAndto = ExtractFromAndToStationCov();

   // Compute the geodesic distance between station 1 and station 2
   Eigen::VectorXd fromAndtoXYZ(6);
   fromAndtoXYZ(0) = fromXYZ(0); fromAndtoXYZ(1) = fromXYZ(1); fromAndtoXYZ(2) = fromXYZ(2);
   fromAndtoXYZ(3) = toXYZ(0); fromAndtoXYZ(4) = toXYZ(1); fromAndtoXYZ(5) = toXYZ(2);
   Eigen::VectorXd geodesicDistance = computeGeodesic(fromAndtoXYZ);
   GD.geodesicDistance = geodesicDistance(0);

   // Map the combined covariance of both station positions into an uncertainty sigma
   // for the geodesic distance
   if(GD.scaleByAPV)//Fix to Bug #1365
      CovXYZfromAndto *= GD.APV;
   GD.geodesicDistanceSigma = MapECEFcovToGeodesicSigma(fromXYZ,toXYZ,CovXYZfromAndto);

}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
Eigen::VectorXd computeGeodesic(Eigen::VectorXd Pos1and2) throw(Exception)
{
try {
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

   double GeodesicDist;
   Eigen::VectorXd GeodesicDistOutput(1);

   // Extract the WGS-84 flattening parameter
   //double flattening = gnsstk::EllipsoidModel.flattening();
   gnsstk::WGS84Ellipsoid ellipse;
   double flattening = ellipse.flattening();
   double SMA = ellipse.a();// Fix to Bug #1379 - want path along surface of the ellipsoid

   // Convert the 'from' and 'to' station positions from ECEF cartesian to geodetic lat/lon/height
   gnsstk::Position fromPosition(Pos1and2(0),Pos1and2(1),Pos1and2(2), gnsstk::Position::Cartesian);
   gnsstk::Position toPosition(Pos1and2(3),Pos1and2(4),Pos1and2(5), gnsstk::Position::Cartesian);

   // NOTE: these values will be in degrees!
   double lat1 = fromPosition.getGeodeticLatitude();
   double lon1 = fromPosition.getLongitude();
   double lat2 = toPosition.getGeodeticLatitude();
   double lon2 = toPosition.getLongitude();

   // Convert from degrees to radians
   lat1 = lat1*::DEG_TO_RAD;
   lon1 = lon1*::DEG_TO_RAD;
   lat2 = lat2*::DEG_TO_RAD;
   lon2 = lon2*::DEG_TO_RAD;

   // employ Vincenty's method
   // Compute reduced latitude (latitude on auxiliary sphere)
   // U1 = atan((1 - f) * tan(LatLon1[0]))  # zero-based indexing!!!!
   // U2 = atan((1 - f) * tan(LatLon2[0]))
   double U1 = std::atan( (1-flattening) * std::tan(lat1) );
   double U2 = std::atan( (1-flattening) * std::tan(lat2) );

   // LonDiff = LatLon2[1] - LatLon1[1]  # zero-based indexing!!!!
   double LonDiff = lon2 - lon1;

   // iterate until lambda converges
   int numIter = 0;
   int numIterLimit = 100;
   double lambdaVar = LonDiff;  // initialize lambda to the longitude diff
   double lambdaDiffBnIter = 1; // initialize to start while loop
   // declare variables prior to while loop, as some are needed after
   double sinsigma, cossigma, sigma, sinalpha, cosalphaSq, cos2sigmam, C, lambdaVarNew;

   // while (abs(lambdaDiffBnIter) > 1e-12):
   while( (std::abs(lambdaDiffBnIter) > 1.0e-12) && (numIter <= numIterLimit) ) {
       numIter = numIter + 1;
       // print('Iteration Number: ', numIter)
       //cout << "Iteration Number: " << numIter << endl;//turning this off because it breaks the lsaInverse gui test -- Don

       // sinsigma = sqrt((cos(U2) * sin(lambdaVar)) ** 2 + (cos(U1) * sin(U2) - sin(U1) * cos(U2) * cos(lambdaVar)) ** 2)
       sinsigma = std::sqrt(std::pow(std::cos(U2)*std::sin(lambdaVar),2.0) +
                                   std::pow(std::cos(U1) * std::sin(U2) - std::sin(U1) * std::cos(U2) * std::cos(lambdaVar),2.0));

       // cossigma = sin(U1) * sin(U2) + cos(U1) * cos(U2) * cos(lambdaVar)
       cossigma = std::sin(U1) * std::sin(U2) + std::cos(U1) * std::cos(U2) * std::cos(lambdaVar);


       // if cossigma is 1, and sinsigma is 0, then the points are concident
       if ( (sinsigma == 0) && (cossigma == 1) ) {
           GeodesicDistOutput(0) = 0;
           return GeodesicDistOutput;
       }

       // if cossigma is -1, then the points are antipodes
       if (cossigma == -1) {
           // display warning:
           LOG(INFO) << " Warning - geodesic distance computation halted, Vincenty's formula does not converge for antipodes. Output set to 0.";
           GeodesicDistOutput(0) = 0;
           return GeodesicDistOutput;
       }

       // sigma = atan2(sinsigma, cossigma)
       sigma = std::atan2(sinsigma, cossigma);

       // sinalpha = cos(U1) * cos(U2) * sin(lambdaVar) / sinsigma
       sinalpha = std::cos(U1) * std::cos(U2) * std::sin(lambdaVar) / sinsigma;

       // cosalphaSq = 1 - sinalpha ** 2
       cosalphaSq = 1 - std::pow(sinalpha,2.0);

       // cos2sigmam = cossigma - 2 * sin(U1) * sin(U2) / cosalphaSq
       // if cosalphaSq = 0, then it's an equatorial line, and cos2sigmam must be set = 0
       if (cosalphaSq != 0) {
           cos2sigmam = cossigma - 2 * std::sin(U1) * std::sin(U2) / cosalphaSq;
       }
       else {
           cos2sigmam = 0; // doesn't matter what the value is, as long as not NaN
       }

       // C = f / 16 * cosalphaSq * (4 + f * (4 - 3 * cosalphaSq))
       C = flattening / 16 * cosalphaSq * (4 + flattening * (4 - 3 * cosalphaSq));

       // lambdaVarNew = LonDiff + (1 - C) * f * sinalpha * (sigma + C * sinsigma *
       //                (cos2sigmam + C * cossigma * (-1 + 2 * cos2sigmam ** 2)))
       lambdaVarNew = LonDiff + (1 - C) * flattening * sinalpha * (sigma + C *
                             sinsigma * (cos2sigmam + C * cossigma * (-1 + 2 *
                             std::pow(cos2sigmam,2.0))));

       // lambdaDiffBnIter = lambdaVarNew - lambdaVar
       lambdaDiffBnIter = lambdaVarNew - lambdaVar;

       // Set lambdaVar = lambdaVarNew for next iteration
       lambdaVar = lambdaVarNew;

   }
   // Output warning if number of itererations exceeded without convergence
   if (numIter >= numIterLimit) {
       // Output warning that process did not converge
       LOG(INFO) << " Warning - geodesic distance computation did not converge (points may be antipodes). Output set to 0.";
       GeodesicDistOutput(0) = 0;
       return GeodesicDistOutput;
   }


   // after lambda has converged, compute geodesic distance
   // b = (1 - f) * a
   double b = (1 - flattening) * SMA;

   // u2 = cosalphaSq * (a ** 2 - b ** 2) / b ** 2
   double u2 = cosalphaSq * (std::pow(SMA,2) - std::pow(b,2)) / std::pow(b,2);

   // k1 = (sqrt(1 + u2) - 1) / (sqrt(1 + u2) + 1)
   double k1 = (std::sqrt(1 + u2) - 1) / (std::sqrt(1 + u2) + 1);

   // A = (1 + 0.25 * k1 ** 2) / (1 - k1)
   double A = (1 + 0.25 * std::pow(k1,2)) / (1 - k1);

   // B = k1 * (1 - (3 / 8) * k1 ** 2)
   double B = k1 * (1 - (3 / 8) * std::pow(k1,2));


   // Deltasigma = B * sinsigma * (cos2sigmam + 0.25 * B * (cossigma * (-1 + 2 *
   //              cos2sigmam ** 2) - (1 / 6) * B * cos2sigmam * (-3 + 4 *
   //              sinsigma ** 2) * (-3 + 4 * cos2sigmam ** 2)))
   double Deltasigma = B * sinsigma * (cos2sigmam + 0.25 * B * (cossigma * (-1 + 2 *
                       std::pow(cos2sigmam,2)) - (1 / 6) * B * cos2sigmam * (-3 + 4 *
                       std::pow(sinsigma,2)) * (-3 + 4 * std::pow(cos2sigmam,2))));

   // GeodesicDist = b * A * (sigma - Deltasigma)
   GeodesicDist = b * A * (sigma - Deltasigma);

   GeodesicDistOutput(0) = GeodesicDist;

   return GeodesicDistOutput;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
gnsstk::Matrix<double> ExtractFromAndToStationCov(void) throw(Exception)
{
try {
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

   gnsstk::Matrix<double> CovXYZfromAndto(6,6);
   bool fromIsPosSemiDef=false, toIsPosSemiDef=false;

   // determine how many dimensions the covariance has (3x the # of floating stations)

   int numCovDim = GD.hdf5File.solutionCovariance->getNumOfLabels();

   // declare fromIndex, toIndex - needed in multiple for loops
   int fromIndex = -1, toIndex = -1;

   // determine if 'from' and 'to' stations are part of solution covariance, and if so, the
   // index of the x component for each
   // if station is floating and 'Utilized' = True, will be part of solution cov
   // if station is floating and 'Utilized' = False, will NOT be part of solution cov - assume cov = 0
   // if station is constrained (which allows a priori cov) and 'Utilized' = False, station should
   // still be part of the solution covariance, and have the same cov as a priori - use that cov

   for(int i=0; i<numCovDim; i++) {
       // extract label
       std::string label = GD.hdf5File.solutionCovariance->getLabel(i);
       // remove the x, y, or z value at the end
       std::string labelNoComp = label.substr(0,label.size()-1);
       // if the label matches the from station, then this is the x-axis component index of the
       // the from station
       if (labelNoComp == GD.fromStation) {
           fromIndex = i;
           break;
       }
   }
   for(int i=0; i<numCovDim; i++) {
       // extract label
       std::string label = GD.hdf5File.solutionCovariance->getLabel(i);
       // remove the x, y, or z value at the end
       std::string labelNoComp = label.substr(0,label.size()-1);
       // if the label matches the to station, then this is the x-axis component index of the
       // the to station
       if (labelNoComp == GD.toStation) {
           toIndex = i;
           break;
       }
   }

   // Extract "From" point 3x3 covariance:
   // If the station is part of the solution cov:
   if (fromIndex > -1) {
       // Pull the 3x3 covariance associated with that index into combined cov matrix
       for(int i=0; i<3; i++) { //rows
           for(int j=0; j<3; j++) { //columns
               CovXYZfromAndto(i,j) = GD.Covariance(fromIndex+i,fromIndex+j);
           }
       }
   }
   else {
       // set the cov values directly associated with the station to zero
       for(int i=0; i<3; i++) { //rows
           for(int j=0; j<3; j++) { //columns
               CovXYZfromAndto(i,j) = 0;
           }
       }
   }

   // Extract "To" point 3x3 covariance:
   // If the station is part of the solution cov:
   if (toIndex > -1) {
       // Pull the 3x3 covariance associated with that index into combined cov matrix
       for(int i=0; i<3; i++) { //rows
           for(int j=0; j<3; j++) { //columns
               CovXYZfromAndto(i+3,j+3) = GD.Covariance(toIndex+i,toIndex+j);
           }
       }
   }
   else {
       // set the cov values directly associated with the station to zero
       for(int i=0; i<3; i++) { //rows
           for(int j=0; j<3; j++) { //columns
               CovXYZfromAndto(i+3,j+3) = 0;
           }
       }
   }

   // If both From and To stations are part of solution cov, capture cross covariance values
   if ( (fromIndex > -1) && (toIndex > -1) ) {
      for(int i=0; i<3; i++) { //rows
         for(int j=0; j<3; j++) { //columns
             CovXYZfromAndto(i,j+3) = GD.Covariance(fromIndex+i,toIndex+j);
             CovXYZfromAndto(i+3,j) = GD.Covariance(toIndex+i,fromIndex+j);
         }
      }
   }
   else{
      // else set all cross covariance values to zero
      for(int i=0; i<3; i++) { //rows
         for(int j=0; j<3; j++) { //columns
            CovXYZfromAndto(i,j+3) = 0;
            CovXYZfromAndto(i+3,j) = 0;
         }
      }
   }

   return CovXYZfromAndto;

}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
double MapECEFcovToGeodesicSigma(gnsstk::Vector<double> Pos1,gnsstk::Vector<double> Pos2,
                                 gnsstk::Matrix<double> CovXYZfromAndto) // throw(Exception)
{
try {

   double GeodesicSigma;

   // dimension of state
   int n = CovXYZfromAndto.rows(); // or CovXYZfromAndto.cols();
   // or int n = Pos1.size() + Pos2.size();

   // Convert CovXYZfromAndto to Eigen matrix
   Eigen::MatrixXd CovXYZfromAndtoEigen = castMatrixgnsstktoEigen(CovXYZfromAndto);

   // Convert Pos1 and Pos2 to a single Eigen vector
   Eigen::VectorXd Pos1and2(n);
   for(int ct=0; ct<n/2; ct++) { //rows
       Pos1and2(ct) = Pos1(ct);
       Pos1and2(ct+n/2) = Pos2(ct);
   }

   // Employ Sigma Point Transform function to map the CovXYZfromAndtoEigen covariance to the
   // variance of the geodesic distance, providing the mapping function computeGeodesic
   Eigen::MatrixXd MappedCov = SigmaPointTransformFunc(Pos1and2,CovXYZfromAndtoEigen,computeGeodesic);

   GeodesicSigma = std::sqrt(MappedCov(0,0));

   return GeodesicSigma;

}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int outputResults(void) throw(Exception)
{
try {
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

   formatOutput();

   LOG(INFO) <<  "\n";
   LOG(INFO) <<  GD.outputCombined.fromStationName;
   LOG(INFO) <<  GD.outputCombined.fromGeodeticLatitudeDMS;
   LOG(INFO) <<  GD.outputCombined.fromGeodeticLongitudeDMS;
   LOG(INFO) <<  GD.outputCombined.fromEllipsoidalHeight;
   LOG(INFO) <<  GD.outputCombined.fromOrthometricHeight;
   LOG(INFO) <<  GD.outputCombined.fromUndulation;
   LOG(INFO) <<  GD.outputCombined.fromDoVN;
   LOG(INFO) <<  GD.outputCombined.fromDoVE;
   LOG(INFO) <<  "";
   LOG(INFO) <<  GD.outputCombined.toStationName;
   LOG(INFO) <<  GD.outputCombined.toGeodeticLatitudeDMS;
   //to Station value
   LOG(INFO) <<  GD.outputCombined.toGeodeticLongitudeDMS;
   LOG(INFO) <<  GD.outputCombined.toEllipsoidalHeight;
   LOG(INFO) <<  GD.outputCombined.toOrthometricHeight;
   LOG(INFO) <<  GD.outputCombined.toUndulation;
   LOG(INFO) <<  GD.outputCombined.toDoVN;
   LOG(INFO) <<  GD.outputCombined.toDoVE;
   LOG(INFO) <<  "";
   LOG(INFO) << "EXTRACTED MEASUREMENTS (value, (standard deviation)):";
   LOG(INFO) <<  GD.outputCombined.slantDistance;
   LOG(INFO) <<  GD.outputCombined.geodesicDistance;
   LOG(INFO) <<  GD.outputCombined.horizontalDistance;
   LOG(INFO) <<  GD.outputCombined.azimuth;
   LOG(INFO) <<  GD.outputCombined.verticalAngle;
   LOG(INFO) <<  GD.outputCombined.heightDifference;
   LOG(INFO) <<  GD.outputCombined.dXdYdZ;
   LOG(INFO) <<  GD.outputCombined.deltaCovariance;
   LOG(INFO) <<  "\n";

   return LSAINVERSE_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

void computeHorizontalDistance(void) throw(Exception)
{
try
{
    GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

    //horz_distance = sqrt(delta_east^2 + delta_north^2)

    gnsstk::Matrix<double> CovXYZfromAndto(6, 6);
    gnsstk::Vector<double> fromXYZ = generateViaPosition(gnsstk::Position(GD.fromLLH.lat, GD.fromLLH.lon,
                                                                        GD.fromLLH.ht, gnsstk::Position::Geodetic));
    gnsstk::Vector<double> toXYZ = generateViaPosition(gnsstk::Position(GD.toLLH.lat, GD.toLLH.lon,
                                                                        GD.toLLH.ht, gnsstk::Position::Geodetic));

    CovXYZfromAndto = ExtractFromAndToStationCov();

    // Create augmented vector
    Eigen::VectorXd fromAndtoXYZ(6);
    for(unsigned int index = 0; index < 3; index++)
    {
        fromAndtoXYZ(index) = fromXYZ(index);
        fromAndtoXYZ(index + 3) = toXYZ(index);
    }

    // Compute the horizontal distance between station 1 and station 2
    Eigen::VectorXd horizontalDistance = computeHorizontal(fromAndtoXYZ);
    GD.horzDistance = horizontalDistance(0);

    // Fix to Bug #1365
    if(GD.scaleByAPV)
    {
        CovXYZfromAndto *= GD.APV;
    }

    Eigen::MatrixXd castCovariance = castMatrixgnsstktoEigen(CovXYZfromAndto);
    Eigen::MatrixXd MappedCov = SigmaPointTransformFunc(fromAndtoXYZ, castCovariance, computeHorizontal);
    GD.horzDistanceSigma = std::sqrt(MappedCov(0,0));
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

void formatOutput(void) throw(Exception)
{
try {
   GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

   char obuff[300] = "", dir;
   char headerBuff[500] = "";
   bool is_neg=false;
   int Deg=0, Min=0;
   double Sec=0.0, lon=0.0;

   // ** FROM STATION: always the same **
   std::sprintf(headerBuff, "FROM STATION");
   GD.outputLabels.fromStationName = string(headerBuff);
   std::sprintf(obuff,"%s",GD.fromStation.c_str());
   GD.outputValues.fromStationName = string(obuff);
   GD.outputCombined.fromStationName = GD.outputLabels.fromStationName + ": " + GD.outputValues.fromStationName;

   //formatted FROM LATITUDE
   degToDMS(GD.fromLLH.lat, is_neg, Deg, Min, Sec);

   std::sprintf(headerBuff, "Geodetic Latitude (DMS): N");
   if(is_neg)
   {
       std::sprintf(obuff,"-%d %d %.*f",Deg,Min,getAngularPositionPrecisionSOA(),Sec);
   }

   else
   {
       std::sprintf(obuff,"%d %d %.*f",Deg,Min,getAngularPositionPrecisionSOA(),Sec);
   }

   GD.outputLabels.fromGeodeticLatitudeDMS = string(headerBuff);
   GD.outputValues.fromGeodeticLatitudeDMS = string(obuff);
   GD.outputCombined.fromGeodeticLatitudeDMS = GD.outputLabels.fromGeodeticLatitudeDMS + " " + GD.outputValues.fromGeodeticLatitudeDMS;

   //formatted FROM LONGITUDE
   if(GD.westLon)
   {
       if(::fabs(GD.fromLLH.lon)<lsa::ZERO_BOUND)//fix to Bug #1301
           lon = 0.0;
       else
       {
           lon = 360.0 - GD.fromLLH.lon;//convert from east lon to west lon
           if(lon > 360.0) lon -= 360.0;
       }
       dir = 'W';
   }
   else
   {
       lon = GD.fromLLH.lon;
       dir = 'E';
   }
   degToDMS(lon, is_neg, Deg, Min, Sec);

   std::sprintf(headerBuff, "Geodetic Longitude (DMS): %c", dir);
   if(is_neg)
   {
       std::sprintf(obuff,"-%d %d %.*f",Deg,Min,getAngularPositionPrecisionSOA(),Sec);
   }
   else
   {
       std::sprintf(obuff,"%d %d %.*f",Deg,Min,getAngularPositionPrecisionSOA(),Sec);
   }

   GD.outputLabels.fromGeodeticLongitudeDMS = string(headerBuff);
   GD.outputValues.fromGeodeticLongitudeDMS = string(obuff);
   GD.outputCombined.fromGeodeticLongitudeDMS = GD.outputLabels.fromGeodeticLongitudeDMS + " " + GD.outputValues.fromGeodeticLongitudeDMS;

   //formatted TO ELLIPSOIDAL HEIGHT
   std::sprintf(headerBuff, "Ellipsoidal Height (m)");
   GD.outputLabels.fromEllipsoidalHeight = string(headerBuff);
   std::sprintf(obuff,"%.*f",getLinearMeasurementPrecisionMeters(),GD.fromLLH.ht);
   GD.outputValues.fromEllipsoidalHeight = string(obuff);
   GD.outputCombined.fromEllipsoidalHeight = GD.outputLabels.fromEllipsoidalHeight + ": " + GD.outputValues.fromEllipsoidalHeight;

   //formatted TO ORTHOMETRIC HEIGHT
   std::sprintf(headerBuff, "Orthometric Height (m)");
   GD.outputLabels.fromOrthometricHeight = string(headerBuff);
   std::sprintf(obuff,"%.*f",getLinearMeasurementPrecisionMeters(),GD.fromLLH.oht);
   GD.outputValues.fromOrthometricHeight = string(obuff);
   GD.outputCombined.fromOrthometricHeight = GD.outputLabels.fromOrthometricHeight + ": " + GD.outputValues.fromOrthometricHeight;

   //formatted TO UNDULATION
   std::sprintf(headerBuff, "Undulation (m)");
   GD.outputLabels.fromUndulation = string(headerBuff);
   std::sprintf(obuff,"%.*f",getLinearMeasurementPrecisionMeters(),GD.fromLLH.ht - GD.fromLLH.oht);
   GD.outputValues.fromUndulation = string(obuff);
   GD.outputCombined.fromUndulation = GD.outputLabels.fromUndulation + ": " + GD.outputValues.fromUndulation;

   //formatted TO N DEFLECTION
   std::sprintf(headerBuff, "N Deflection (soa)");
   GD.outputLabels.fromDoVN = string(headerBuff);
   std::sprintf(obuff,"%.*f",getAngularMeasurementPrecisionSOA(),GD.fromLLH.dovN);
   GD.outputValues.fromDoVN = string(obuff);
   GD.outputCombined.fromDoVN = GD.outputLabels.fromDoVN + ": " + GD.outputValues.fromDoVN;

   //formatted TO E DEFLECTION
   std::sprintf(headerBuff, "E Deflection (soa)");
   GD.outputLabels.fromDoVE = string(headerBuff);
   std::sprintf(obuff,"%.*f",getAngularMeasurementPrecisionSOA(),GD.fromLLH.dovE);
   GD.outputValues.fromDoVE = string(obuff);
   GD.outputCombined.fromDoVE = GD.outputLabels.fromDoVE + ": " + GD.outputValues.fromDoVE;

   //TO STATION: always the same
   std::sprintf(headerBuff, "TO STATION");
   GD.outputLabels.toStationName = string(headerBuff);
   std::sprintf(obuff,"%s",GD.toStation.c_str());
   GD.outputValues.toStationName = string(obuff);
   GD.outputCombined.toStationName = GD.outputLabels.toStationName + ": " + GD.outputValues.toStationName;

   degToDMS(GD.toLLH.lat, is_neg, Deg, Min, Sec);

   std::sprintf(headerBuff, "Geodetic Latitude (DMS): N");
   if(is_neg)
   {
       std::sprintf(obuff,"-%d %d %.*f",Deg,Min,getAngularPositionPrecisionSOA(),Sec);
   }
   else
   {
       std::sprintf(obuff,"%d %d %.*f",Deg,Min,getAngularPositionPrecisionSOA(),Sec);
   }

   GD.outputLabels.toGeodeticLatitudeDMS = string(headerBuff);
   GD.outputValues.toGeodeticLatitudeDMS = string(obuff);
   GD.outputCombined.toGeodeticLatitudeDMS = GD.outputLabels.toGeodeticLatitudeDMS + " " + GD.outputValues.toGeodeticLatitudeDMS;

   //fomatted TO LONGITUDE
   if(GD.westLon)
   {
       if(::fabs(GD.toLLH.lon)<lsa::ZERO_BOUND)//fix to Bug #1301
           lon = 0.0;
       else
       {
           lon = 360.0 - GD.toLLH.lon;//convert from east lon to west lon
           if(lon > 360.0) lon -= 360.0;
       }
       dir = 'W';
   }
   else
   {
       lon = GD.toLLH.lon;
       dir = 'E';
   }
   degToDMS(lon, is_neg, Deg, Min, Sec);

   std::sprintf(headerBuff, "Geodetic Longitude (DMS): %c", dir);
   if(is_neg)
   {
       std::sprintf(obuff,"-%d %d %.*f",Deg,Min,getAngularPositionPrecisionSOA(),Sec);
   }
   else
   {
       std::sprintf(obuff,"%d %d %.*f",Deg,Min,getAngularPositionPrecisionSOA(),Sec);
   }

   GD.outputLabels.toGeodeticLongitudeDMS = string(headerBuff);
   GD.outputValues.toGeodeticLongitudeDMS = string(obuff);
   GD.outputCombined.toGeodeticLongitudeDMS = GD.outputLabels.toGeodeticLongitudeDMS + " " + GD.outputValues.toGeodeticLongitudeDMS;

   //formatted TO ELLIPSOIDAL HEIGHT
   std::sprintf(headerBuff, "Ellipsoidal Height (m)");
   GD.outputLabels.toEllipsoidalHeight = string(headerBuff);
   std::sprintf(obuff,"%.*f",getLinearMeasurementPrecisionMeters(),GD.toLLH.ht);
   GD.outputValues.toEllipsoidalHeight = string(obuff);
   GD.outputCombined.toEllipsoidalHeight = GD.outputLabels.toEllipsoidalHeight + ": " + GD.outputValues.toEllipsoidalHeight;

   //formatted TO ORTHOMETRIC HEIGHT
   std::sprintf(headerBuff, "Orthometric Height (m)");
   GD.outputLabels.toOrthometricHeight = string(headerBuff);
   std::sprintf(obuff,"%.*f",getLinearMeasurementPrecisionMeters(),GD.toLLH.oht);
   GD.outputValues.toOrthometricHeight = string(obuff);
   GD.outputCombined.toOrthometricHeight = GD.outputLabels.toOrthometricHeight + ": " + GD.outputValues.toOrthometricHeight;

   //formatted TO UNDULATION
   std::sprintf(headerBuff, "Undulation (m)");
   GD.outputLabels.toUndulation = string(headerBuff);
   std::sprintf(obuff,"%.*f",getLinearMeasurementPrecisionMeters(),GD.toLLH.ht - GD.toLLH.oht);
   GD.outputValues.toUndulation = string(obuff);
   GD.outputCombined.toUndulation = GD.outputLabels.toUndulation + ": " + GD.outputValues.toUndulation;

   //formatted TO N DEFLECTION
   std::sprintf(headerBuff, "N Deflection (soa)");
   GD.outputLabels.toDoVN = string(headerBuff);
   std::sprintf(obuff,"%.*f",getAngularMeasurementPrecisionSOA(),GD.toLLH.dovN);
   GD.outputValues.toDoVN = string(obuff);
   GD.outputCombined.toDoVN = GD.outputLabels.toDoVN + ": " + GD.outputValues.toDoVN;

   //formatted TO E DEFLECTION
   std::sprintf(headerBuff, "E Deflection (soa)");
   GD.outputLabels.toDoVE = string(headerBuff);
   std::sprintf(obuff,"%.*f",getAngularMeasurementPrecisionSOA(),GD.toLLH.dovE);
   GD.outputValues.toDoVE = string(obuff);
   GD.outputCombined.toDoVE = GD.outputLabels.toDoVE + ": " + GD.outputValues.toDoVE;

   // ** EXTRACTED DATA **
   if(GD.stationIsFloatUnused)
   {
       // slant distance
       std::sprintf(headerBuff, "Slant Distance (m)");
       GD.outputLabels.slantDistance = string(headerBuff);
       std::sprintf(obuff,"%.*f (n/a)"
                    ,getLinearPositionPrecisionMeters(),GD.slantDistance);
       GD.outputValues.slantDistance = string(obuff);

       // geodesic distance
       std::sprintf(headerBuff, "Ellipsoidal Distance (m)");
       GD.outputLabels.geodesicDistance = string(headerBuff);
       std::sprintf(obuff,"%.*f (n/a)"
                    ,getLinearPositionPrecisionMeters(),GD.geodesicDistance);
       GD.outputValues.geodesicDistance = string(obuff);

       // horizontal distance
       std::sprintf(headerBuff, "Horizontal Distance (m)");
       GD.outputLabels.horizontalDistance = string(headerBuff);
       std::sprintf(obuff,"%.*f (n/a)"
                    ,getLinearPositionPrecisionMeters(),GD.horzDistance);
       GD.outputValues.horizontalDistance = string(obuff);

       // azimuth
       std::sprintf(headerBuff, "Azimuth [N] (DMS (soa))");
       if(GD.azimuthIsNeg)
       {
           std::sprintf(obuff,"-%d %d %.*f (n/a)",GD.azimuthDeg,GD.azimuthMin,getAngularMeasurementPrecisionSOA(),GD.azimuthSec);
       }

       else
       {
           std::sprintf(obuff,"%d %d %.*f (n/a)",GD.azimuthDeg,GD.azimuthMin,getAngularMeasurementPrecisionSOA(),GD.azimuthSec);
       }

       GD.outputLabels.azimuth = string(headerBuff);
       GD.outputValues.azimuth = string(obuff);

       // vertical Angle
       std::sprintf(headerBuff, "Vertical Angle (DMS (soa))");
       if(GD.verticalAngleIsNeg)
       {

           std::sprintf(obuff,"-%d %d %.*f (n/a)",GD.verticalAngleDeg,GD.verticalAngleMin,getAngularMeasurementPrecisionSOA(),
                        GD.verticalAngleSec);
       }

       else
       {
           std::sprintf(obuff,"%d %d %.*f (n/a)",GD.verticalAngleDeg,GD.verticalAngleMin,getAngularMeasurementPrecisionSOA(),
                        GD.verticalAngleSec);
       }
       GD.outputLabels.verticalAngle = string(headerBuff);
       GD.outputValues.verticalAngle = string(obuff);

       std::sprintf(headerBuff, "Geodetic Height Difference (m)");
       GD.outputLabels.heightDifference = string(headerBuff);
       std::sprintf(obuff,"%.*f (n/a)"
                    ,getLinearPositionPrecisionMeters(),GD.heightDiff);
       GD.outputValues.heightDifference = string(obuff);
       // height difference

       std::sprintf(headerBuff, "Geocentric dX, dY, dZ (m)");
       GD.outputLabels.dXdYdZ = string(headerBuff);
       std::sprintf(obuff,"%.*f %.*f %.*f"
                    ,getLinearPositionPrecisionMeters(),GD.deltaX,getLinearPositionPrecisionMeters(),GD.deltaY,
                    getLinearPositionPrecisionMeters(),GD.deltaZ);
       // dx dy dz
       GD.outputValues.dXdYdZ = string(obuff);

       // covariance matrix

       std::sprintf(headerBuff, "Covariance Matrix");
       GD.outputLabels.deltaCovariance = string(headerBuff);
       std::sprintf(obuff,"n/a  n/a  n/a\n"
                          "                              n/a  n/a\n"
                          "                                   n/a");
       GD.outputValues.deltaCovariance = string(obuff);
   }
   else
   {
       // slant distance
       std::sprintf(headerBuff, "Slant Distance (m)");
       GD.outputLabels.slantDistance = string(headerBuff);
       std::sprintf(obuff,"%.*f (%.*f)"
                    ,getLinearPositionPrecisionMeters(),GD.slantDistance,getLinearPositionPrecisionMeters(),GD.slantDistanceSigma);
       GD.outputValues.slantDistance = string(obuff);

       // geodesic distance
       std::sprintf(headerBuff, "Ellipsoidal Distance (m)");
       GD.outputLabels.geodesicDistance = string(headerBuff);
       std::sprintf(obuff,"%.*f (%.*f)"
                    ,getLinearPositionPrecisionMeters(),GD.geodesicDistance,getLinearPositionPrecisionMeters(),GD.geodesicDistanceSigma);
       GD.outputValues.geodesicDistance = string(obuff);

       // horizontal distance
       std::sprintf(headerBuff, "Horizontal Distance (m)");
       GD.outputLabels.horizontalDistance = string(headerBuff);
       std::sprintf(obuff,"%.*f (%.*f)"
                    ,getLinearPositionPrecisionMeters(),GD.horzDistance,getLinearPositionPrecisionMeters(),GD.horzDistanceSigma);
       GD.outputValues.horizontalDistance = string(obuff);

       // azimuth

       std::sprintf(headerBuff, "Azimuth [N] (DMS (soa))");
       if(GD.azimuthIsNeg)
       {
           std::sprintf(obuff,"-%d %d %.*f (%.*f)",GD.azimuthDeg,GD.azimuthMin,getAngularMeasurementPrecisionSOA(),GD.azimuthSec,
                        getAngularMeasurementPrecisionSOA(),GD.azimuthSigma);
       }

       else
       {
           std::sprintf(obuff,"%d %d %.*f (%.*f)",GD.azimuthDeg,GD.azimuthMin,getAngularMeasurementPrecisionSOA(),GD.azimuthSec,
                        getAngularMeasurementPrecisionSOA(),GD.azimuthSigma);
       }

       GD.outputLabels.azimuth = string(headerBuff);
       GD.outputValues.azimuth = string(obuff);

       // vertical Angle
       std::sprintf(headerBuff, "Vertical Angle (DMS (soa))");
       if(GD.verticalAngleIsNeg)
           std::sprintf(obuff,"-%d %d %.*f (%.*f)",GD.verticalAngleDeg,GD.verticalAngleMin,getAngularMeasurementPrecisionSOA(),
                        GD.verticalAngleSec,getAngularMeasurementPrecisionSOA(),GD.verticalAngleSigma);
       else
           std::sprintf(obuff,"%d %d %.*f (%.*f)",GD.verticalAngleDeg,GD.verticalAngleMin,getAngularMeasurementPrecisionSOA(),
                        GD.verticalAngleSec,getAngularMeasurementPrecisionSOA(),GD.verticalAngleSigma);

       GD.outputLabels.verticalAngle = string(headerBuff);
       GD.outputValues.verticalAngle = string(obuff);

       // height difference
       std::sprintf(headerBuff, "Geodetic Height Difference (m)");
       std::sprintf(obuff,"%.*f (%.*f)"
                    ,getLinearPositionPrecisionMeters(),GD.heightDiff,getLinearPositionPrecisionMeters(),GD.heightDiffSigma);
       GD.outputLabels.heightDifference = string(headerBuff);
       GD.outputValues.heightDifference = string(obuff);

       // dx dy dz
       std::sprintf(headerBuff, "Geocentric dX, dY, dZ (m)");
       std::sprintf(obuff,"%.*f %.*f %.*f"
                    ,getLinearPositionPrecisionMeters(),GD.deltaX,getLinearPositionPrecisionMeters(),GD.deltaY,
                    getLinearPositionPrecisionMeters(),GD.deltaZ);
       GD.outputLabels.dXdYdZ = string(headerBuff);
       GD.outputValues.dXdYdZ = string(obuff);

       // covariance matrix
       std::sprintf(headerBuff, "Covariance Matrix (m^2)");
       std::sprintf(obuff,"%-8.2e  %-8.2e  %-8.2e\n"
                          "        %-8.2e  %-8.2e\n"
                          "                  %-8.2e",
                    GD.deltaCovXX,GD.deltaCovXY,GD.deltaCovXZ,GD.deltaCovYY,GD.deltaCovYZ,GD.deltaCovZZ);
       GD.outputLabels.deltaCovariance = string(headerBuff);
       GD.outputValues.deltaCovariance = string(obuff);
   }

   GD.outputCombined.slantDistance = GD.outputLabels.slantDistance + ": " + GD.outputValues.slantDistance;
   GD.outputCombined.geodesicDistance = GD.outputLabels.geodesicDistance + ": " + GD.outputValues.geodesicDistance;
   GD.outputCombined.horizontalDistance = GD.outputLabels.horizontalDistance + ": " + GD.outputValues.horizontalDistance;
   GD.outputCombined.azimuth = GD.outputLabels.azimuth + ": " + GD.outputValues.azimuth;
   GD.outputCombined.verticalAngle = GD.outputLabels.verticalAngle + ": " + GD.outputValues.verticalAngle;
   GD.outputCombined.heightDifference = GD.outputLabels.heightDifference + ": " + GD.outputValues.heightDifference;
   GD.outputCombined.dXdYdZ = GD.outputLabels.dXdYdZ + ": " + GD.outputValues.dXdYdZ;
   GD.outputCombined.deltaCovariance = GD.outputLabels.deltaCovariance + ": " + GD.outputValues.deltaCovariance;

   if(GD.GUIOutput)
   {
       QString str;

       //from station
         //same as human readable output

       //from latitude
       str = QString::fromStdString(GD.outputCombined.fromGeodeticLatitudeDMS);
       str.remove(" (DMS)");
       GD.outputCombined.fromGeodeticLatitudeDMS = str.toStdString();

       //from longitude
       str = QString::fromStdString(GD.outputCombined.fromGeodeticLongitudeDMS);
       str.remove(" (DMS)");
       GD.outputCombined.fromGeodeticLongitudeDMS = str.toStdString();

       //from Ellipsodial Height
       str = QString::fromStdString(GD.outputCombined.fromEllipsoidalHeight);
       str.remove(" (m)");
       str = str.trimmed();
       GD.outputCombined.fromEllipsoidalHeight = str.toStdString();

       //from Orthometric Height
       str = QString::fromStdString(GD.outputCombined.fromOrthometricHeight);
       str.remove(" (m)");
       str = str.trimmed();
       GD.outputCombined.fromOrthometricHeight = str.toStdString();

       //from undulation
       str = QString::fromStdString(GD.outputCombined.fromUndulation);
       str.remove(" (m)");
       str = str.trimmed();
       GD.outputCombined.fromUndulation = str.toStdString();

       //from deflection North
       str = QString::fromStdString(GD.outputCombined.fromDoVN);
       str.remove(" (soa)");
       str = str.trimmed();
       GD.outputCombined.fromDoVN = str.toStdString();

       //from deflection East
       str = QString::fromStdString(GD.outputCombined.fromDoVE);
       str.remove(" (soa)");
       str = str.trimmed();
       GD.outputCombined.fromDoVE = str.toStdString();

       //to station
          //same as human readable output

       //to latitude
       str = QString::fromStdString(GD.outputCombined.toGeodeticLatitudeDMS);
       str.remove(" (DMS)");
       GD.outputCombined.toGeodeticLatitudeDMS = str.toStdString();

       //to longitude
       str = QString::fromStdString(GD.outputCombined.toGeodeticLongitudeDMS);
       str.remove(" (DMS)");
       GD.outputCombined.toGeodeticLongitudeDMS = str.toStdString();

       //to ellipsodial height
       str = QString::fromStdString(GD.outputCombined.toEllipsoidalHeight);
       str.remove(" (m)");
       str = str.trimmed();
       GD.outputCombined.toEllipsoidalHeight = str.toStdString();

       //to orthometric height
       str = QString::fromStdString(GD.outputCombined.toOrthometricHeight);
       str.remove(" (m)");
       str = str.trimmed();
       GD.outputCombined.toOrthometricHeight = str.toStdString();

       //to undulation
       str = QString::fromStdString(GD.outputCombined.toUndulation);
       str.remove(" (m)");
       str = str.trimmed();
       GD.outputCombined.toUndulation = str.toStdString();

       //to deflection North
       str = QString::fromStdString(GD.outputCombined.toDoVN);
       str.remove(" (soa)");
       str = str.trimmed();
       GD.outputCombined.toDoVN = str.toStdString();

       //to deflection East
       str = QString::fromStdString(GD.outputCombined.toDoVE);
       str.remove(" (soa)");
       str = str.trimmed();
       GD.outputCombined.toDoVE = str.toStdString();

       //EXTRACTED

       //slant distance
       str = QString::fromStdString(GD.outputCombined.slantDistance);
       str.remove(" (m)").remove(",").remove("(").remove(")");;
       GD.outputCombined.slantDistance = str.toStdString();

       //ellipsoidal distance
       str = QString::fromStdString(GD.outputCombined.geodesicDistance);
       str.remove(" (m)").remove(",").remove("(").remove(")");;
       GD.outputCombined.geodesicDistance = str.toStdString();

       //horizontal distance
       str = QString::fromStdString(GD.outputCombined.horizontalDistance);
       str.remove(" (m)").remove(",").remove("(").remove(")");;
       GD.outputCombined.horizontalDistance = str.toStdString();

       //azimuth
       str = QString::fromStdString(GD.outputCombined.azimuth);
       str.remove(" (DMS (soa))").remove("(").remove(")");;
       GD.outputCombined.azimuth = str.toStdString();

       //vertical angle
       str = QString::fromStdString(GD.outputCombined.verticalAngle);
       str.remove(" (DMS (soa))").remove("(").remove(")");;
       GD.outputCombined.verticalAngle = str.toStdString();

       //height difference
       str = QString::fromStdString(GD.outputCombined.heightDifference);
       str.remove(" (m)").remove("(").remove(")");;
       GD.outputCombined.heightDifference = str.toStdString();

       //dx dy dx
       str = QString::fromStdString(GD.outputCombined.dXdYdZ);
       str.remove(" (m)").remove(",");
       GD.outputCombined.dXdYdZ = str.toStdString();

       //covariance
       str = QString::fromStdString((GD.outputCombined.deltaCovariance));
       str = str.remove(" (m^2)").simplified();
       GD.outputCombined.deltaCovariance = str.toStdString();
   }

}
    catch(Exception& e) { GNSSTK_RETHROW(e); }
}
int writeOutputCSV()
{
    GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();

    // Below are checks to make sure inv file exists and a name for output csv can
    // be properly pieced together.

    std::string dirEnding;

    size_t fileEnding = GD.logfile.find(".inv");
    size_t lastDirSep, spacing;

    // Found inv file
    if(fileEnding != string::npos)
    {
        lastDirSep = GD.logfile.find_last_of("/");

        // If true, Windows platform
        if(lastDirSep == string::npos)
        {
            lastDirSep = GD.logfile.find_last_of("\\");
            dirEnding = "\\";

            // Grab project directory info
            if(lastDirSep != string::npos)
            {
                spacing = fileEnding - (lastDirSep + 1);
            }

            // Indicates directory problem.
            else
            {
                return BAD_CSV_OPEN;
            }
        }

        // Otherwise, Linux platform
        else
        {
            dirEnding = "/";
            spacing = fileEnding - (lastDirSep + 1);
        }
    }

    // No inv File
    else
    {
        return INV_FILE_ERROR;
    }

    std::string projSub = GD.logfile.substr(lastDirSep + 1, spacing);
    std::string outputFileName = GD.logpath + dirEnding + projSub + "Inv.csv";

    bool hasData = nonEmpty(outputFileName);

    ofstream outFile;
    outFile.open(outputFileName, ios_base::out | ios_base::app);

    if(!outFile.is_open())
    {
        return BAD_CSV_OPEN;
    }



    if(!hasData)
    {
        outFile << "\n";
        outFile <<  GD.outputLabels.fromStationName << ",";
        outFile <<  GD.outputLabels.fromGeodeticLatitudeDMS << ",,,";
        outFile <<  GD.outputLabels.fromGeodeticLongitudeDMS << ",,,";
        outFile <<  GD.outputLabels.fromEllipsoidalHeight << ",";
        outFile <<  GD.outputLabels.fromOrthometricHeight << ",";
        outFile <<  GD.outputLabels.fromUndulation << ",";
        outFile <<  GD.outputLabels.fromDoVN << ",";
        outFile <<  GD.outputLabels.fromDoVE << ",";

        outFile <<  GD.outputLabels.toStationName << ",";
        outFile <<  GD.outputLabels.toGeodeticLatitudeDMS << ",,,";
        outFile <<  GD.outputLabels.toGeodeticLongitudeDMS << ",,,";
        outFile <<  GD.outputLabels.toEllipsoidalHeight << ",";
        outFile <<  GD.outputLabels.toOrthometricHeight << ",";
        outFile <<  GD.outputLabels.toUndulation << ",";
        outFile <<  GD.outputLabels.toDoVN << ",";
        outFile <<  GD.outputLabels.toDoVE << ",";

        outFile << "EXTRACTED MEASUREMENTS (value; standard deviation):,";
        outFile <<  GD.outputLabels.slantDistance << ",sig,";
        outFile <<  GD.outputLabels.geodesicDistance << ",sig,";
        outFile <<  GD.outputLabels.horizontalDistance << ",sig,";
        outFile <<  GD.outputLabels.azimuth << ",,,sig,";
        outFile <<  GD.outputLabels.verticalAngle << ",,,sig,";
        outFile <<  GD.outputLabels.heightDifference << ",sig,";
        outFile <<  GD.outputLabels.dXdYdZ << ",";
        outFile <<  GD.outputLabels.deltaCovariance;
        outFile << " xx,xy,xz,yy,yz,zz";
    }

    outFile << "\n";

    outFile <<  GD.outputValues.fromStationName << ",";
    //outFile <<  GD.outputValues.fromGeodeticLatitudeDMS << ",";
    outFile <<  splitAndJoin(GD.outputValues.fromGeodeticLatitudeDMS, true);
    //outFile <<  GD.outputValues.fromGeodeticLongitudeDMS << ",";
    outFile <<  splitAndJoin(GD.outputValues.fromGeodeticLongitudeDMS, true);
    outFile <<  GD.outputValues.fromEllipsoidalHeight << ",";
    outFile <<  GD.outputValues.fromOrthometricHeight << ",";
    outFile <<  GD.outputValues.fromUndulation << ",";
    outFile <<  GD.outputValues.fromDoVN << ",";
    outFile <<  GD.outputValues.fromDoVE << ",";

    outFile <<  GD.outputValues.toStationName << ",";
    outFile <<  splitAndJoin(GD.outputValues.toGeodeticLatitudeDMS, true);
    outFile <<  splitAndJoin(GD.outputValues.toGeodeticLongitudeDMS, true);
    outFile <<  GD.outputValues.toEllipsoidalHeight << ",";
    outFile <<  GD.outputValues.toOrthometricHeight << ",";
    outFile <<  GD.outputValues.toUndulation << ",";
    outFile <<  GD.outputValues.toDoVN << ",";
    outFile <<  GD.outputValues.toDoVE << ",,";

    outFile <<  splitAndJoin(GD.outputValues.slantDistance);
    outFile <<  splitAndJoin(GD.outputValues.geodesicDistance);
    outFile <<  splitAndJoin(GD.outputValues.horizontalDistance);
    outFile <<  splitAndJoin(GD.outputValues.azimuth, true);
    outFile <<  splitAndJoin(GD.outputValues.verticalAngle, true);
    outFile <<  splitAndJoin(GD.outputValues.heightDifference);
    outFile <<  splitAndJoin(GD.outputValues.dXdYdZ);

    std::vector<std::string> covarianceBreak = gnsstk::StringUtils::split(GD.outputValues.deltaCovariance, '\n');
    std::string combString;
    for(auto &covElem : covarianceBreak)
    {
        gnsstk::StringUtils::stripLeading(covElem, ' ');
        covElem = splitAndJoin(covElem);
        combString += covElem;
    }

    outFile << combString;

    outFile.close();

    return LSAINVERSE_OK;
}

bool nonEmpty(const string &fileName)
{
    bool bNonEmpty;

    ifstream inFile(fileName);
    if(!inFile.is_open())
    {
        bNonEmpty = false;
    }

    else
    {
        inFile.peek() == ifstream::traits_type::eof() ? bNonEmpty = false : bNonEmpty = true;

        inFile.close();
    }

    return bNonEmpty;
}

string splitAndJoin(const string &origString, bool isAngle, char delim, char join)
{
    std::vector<std::string> splitComponents = gnsstk::StringUtils::split(origString, delim);
    std::string joinedStr;

    // Handles negative angle that has a degrees value of 0. Issue 1221
    if(isAngle)
    {
        if(splitComponents[0] == "-0")
        {
            splitComponents[0] = "\"=\"\"-0\"\"\"";
        }
    }

    for(const auto &strComp : splitComponents)
    {
        joinedStr += (strComp + join);
    }

    std::string removalChars = "()";

    // Removes parentheses from sigma for output csv. Issue 1221
    joinedStr.erase(std::remove_if(joinedStr.begin(), joinedStr.end(),
                                   [&removalChars](const char &c) {
        return removalChars.find(c) != std::string::npos;
    }), joinedStr.end());

    return joinedStr;
}

gnsstk::Vector<double> generateViaPosition(const gnsstk::Position &pos)
{
    gnsstk::Vector<double> castVector(3);
    castVector(0) = pos.getX();
    castVector(1) = pos.getY();
    castVector(2) = pos.getZ();

    return castVector;
}

Eigen::MatrixXd castMatrixgnsstktoEigen(const gnsstk::Matrix<double> &gnsstkMatrix)
{
    int numRows = gnsstkMatrix.rows();
    int numCols = gnsstkMatrix.cols();
    Eigen::MatrixXd castEigen;
    castEigen.setZero(numRows, numCols);
    for(int row = 0; row < numRows; row++)
        for(int col = 0; col < numCols; col++)
            castEigen(row, col) = gnsstkMatrix(row, col);

    return castEigen;
}

Eigen::VectorXd computeHorizontal(Eigen::VectorXd Pos1and2) throw(Exception)
{
    try
    {
        GlobalDataLsainverse& GD=GlobalDataLsainverse::Instance();
        gnsstk::Position fromPosition(Pos1and2(0), Pos1and2(1), Pos1and2(2), gnsstk::Position::Cartesian);
        gnsstk::Position toPosition(Pos1and2(3), Pos1and2(4), Pos1and2(5), gnsstk::Position::Cartesian);

        // Calculate Delta Vector
        gnsstk::Vector<double> DXYZ(3);
        DXYZ = generateViaPosition(toPosition) - generateViaPosition(fromPosition);

        // Double check that fromPosition doesn't have to be constructed gnsstk::Position::Geodetic
        gnsstk::Matrix<double> rotationMat = northEastUpGeodetic(fromPosition);
        gnsstk::Vector<double> DNEU(3);

        // Transform Delta vector
        DNEU = rotationMat * DXYZ;

        Eigen::VectorXd HorizontalDistOutput(1);
        HorizontalDistOutput(0) = std::sqrt(std::pow(DNEU(0), 2.0) + std::pow(DNEU(1), 2.0));
        return HorizontalDistOutput;
    }

    catch(Exception& e)
    {
        GNSSTK_RETHROW(e);
    }
}
