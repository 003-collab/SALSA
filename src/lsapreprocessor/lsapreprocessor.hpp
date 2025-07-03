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
/// @file lsapreprocessor.hpp  Global include file for ARL:UT LSA least squares PREPROCESSOR.

// system includes
#include <ostream>
#include <string>
#include <vector>
#include <map>

// GNSSTk
#include "Exception.hpp"
#include "StringUtils.hpp"
#include "Epoch.hpp"
#include "stl_helpers.hpp"
#include "Stats.hpp"
#include "singleton.hpp"

// geomatics
#include "expandpath.hpp"
#include "logstream.hpp"
#include "CommandLine.hpp"
#include "Namelist.hpp"
#include "SRI.hpp"

// lsafile
#include "LSAFile.hpp"

// dat record types
#include "DATAzimuth.hpp"
#include "DATConfig.hpp"
#include "DATDelta.hpp"
#include "DATDist.hpp"
#include "DATHeight.hpp"
#include "DATHAngle.hpp"
#include "DATPoint.hpp"
#include "DATVAngle.hpp"
#include "DATZAngle.hpp"

#include "lsaUtils.hpp"

#ifndef LSA_PREPROCESSOR_INCLUDE
#define LSA_PREPROCESSOR_INCLUDE

//------------------------------------------------------------------------------------
/// Class PreprocessorData encapsulates global static data for program lsapreprocessor.
class PreprocessorData : public gnsstk::Singleton<PreprocessorData> {
public:
   /// Default and only constructor, sets defaults.
   PreprocessorData() throw() { SetDefaults(); }
   ~PreprocessorData() { delete lsaFile; }

   // prgm housekeeping
   static const std::string Version;   ///< see below
   std::string PrgmName;               ///< name of this program
   std::string RunStr;                 ///< name, version and runtime
   std::string logfile;                ///< name of log file
   std::ofstream oflog;                ///< output log file stream

   // command line input ----------------------------------------------------------
   bool inputIsValid;                  ///< true if input is valid

   // strings filled by parser
   std::string cmdlineErrors;          ///< command line errors
   std::string cmdlineDump;            ///< command line summary
   std::string cmdlineUsage;           ///< command line usage
   /// vector of command line unrecognized arguments
   std::vector<std::string> cmdlineUnrecognized;

   std::string lsaPath;                ///< input path for lsa input file
   std::string lsaFilename;            ///< input file name (.lsa file)
   std::string datPath;                ///< output path for dat file
   std::string datFilename;            ///< output file name (.dat file)

   bool verbose;                       ///< verbose output, if true
   int outPrec;                        ///< output precision in general
   int outWidth;                       ///< output width in general
   int outLinPrec;                     ///< output precision in linear quanitites
   int outLinWidth;                    ///< output width in linear quanitites
   int outAngPrec;                     ///< output precision in angular quanitites
   int outAngWidth;                    ///< output width in angular quanitites

   int datLinPrec;                     ///< output precision in linear quanitites
   int datLinWidth;                    ///< output width in linear quanitites
   int datAngPrec;                     ///< output precision in angular quanitites
   int datAngWidth;                    ///< output width in angular quanitites

   bool help;                          ///< if true, output syntax page and quit
   bool validate;                      ///< if true, check validity of input only
   int debug;                          ///< level of debug output, > -1 (the default)
   // end command line input ------------------------------------------------------

   LSAFile *lsaFile;                    ///< all data from input .lsa file

private:
   /// Set default values in the configuration, called by constructor
   void SetDefaults(void) throw()
   {
      PrgmName = std::string("lsapreprocessor");


      // command line input -------------------------------------------
      // input control
      // editing
      // output
      debug = -1;
      help = verbose = validate = false;
      outPrec = 3;
      outWidth = 12;
      outLinPrec = 4;   // 1/10 MM
      outLinWidth = 10;
      outAngPrec = 9;
      outAngWidth = 14;

      datLinPrec = 6;                     ///< output precision in linear quanitites
      datLinWidth = 12;                    ///< output width in linear quanitites
      datAngPrec = 11;                     ///< output precision in angular quanitites
      datAngWidth = 14;

      lsaFile = new LSAFile(false);
      // end command line input ---------------------------------------
   }
}; // end class PreprocessorData


//------------------------------------------------------------------------------------
#endif   // LSA_PREPROCESSOR_INCLUDE
