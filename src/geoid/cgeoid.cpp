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
// cgeoid.cpp  A command line geoid tool

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

//------------------------------------------------------------------------------------
// system includes
#include <ctime>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <QDir>
// GNSSTk
//#define RANGECHECK 1 // if defined, Matrix and Vector will throw on invalid index.
#include "Exception.hpp"
#include "StringUtils.hpp"
#include "Epoch.hpp"
#include "Position.hpp"
//#include "Stats.hpp"
#include "singleton.hpp"
// geomatics
#include "expandtilde.hpp"
#include "logstream.hpp"
// cgeoid
#include "CommandLine.hpp"
#include "geoid.hpp"

#include "LSAConstants.hpp"

//------------------------------------------------------------------------------------
using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

enum CGEOID_RETURNS
{
    COMMAND_LINE_INVALID_DEF = -3,
    BAD_ALLOCATION,
    CGEOID_OK = 0,
    COMMAND_LINE_USAGE,
    COMMAND_LINE_ERRORS,
    REQUESTED_VALIDATION,
    INVALID_INPUT,
    OPEN_FAILURE,
    DUMP_REQUEST,
    OTHER_FAILURE
};

//------------------------------------------------------------------------------------
// NB Version below class GlobalData

//------------------------------------------------------------------------------------
// prototypes
int GetCommandLine(int argc, char **argv) throw(Exception);
int ValidateInput(void) throw(Exception);
int PreProcess(void) throw(Exception);
int Process(void) throw(Exception);

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
// Class GlobalData encapsulates global static data.
class GlobalData : public gnsstk::Singleton<GlobalData> {
public:
   // Default and only constructor, sets defaults.
   GlobalData() throw() { SetDefaults(); }

   // prgm housekeeping
   static const std::string Version;        // see below
   std::string PrgmName;                    // this program (must match Jamfile)
   std::string Title;                       // name, version and runtime
   std::string logfile;                     // name of log file
   std::ofstream oflog;                     // output log file stream

   // command line input ----------------------------------------------------------
   bool inputIsValid;

   std::string cmdlineErrors,cmdlineDump,cmdlineUsage;   // strings filled by parser
   std::vector<std::string> cmdlineUnrecognized;

   std::string gfile;                        // geoid file
   std::string logpath,gpath;                // paths

   // limits of the grid read from the file
   double latmin,latmax;                     // limits of latitude (deg)
   double lonmin,lonmax;                     // limits of longitude (deg)
   std::vector<gnsstk::Position> positions;   // positions at which to compute
   std::string interpStr;                    // String of interpolator
   Geoid::Interpolator interp;               // Interpolator to use

   bool verbose,help,validate;                           // output
   int debug;
   // end command line input ------------------------------------------------------

   // results

private:
   void SetDefaults(void) throw()
   {
      PrgmName = std::string("cgeoid");
      logfile = std::string("");
      gfile = std::string("egm2008_2.5m.und");

      // command line input -------------------------------------------
      latmin = lsa::MIN_LATITUDE_BOUNDARY;
      latmax = lsa::MAX_LATITUDE_BOUNDARY;
      lonmin = 0.0;
      lonmax = 360.0;
      interpStr = "bicubic";

      // editing

      // output
      debug = -1;
      help = verbose = validate = false;

      // end command line input ---------------------------------------
   }

}; // end class GlobalData

//------------------------------------------------------------------------------------
const string GlobalData::Version(string("1.0 4/30/15"));

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
   string PrgmName;        // for catch
