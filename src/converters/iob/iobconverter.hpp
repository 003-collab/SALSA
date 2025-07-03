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
/// @file lsasolver.hpp  Global include file for ARL:UT LSA least squares solver.


// disable some MSVC compiler warnings
#pragma warning(disable:4290)

// system includes
#include <ostream>
#include <string>
#include <vector>
#include <map>

// GNSSTk
#include "Exception.hpp"
#include "Epoch.hpp"
#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Vector.hpp"
#include "singleton.hpp"

// geomatics
//#include "expandpath.hpp"
#include "expandtilde.hpp"
#include "logstream.hpp"
#include "CommandLine.hpp"
//#include "Namelist.hpp"
//#include "SRI.hpp"

// lsa
#include "IOBfile.hpp"
//#include "lsaUtils.hpp"

#ifndef IOB_CONVERTER_INCLUDE
#define IOB_CONVERTER_INCLUDE

//------------------------------------------------------------------------------------
/// Class GlobalData encapsulates global static data for program lsasolver.
class GlobalData : public gnsstk::Singleton<GlobalData> {
public:
   /// Default and only constructor, sets defaults.
   GlobalData() throw() { SetDefaults(); }

   // prgm housekeeping
   static const std::string Version;   ///< see below
   std::string PrgmName;               ///< name of this program
   std::string RunStr;                 ///< name, version and runtime
   std::string infile;                 ///< input file name (.iob file)
   std::string inpath;                 ///< input path for input file
   std::string logfile;                ///< name of log file
   std::string logpath;                ///< input path for logfile
   std::string warnfile;
   std::string warnpath;
   std::ofstream oflog;                ///< output log file stream
   std::ofstream ofwarn;
   // strings filled by parser
   std::string cmdlineErrors;          ///< command line errors
   std::string cmdlineDump;            ///< command line summary
   std::string cmdlineUsage;           ///< command line usage
   /// vector of command line unrecognized arguments
   bool inputIsValid;                  ///< true if input is valid
   std::vector<std::string> cmdlineUnrecognized;
   bool isProject;                     ///< true if need to include lsa options file
   std::string projDir;             ///< path above which lsa files are not generated
   bool verbose;                       ///< verbose output, if true
   bool help;                          ///< if true, output syntax page and quit
   bool validate;                      ///< if true, check validity of input only
   int debug;                          ///< level of debug output, > -1 (the default)
   bool conversionFailures;            ///<true if failure encountered generating LSA string
   bool parseFailures;                 ///<true if failure parsing an IOB record
   bool unsupportedRecords;            ///<true if unsupported GeoLab record encountered
   bool invalidUnits;                  ///<true if invalid or no units are encountered
   bool unsupportedCOVScaling;         ///<true if non-whole-matrix scaling factors encountered
   bool commentedIncludes;             ///<true if there were commented geolab include records
   bool missingHFROM;                  ///<true if DIST, ZANG, or VANG record is missing an HFROM

private:
   /// Set default values in the configuration, called by constructor
   void SetDefaults(void) throw()
   {
      PrgmName = std::string("iobconverter");
      logfile = std::string("");
      cmdlineUsage = std::string("You gotta specify inputs 'n' outputs. --lsa for output and --iob for input.");           ///< command line usage

      // command line input -------------------------------------------
   
      // input control

      // editing

      // solution

      // output
      debug = -1;
      help = verbose = validate = false;
      // end command line input ---------------------------------------
   }
}; // end class GlobalData

//------------------------------------------------------------------------------------
// prototypes

//------------------------------------------------------------------------------------
#endif   // IOB_CONVERTER_INCLUDE
