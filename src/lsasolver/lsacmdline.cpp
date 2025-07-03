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
/// @file lsacmdline.cpp  Command line input for program lsasolver

#include "lsasolver.hpp"
#include "lsaobseqn.hpp"

//------------------------------------------------------------------------------------
using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
// prototypes
int GetCommandLine(int argc, char **argv) throw(Exception);
int ValidateInput(void) throw(Exception);
int PreProcess(void) throw(Exception);

//------------------------------------------------------------------------------------
int ProcessCommandLine(int argc, char **argv) throw(gnsstk::Exception)
{
try {
   GlobalData& GD=GlobalData::Instance();

   // process the command line ------------------------------------
   int iret = GetCommandLine(argc,argv);
   if(iret) return iret;

   // check that its valid
   iret = ValidateInput();
   if(iret) return iret;

   // open log file and configure verbose / debug
   iret = PreProcess();

   // title
   if(pLOGstrm != &cout) LOG(INFO) << GD.RunStr;

   // dump configuration
   if(GD.debug > -1) {
      LOG(INFO) << "Found debug switch at level " << GD.debug;
      LOG(INFO) << "\n" << GD.cmdlineUsage;  // this will contain list of args
      // NB debug turns on verbose
   }

   return iret;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int GetCommandLine(int argc, char **argv) throw(Exception)
{
try {
   int i;
   GlobalData& GD=GlobalData::Instance();
   ObsEqnData& EQD=ObsEqnData::Instance();
   SolutionData& SD=SolutionData::Instance();

   // create list of command line options, and fill it
   // put required options first - they will get listed first anyway
   CommandLine opts;

   // build the options list == syntax page
   string PrgmDesc =
      " Program " + GD.PrgmName + " will read an input file and ..."
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
            "Name of file with more options [#-EOL = comment]");
   opts.Add('l', "out", "name", false, req, &GD.logfile, "",
            "Name of output file");
   opts.Add(0, "outpath", "path", false, req, &GD.logpath, "",
            "Path for output file");
   opts.Add('i', "input", "name", false, req, &GD.infile, "",
            "Name of input file(s)");
   opts.Add(0, "inpath", "path", false, req, &GD.inpath, "",
            "Path for input file");
   // Flow control
   opts.Add(0, "apquit", "", false, req, &GD.apquit, "\n# Program control:",
            "Quit after computing a priori positions");
   // LS problem definition
   opts.Add(0, "niter", "n", false, req, &GD.cmd_nitermax,
            "\n# Algorithm [*overwrite DAT file input; default if no DAT input]:",
            "Limit on the number of iterations (default " + asString(GD.nitermax)
            + ") [*]");
   opts.Add(0, "conv", "frac", false, req, &GD.cmd_converge, "",
            "Convergence criterion (default "
            + doubleToScientific(GD.converge,6,1,2) + ") [*]");
   opts.Add(0, "2D","", false, req, &EQD.is2D, "",        // undocument for release
            "Solve a true 2D problem (ignore Z)");
   opts.Add(0, "noAPV", "", false, req, &GD.cmd_noAPV, "",
            "Leave covariance in relative units (default F) [*]");
   opts.Add(0, "APV", "", false, req, &GD.cmd_doAPV, "",
            "Scale covariance with the APV (default T) [*]");
   opts.Add(0, "alpha", "prob", false, req, &SD.alpha, "",
            "Significance level of Chi-squared test [0<prob<1]");
   opts.Add(0, "SOA", "", false, req, &EQD.doSOA, "",
            "Express angle equations in seconds-of-arc");
   opts.Add(0, "allowCOM", "", false, req, &GD.allowAPcentroid, "",
            "Allow center-of-mass in a priori computation");
   opts.Add(0, "statsAll", "", false, req, &GD.statTests, "",
            "Output chi squared and data snooping at every iter.");
   opts.Add(0, "StdResConfid", "prob", false, req, &SD.StdResConfid, "",
            "Confidence level of Standard Residual Threshold (Tau Dist) [0<prob<1]");
   // Data input
   opts.Add('g', "geoidfile", "name", false, req, &GD.geoidfile, "\n# Geoid:",
            "Name of geoid file");
   opts.Add(0, "geoidpath", "path", false, req, &GD.geoidpath, "",
            "Path for geoid file");
   opts.Add(0, "interp", "method", false, req, &GD.geoidinterp, "",
            "Geoid interpolation method [bicubic or bilinear]");
   opts.Add(0, "patchoverwrite", "", false, req, &GD.overwriteGeoidPatch, "",
            "Overwrite existing .h5 file with geoid heights in project data directory");
   // Editing
   // Output
   opts.Add(0, "eqnout", "file", false, req, &GD.obseqnfile, "\n# Output:",
            "Output observation equations to this file");
   opts.Add(0, "bin", "file", false, req, &GD.binfile, "",
            "Output results for GUI to file in binary format");
   opts.Add(0, "extr", "\"str\"", true, req, &GD.Extracts, "",
            "Extraction string (EXTR tag TYP label[s])");
   opts.Add(0, "westLon", "", false, req, &GD.doWest, "",
            "Output west longitude");
   opts.Add(0, "datout", "file", false, req, &GD.outdatfile, "",
            "Output a complete DAT file after adjustment");
   opts.Add(0, "progress", "", false, req, &GD.doProgress, "",
            "Output summary information to stdout, incl. each iter.");
   opts.Add(0, "linprecM", "", false, req, &GD.linprecM, "",
            "Output linear precision (M)");
   opts.Add(0, "linprecP", "", false, req, &GD.linprecP, "",
            "Output linear precision (P)");
   opts.Add(0, "linwidth", "", false, req, &GD.linwidth, "",
            "Output linear width (M)");
   opts.Add(0, "angprecM", "", false, req, &GD.angprecM, "",
            "Output angular precision for measurements in SOA");
   opts.Add(0, "angwidthM", "", false, req, &GD.angwidthM, "",
            "Output angular width for measurements in SOA");
   opts.Add(0, "angprecP", "", false, req, &GD.angprecP, "",
            "Output angular precision of SOA for positions (ie. LLH)");
   opts.Add(0,"noExtRelVect", "", false, req, &GD.noExtRelVect, "",
           "Do not ouptut the external reliability vectors");
   // Help
   opts.Add(0, "validate", "", false, req, &GD.validate, "\n# Help:",
            "Read input and test its validity, then quit");
   opts.Add(0, "verbose", "", false, req, &GD.verbose, "",
            "Print extended output information");
   opts.Add(0, "debug", "", false, req, &GD.debug, "",
            "Print debug output at level 0 [debug<n> for level n=1-7]");
   opts.Add(0, "timing", "", false, req, &GD.doTiming, "",
            "Print timing information");
   opts.Add(0, "forcefast", "", false, req, &GD.forcefast, "",
            "Placeholder for config file");
   opts.Add(0, "forcestable", "", false, req, &GD.forcestable, "",
            "Use SRIF solver only");
   opts.Add(0, "help", "", false, req, &GD.help, "",
            "Print this and quit");

   // add options that are ignored (true if it has an arg)
   //opts.Add_ignore("--PRSoutput",true);
   opts.Add_ignore("--confid",true);
   opts.Add_ignore("--customExportCopy", true);
   opts.Add_ignore("--customImportCopy", true);
   opts.Add_ignore("--includeUnused");
   opts.Add_ignore("--warningExtRelVect", true);
   opts.Add_ignore("--errorExtRelVect", true);
   opts.Add_ignore("--calcExtRelVect");

   // add deprecated (pseudonym) options (old,new)
   opts.Add_deprecated("--dat","--input");
   opts.Add_deprecated("--log","--out");

   // --------------------------------------------------------------------------
   // declare it and parse it; write all errors to string GD.cmdlineErrors
   int iret = opts.ProcessCommandLine(argc,argv,PrgmDesc,
                         GD.cmdlineUsage,GD.cmdlineErrors,GD.cmdlineUnrecognized);
   if(iret == -2) return iret;      // bad alloc
   if(iret == -3) return iret;      // cmd line definition invalid

   // if --forcestable is specified, do not use Eigen3 solver
   if(GD.forcestable)
       GD.useSRIF = true;

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

   if(opts.hasHelp()) return 1;
   if(opts.hasErrors()) return 2;
   if(!GD.cmdlineErrors.empty()) {
      //LOG(INFO) << "RETURNING invalid b/c of errors:\n" << GD.cmdlineErrors;
      return 2;
   }
   return 0;
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
   include_path(GD.logpath,GD.logfile);
   expand_filename(GD.logfile);

   include_path(GD.inpath,GD.infile);
   expand_filename(GD.infile);
   LOG(VERBOSE) << " Found input file name " << GD.infile;

   if(!GD.geoidfile.empty())
   {
      GD.geoidfile0 = GD.geoidfile;
      if (GD.geoidfile0.find_last_of("/\\") != string::npos)
      {
         GD.geoidfile0 = GD.geoidfile0.substr(GD.geoidfile0.find_last_of("/\\")+1);
      }
      include_path(GD.geoidpath,GD.geoidfile);
      expand_filename(GD.geoidfile);
      LOG(VERBOSE) << " Found geoid file name " << GD.geoidfile;
   }

   //ifstream istrm;
   //istrm.open(GD.infile.c_str(), ios::in);
   //if(!istrm.is_open()) {
   //   msg = string("  Error - input file ") + GD.infile
   //       + string(" could not be opened.");
   //   LOG(ERROR) << msg;
   //   GD.inputIsValid = false;
   //}
   //else {
   //   istrm.clear();
   //   istrm.close();
   //   LOG(VERBOSE) << " Found input file " << GD.infile;
   //}

   if(GD.validate) {
      LOG(INFO) << " ---- Input is "
         << (GD.inputIsValid ? "" : "NOT ") << "valid ----";
      return 3;
   }

   if(GD.inputIsValid) return 0;

   return 4;
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
         return 5;
      }
      LOG(INFO) << "Output directed to log file " << GD.logfile;
      pLOGstrm = &GD.oflog; // ConfigureLOG::Stream() = &GD.oflog;
   }

   // configure log stream
   ConfigureLOG::ReportLevels() = false;
   ConfigureLOG::ReportTimeTags() = false;
   // debug and verbose handled earlier in GetCommandLine/PreProcessArgs
   if(GD.debug > -1)
      ; // handled in CommandLine::PreProcessArgs()
   else if(GD.verbose)
      LOGlevel = ConfigureLOG::Level("VERBOSE");

   return 0;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