try {
   // begin counting time
   clock_t totaltime = clock();
   Epoch wallbegin,wallend;
   wallbegin.setLocalTime();

   // get (create) the global data object (a singleton);
   // since this is the first instance, this will also set default values
   GlobalData& GD=GlobalData::Instance();
   PrgmName = GD.PrgmName;

   // Build title
   Epoch ttag;
   ttag.setLocalTime();
   GD.Title = GD.PrgmName + ", Ver. "
      + GD.Version + ttag.printf(", Run %04Y/%02m/%02d at %02H:%02M:%02S");
 
   // display title on screen
   LOG(INFO) << GD.Title;

   // TEMP, for debugging CommandLine;
   //LOGlevel = ConfigureLOG::Level("DEBUG");

   // process : loop once -----------------------------------------------------
   int iret;
   for(bool go=true; go; go=false)  {

      // process the command line ------------------------------------
      iret = GetCommandLine(argc,argv);
      if(iret) break;

      // check that its valid
      iret = ValidateInput();
      if(iret) break;
      // open log file, dump config
      iret = PreProcess();
      if(iret) break;
      // do it
      iret = Process();
      if(iret) break;

   }  // end loop once

   // error condition ---------------------------------------------------------
   // return codes: 0 ok
   //               1 help
   //               2 cmd line errors
   //               3 requested validation
   //               4 invalid input
   //               5 open fail
   //               6 requested dump
   //               7 other...
   //              -3 cmd line definition invalid (CommandLine)
   //LOG(INFO) << "Return code is " << iret;
   if(iret != CGEOID_OK) {
      if(iret != COMMAND_LINE_USAGE) {
         string msg;
         msg = GD.PrgmName + string(" is terminating with code ")
                           + StringUtils::asString(iret);
         LOG(ERROR) << msg;
      }

      if(iret == COMMAND_LINE_USAGE) { LOG(INFO) << GD.cmdlineUsage; }
      else if(iret == COMMAND_LINE_ERRORS) { LOG(INFO) << GD.cmdlineErrors; }
      else if(iret == REQUESTED_VALIDATION) { LOG(INFO) << "The user requested input validation."; }
      else if(iret == INVALID_INPUT) { LOG(INFO) << "The input is invalid."; }
      else if(iret == OPEN_FAILURE) { LOG(INFO) << "The log file could not be opened."; }
      else if(iret == OTHER_FAILURE) { LOG(INFO) << "The input file could not be opened."; }
      else if(iret == COMMAND_LINE_INVALID_DEF) { // cmd line definition invalid
         LOG(INFO) << "The command line definition is invalid.\n" << GD.cmdlineErrors;
      }
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
      if(pLOGstrm != &cout) LOG(INFO) << oss.str();
      cout << oss.str() << endl;
   }

   if(iret == CGEOID_OK) return 0; else return -1;
}
catch(Exception& e) {
   cerr << PrgmName << " caught Exception:\n" << e.what() << endl;
   // don't use LOG here - causes hangup - don't know why
}
catch (...) {
   cerr << "Unknown error in " << PrgmName << ".  Abort." << endl;
}
   return -1;
}   // end main()

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
int GetCommandLine(int argc, char **argv) throw(Exception)
{
try {
   int i;
   vector<string> pos_input;
   GlobalData& GD=GlobalData::Instance();

   // create list of command line options, and fill it
   // put required options first - they will get listed first anyway
   CommandLine opts;

   // build the options list == syntax page
   string PrgmDesc =
" Program " + GD.PrgmName + " will read an input geoid file and compute undulation"
"\n and deflection of the vertical for given input latitude/longitude pairs."
"\n Input is on the command line, or of the same format in a file "
"(see --file below);\n lines in that file which begin with '#' are ignored. "
"Accepted options are \n shown below, followed by a description, with default "
"value, if any, in ().";

   // opts.Add(char, opt, arg, repeat?, required?, &target, pre-descript, descript.);
   // required options
   bool req(true);
   // optional args
   req = false;
   string dummy("");         // dummy for --file
   opts.Add('f', "file", "name", true, req, &dummy, "\n# File I/O:",
            "Name of file containing more options [#-EOL = comment]");
   opts.Add(0, "gfile", "name", false, req, &GD.gfile, "",
            "Name of input geoid file");
   opts.Add('l', "log", "name", false, req, &GD.logfile, "",
            "Name of output log file");
   opts.Add(0, "gpath", "path", false, req, &GD.gpath, "",
            "Path for input geoid file");
   opts.Add(0, "logpath", "path", false, req, &GD.logpath, "",
            "Path for output log file");
   // Data input
   opts.Add(0, "latmin", "lat", false, req, &GD.latmin, "\n# Geoid data input:",
            "Minimum latitude to read from file, degrees");
   opts.Add(0, "latmax", "lat", false, req, &GD.latmax, "",
            "Maximum latitude to read from file, degrees");
   opts.Add(0, "lonmin", "lon", false, req, &GD.lonmin, "",
            "Minimum E longitude to read from file, degrees");
   opts.Add(0, "lonmax", "lon", false, req, &GD.lonmax, "",
            "Maximum E longitude to read from file, degrees");
   opts.Add(0, "pos", "lat:lon", true, req, &pos_input, "\n# Position data input:",
            "Position (degrees, E lon) at which to compute");
   opts.Add(0, "interpolator", "interp", false, req, &GD.interpStr, "",
            "Interpolation method to be used when calculating values [bicubic|bilinear]");
   // Flow control
   // Editing
   // Output
   opts.Add(0, "validate", "", false, req, &GD.validate, "\n# Output:",
            "Read input and test its validity, then quit");
   opts.Add(0, "verbose", "", false, req, &GD.verbose, "",
            "Print extended output information");
   opts.Add(0, "debug", "", false, req, &GD.debug, "",
            "Print debug output at level 0 [debug<n> for level n=1-7]");
   opts.Add(0, "help", "", false, req, &GD.help, "",
            "Print this and quit");

   // add options that are ignored (true if it has an arg)
   //opts.Add_ignore("--PRSoutput",true);

   // deprecated args
   //opts.Add_deprecated("--HtOffset","--ht");

   // --------------------------------------------------------------------------
   // declare it and parse it; write all errors to string GD.cmdlineErrors
   int iret = opts.ProcessCommandLine(argc,argv,PrgmDesc,
                         GD.cmdlineUsage,GD.cmdlineErrors,GD.cmdlineUnrecognized);
   if(iret == BAD_ALLOCATION) return iret;      // bad alloc
   if(iret == COMMAND_LINE_INVALID_DEF) return iret;      // cmd line definition invalid

   // --------------------------------------------------------------------------
   // do extra parsing -- append errors to GD.cmdlineErrors
   string msg;
   vector<string> fields;
   ostringstream oss;

   // positions
   for(i=0; i<pos_input.size(); i++) {
      fields = splitWithDoubleQuotes(pos_input[i],':');
      if(fields.size() != 2) {
         LOG(WARNING) << " Warning - invalid --pos input: " << pos_input[i] << ".";
         continue;
      }
      Position p(asDouble(fields[0]), asDouble(fields[1]),0.0, Position::Geodetic);
      GD.positions.push_back(p);
   }

   // unrecognized arguments are an error
   if(GD.cmdlineUnrecognized.size() > 0) {
      oss << " Error - unrecognized arguments:\n";
      for(i=0; i<GD.cmdlineUnrecognized.size(); i++)
         oss << GD.cmdlineUnrecognized[i] << "\n";
      oss << " End of unrecognized arguments\n";
   }

   // append errors
   GD.cmdlineErrors += oss.str();

   // --------------------------------------------------------------------------
   LOG(DEBUG) << GD.cmdlineUsage;  // this will contain list of args

   // dump it
   oss.str("");         // clear it
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
   return CGEOID_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Check input at the level of files exist? etc.
// Set bool GD.inputIsValid
int ValidateInput(void) throw(Exception)
{
try {
   int i,j;
   string msg;
   GlobalData& GD=GlobalData::Instance();
   GD.inputIsValid = true;

   if(GD.validate) {
      LOG(INFO) << " ---- Validate configuration ----";
   }

   // where else to do this?
   if(!GD.logfile.empty()) {
      include_path(GD.logpath,GD.logfile);
      expand_filename(GD.logfile);
   }

   if(GD.gfile.empty()) {
      msg = string("  Error - no input geoid file.");
      LOG(ERROR) << msg;
      GD.inputIsValid = false;
   }
   else {
       //Search for gfile in project directory, set gpath if found
       QDir projDir(QString::fromStdString(GD.logpath));
       QStringList projDirFiles = projDir.entryList();
       if(projDirFiles.contains(QString::fromStdString(GD.gfile)))
         GD.gpath = GD.logpath;
      include_path(GD.gpath,GD.gfile);
      expand_filename(GD.gfile);
      LOG(VERBOSE) << " Use input geoid file " << GD.gfile;
   }

/*   if(!GD.interpStr.empty()) {
      if(GD.interpStr == string("bilinear") || GD.interpStr == string("bicubic"))
          LOG(VERBOSE) << " Use interpolator " << GD.interpStr;
      else {
         msg = string("  Error - Invalid interpolation method.");
         LOG(ERROR) << msg;
         GD.inputIsValid = false;
      }
   }
   else {
      msg = string("  Error - No interpolation method selected.");
      LOG(ERROR) << msg;
      GD.inputIsValid = false;
   }
*/

   if(GD.validate) {
      LOG(INFO) << " ---- Input is "
         << (GD.inputIsValid ? "" : "NOT ") << "valid ----";
      return REQUESTED_VALIDATION;
   }

   if(GD.inputIsValid) return CGEOID_OK;

   return INVALID_INPUT;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int PreProcess(void) throw(Exception)
{
try {
   GlobalData& GD=GlobalData::Instance();

   // open log file, if it exists
   if(!GD.logfile.empty()) {
      GD.oflog.open(GD.logfile.c_str(),ios_base::out);
      if(!GD.oflog) {
         cerr << "Failed to open log file " << GD.logfile << endl;
         return OPEN_FAILURE;
      }
      LOG(INFO) << "Output directed to log file " << GD.logfile;
      pLOGstrm = &GD.oflog; // ConfigureLOG::Stream() = &GD.oflog;
   }

   // Set interpolator
   if(GD.interpStr == "bicubic")
       GD.interp = Geoid::BICUBIC;
   else if(GD.interpStr == "bilinear")
       GD.interp = Geoid::BILINEAR;
   else
       GNSSTK_THROW(Exception(string("Invalid interpolation method selected - ") + GD.interpStr));

   // configure log stream
   ConfigureLOG::ReportLevels() = false;
   ConfigureLOG::ReportTimeTags() = false;
   // debug and verbose handled earlier in GetCommandLine/PreProcessArgs
   if(GD.debug > -1)
      ; // handled in CommandLine::PreProcessArgs()
   else if(GD.verbose)
      LOGlevel = ConfigureLOG::Level("VERBOSE");
   if(pLOGstrm != &cout) LOG(INFO) << GD.Title;

   // dump configuration
   if(GD.debug > -1) {
      LOG(INFO) << "Found debug switch at level " << GD.debug;
      LOG(INFO) << "\n" << GD.cmdlineUsage;  // this will contain list of args
      // NB debug turns on verbose
   }
   LOG(VERBOSE) << GD.cmdlineDump;

   return CGEOID_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int Process(void) throw(Exception)
{
try {
   GlobalData& GD=GlobalData::Instance();

   Geoid geoid(GD.gfile);

   geoid.loadGeoid(GD.latmin,GD.latmax,GD.lonmin,GD.lonmax);
   geoid.setInterpolation(GD.interp);
   double und,dov_n,dov_e;
   for(int i=0; i<GD.positions.size(); i++) {
      geoid.calculateAll(GD.positions[i],und,dov_n,dov_e);
      LOG(INFO) << GD.positions[i].printf(" lat N = %10.6A deg, lon E = %10.6L deg,")
         << " Und = " << fixed << setprecision(6) << setw(10) << und
         << " DOV N,E = " << fixed << setprecision(6) << setw(10) << dov_n << ","
                          << fixed << setprecision(6) << setw(10) << dov_e;
   }
    
   return CGEOID_OK;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
