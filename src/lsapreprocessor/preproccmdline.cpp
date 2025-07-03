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
/// @file lsacmdline.cpp  Command line input for program lsapreprocessor

#include "lsapreprocessor.hpp"

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
   // process the command line ------------------------------------
   int iret = GetCommandLine(argc,argv);
   if(iret) return iret;

   // check that its valid
   iret = ValidateInput();
   if(iret) return iret;

   // open log file and configure verbose / debug
   iret = PreProcess();

   // write to output
   PreprocessorData& PD=PreprocessorData::Instance();

   // title
   if(pLOGstrm != &cout) LOG(INFO) << PD.RunStr;

   // dump configuration
   if(PD.debug > -1) {
      LOG(INFO) << "Found debug switch at level " << PD.debug;
      LOG(INFO) << "\n" << PD.cmdlineUsage;  // this will contain list of args
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
   PreprocessorData& PD=PreprocessorData::Instance();

   // create list of command line options, and fill it
   // put required options first - they will get listed first anyway
   CommandLine opts;

   // build the options list == syntax page
   string PrgmDesc =
      " Program " + PD.PrgmName + " will read an input file and ..."
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
   opts.Add('l', "out", "name", false, req, &PD.datFilename, "",
            "Name of output file");
   opts.Add(0, "outpath", "path", false, req, &PD.datPath, "",
            "Path for output file");
   opts.Add('i', "input", "name", false, req, &PD.lsaFilename, "",
            "Name of input file(s)");
   opts.Add(0, "inpath", "path", false, req, &PD.lsaPath, "",
            "Path for input file");
   // Flow control 
   // Data input
   // Editing
   // Output
   // Help
   opts.Add(0, "validate", "", false, req, &PD.validate, "\n# Help:",
            "Read input and test its validity, then quit");
   opts.Add(0, "verbose", "", false, req, &PD.verbose, "",
            "Print extended output information");
   opts.Add(0, "debug", "", false, req, &PD.debug, "",
            "Print debug output at level 0 [debug<n> for level n=1-7]");
   opts.Add(0, "help", "", false, req, &PD.help, "",
            "Print this and quit");

   // add options that are ignored (true if it has an arg)

   // add deprecated (pseudonym) options (old,new)

   // --------------------------------------------------------------------------
   // declare it and parse it; write all errors to string PD.cmdlineErrors
   int iret = opts.ProcessCommandLine(argc,argv,PrgmDesc,
                         PD.cmdlineUsage,PD.cmdlineErrors,PD.cmdlineUnrecognized);
   if(iret == -2) return iret;      // bad alloc
   if(iret == -3) return iret;      // cmd line definition invalid

   // --------------------------------------------------------------------------
   // do extra parsing -- append errors to PD.cmdlineErrors
   //string msg;
   //vector<string> fields;
   ostringstream oss;

   // interpret one unrecognized argument as input file name
   if(PD.cmdlineUnrecognized.size() == 1 && PD.lsaFilename.empty()) {
      PD.lsaFilename = PD.cmdlineUnrecognized[0];
      PD.cmdlineUnrecognized.clear();
      //LOG(INFO) << " Input file: " << PD.infile;
   }

   // unrecognized arguments are an error
   if(PD.cmdlineUnrecognized.size() > 0) {
      oss << " Warning - unrecognized arguments:\n";
      for(i=0; i<PD.cmdlineUnrecognized.size(); i++)
         oss << PD.cmdlineUnrecognized[i] << "\n";
      oss << " End of unrecognized arguments.\n";
   }

   // --------------------------------------------------------------------------
   LOG(DEBUG) << PD.cmdlineUsage;  // this will contain list of args

   // dump it
   oss << "------ Summary of " << PD.PrgmName
      << " command line configuration --------" << endl;
   opts.DumpConfiguration(oss);
      // perhaps dump the 'extra parsing' things
   oss << "------ End configuration summary --------" << endl;
   PD.cmdlineDump = oss.str();

   if(opts.hasHelp()) return 1;
   if(opts.hasErrors()) return 2;
   if(!PD.cmdlineErrors.empty()) {
      //LOG(INFO) << "RETURNING invalid b/c of errors:\n" << PD.cmdlineErrors;
      return 2;
   }
   return 0;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
// Check input at the level of files exist? etc.
// Set bool PD.inputIsValid
int ValidateInput(void) throw(Exception)
{
try {
   int i,j;
   string msg;
   PreprocessorData& PD=PreprocessorData::Instance();
   PD.inputIsValid = true;

   if(PD.validate) {
      LOG(INFO) << " ---- Validate configuration ----";
   }

//   // where else to do this?
//   include_path(PD.logpath,PD.logfile);
//   expand_filename(PD.logfile);

//    include_path(PD.inpath,PD.infile);
//    expand_filename(PD.infile);
//    LOG(VERBOSE) << " Found input file name " << PD.infile;

   //ifstream istrm;
   //istrm.open(PD.infile.c_str(), ios::in);
   //if(!istrm.is_open()) {
   //   msg = string("  Error - input file ") + PD.infile
   //       + string(" could not be opened.");
   //   LOG(ERROR) << msg;
   //   PD.inputIsValid = false;
   //}
   //else {
   //   istrm.clear();
   //   istrm.close();
   //   LOG(VERBOSE) << " Found input file " << PD.infile;
   //}

   if(PD.validate) {
      LOG(INFO) << " ---- Input is "
         << (PD.inputIsValid ? "" : "NOT ") << "valid ----";
      return 3;
   }

   if(PD.inputIsValid) return 0;

   return 4;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int PreProcess(void) throw(Exception)
{
try {
   PreprocessorData& PD=PreprocessorData::Instance();

   // open log file, if it exists
   if(!PD.logfile.empty()) {
      PD.oflog.open(PD.logfile.c_str(),ios_base::out);
      if(!PD.oflog) {
         cerr << "Failed to open log file " << PD.logfile << endl;
         return 5;
      }
      LOG(INFO) << "Output directed to log file " << PD.logfile;
      pLOGstrm = &PD.oflog; // ConfigureLOG::Stream() = &PD.oflog;
   }

   // configure log stream
   ConfigureLOG::ReportLevels() = false;
   ConfigureLOG::ReportTimeTags() = false;
   // debug and verbose handled earlier in GetCommandLine/PreProcessArgs
   if(PD.debug > -1)
      ; // handled in CommandLine::PreProcessArgs()
   else if(PD.verbose)
      LOGlevel = ConfigureLOG::Level("VERBOSE");

   return 0;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
