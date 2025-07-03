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

#include "iobconverter.hpp"
#include "iobcmdline.hpp"
#include "Exception.hpp"
#include "LSASupportedToVersion.hpp"

//------------------------------------------------------------------------------------
using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
int ProcessCommandLine(int argc, char **argv)
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
   GlobalData& GD=GlobalData::Instance();

   // title
   if(pLOGstrm != &cout)
   {   LOG(INFO) << LSAVERSION_FILE_STRING;
       LOG(INFO) << "#" << GD.RunStr;
   }

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
int GetCommandLine(int argc, char **argv)
{
try {
   int i;
   GlobalData& GD=GlobalData::Instance();
   const string defaultstartStr("[Beginning of dataset]");
   const string defaultstopStr("[End of dataset]");
   string startStr(defaultstartStr);
   string stopStr(defaultstopStr);

   // create list of command line options, and fill it
   // put required options first - they will get listed first anyway
   CommandLine opts;

   // build the options list == syntax page
   string PrgmDesc =
      " Program " + GD.PrgmName + " will read an .iob file and convert it\n"
      "and included files (if any) to .lsa files. Input is on the command line.\n"
      "Accepted options are shown below, followed by a description,\n"
      "with default value, if any, in ().";

   // opts.Add(char, opt, arg, repeat?, required?, &target, pre-descript, descript.);
   // required options
   bool req(true);
   opts.Add('l', "lsa", "name", false, req, &GD.logfile, "\n# File I/O:",
            "Name of output .lsa file");
   opts.Add('i', "iob", "name", false, req, &GD.infile, "",
            "Name of input .iob file(s)");
   // optional args
   req = false;
   opts.Add(0, "lsapath", "path", false, req, &GD.logpath, "",
            "Path for output .lsa file");
   opts.Add(0, "iobpath", "path", false, req, &GD.inpath, "",
            "Path for input .iob file");
   opts.Add('w', "warn", "name", false, req, &GD.warnfile, "\n# File I/O:",
            "Name of output .wrn file");
   opts.Add(0, "warnpath", "path", false, req, &GD.warnpath, "",
            "Path for output .wrn file");
   opts.Add('p', "project", "", false, req, &GD.isProject, "",
            "Add include record for lsa option file");
   opts.Add('d', "projdir", "path", false, req, &GD.projDir, "",
            ".lsa files created within this directory");
   // Flow control
   // Data input
   // Editing
   // Output
   opts.Add(0, "validate", "", false, req, &GD.validate, "\n# Help:",
            "Read input and test its validity, then quit");
   opts.Add(0, "verbose", "", false, req, &GD.verbose, "",
            "Print extended output information");
   opts.Add(0, "debug", "", false, req, &GD.debug, "",
            "Print debug output at level 0 [debug<n> for level n=1-7]");
   opts.Add(0, "help", "", false, req, &GD.help, "",
            "Print this and quit");

   // add options that are ignored (true if it has an arg)

   // add deprecated (pseudonym) options (old,new)

   // --------------------------------------------------------------------------
   // declare it and parse it; write all errors to string GD.cmdlineErrors
   int iret = opts.ProcessCommandLine(argc,argv,PrgmDesc,
                         GD.cmdlineUsage,GD.cmdlineErrors,GD.cmdlineUnrecognized);
   if(iret == -2) return iret;      // bad alloc
   if(iret == -3) return iret;      // cmd line definition invalid

   // --------------------------------------------------------------------------
   // do extra parsing -- append errors to GD.cmdlineErrors
   //string msg;
   //vector<string> fields;
   ostringstream oss;

   // interpret one unrecognized argument as input file name
   if(GD.cmdlineUnrecognized.size() == 1 && GD.infile.empty()) {
      GD.infile = GD.cmdlineUnrecognized[0];
      GD.cmdlineUnrecognized.clear();
      //LOG(INFO) << " Input file: " << GD.infile;
   }

   // unrecognized arguments are an error
   if(GD.cmdlineUnrecognized.size() > 0) {
      oss << " Warning - unrecognized arguments:\n";
      for(i=0; i<GD.cmdlineUnrecognized.size(); i++)
         oss << GD.cmdlineUnrecognized[i] << "\n";
      oss << " End of unrecognized arguments.\n";
   }

   // --------------------------------------------------------------------------
   LOG(DEBUG) << GD.cmdlineUsage;  // this will contain list of args

   // dump it
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
int ValidateInput(void)
{
try {
   int i,j;
   string msg;
   GlobalData& GD=GlobalData::Instance();
   GD.inputIsValid = true;

   if(GD.validate) {
      LOG(INFO) << " ---- Validate configuration ----";
   }

   include_path(GD.inpath,GD.infile);
   expand_filename(GD.infile);
//   LOG(VERBOSE) << " Found input file name " << GD.infile;

   ifstream istrm;
   istrm.open(GD.infile.c_str(), ios::in);
   if(!istrm.is_open()) {
      msg = string("  Error - input file ") + GD.infile
          + string(" could not be opened.");
      LOG(ERROR) << msg;
      GD.inputIsValid = false;
   }
   else {
      istrm.clear();
      istrm.close();
      LOG(VERBOSE) << " Found input file " << GD.infile;
   }

   // where else to do this?
   include_path(GD.logpath,GD.logfile);
   expand_filename(GD.logfile);

   ofstream ostrm;
   ostrm.open(GD.logfile.c_str());
   if(!ostrm.is_open()) {
      msg = string("  Error - output file ") + GD.logfile
          + string(" could not be opened.");
      LOG(ERROR) << msg;
      GD.inputIsValid = false;
   }
   else {
      ostrm.clear();
      ostrm.close();
      LOG(VERBOSE) << " Found output file " << GD.logfile;
   }


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
int PreProcess(void)
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
   if(!GD.warnfile.empty()) {
      GD.ofwarn.open(GD.warnfile.c_str(),ios_base::out);
      if(!GD.ofwarn) {
         cerr << "Failed to open warn file " << GD.warnfile << endl;
         return 5;
      }
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
