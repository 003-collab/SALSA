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
/// @file lsapreprocessor.cpp  ARL:UT least squares solver for LSA. Read *.dat file for
/// a priori information, unknowns and data, compute a priori values for unknowns
/// where necessary, and solve the LS problem.

//------------------------------------------------------------------------------------
#include "lsapreprocessor.hpp"
#include "LSAVersion.hpp"
#include "expandpath.hpp"
#include <fstream>
#include <cstdio>
//------------------------------------------------------------------------------------
using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string PreprocessorData::Version(LSAVERSION_MAJOR_MINOR_PATCH);

//------------------------------------------------------------------------------------
// prototypes
// lsacmdline.cpp
int ProcessCommandLine(int argc, char **argv) throw(Exception);
int ReadInputFile(void) throw(Exception);
int WriteDatFile(void) throw(Exception);

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
   PreprocessorData& PD=PreprocessorData::Instance();
   PrgmName = PD.PrgmName;

   // Build title
   Epoch ttag;
   ttag.setLocalTime();
   PD.RunStr = PD.PrgmName + ", Ver. "
      + PD.Version + ttag.printf(", Run %04Y/%02m/%02d at %02H:%02M:%02S");
 
   // display title on screen
   LOG(INFO) << PD.RunStr;

   // remove any log file that may have been created by the GUI in a previous run
   std::string outfile("preprocessor.out");
   include_lsapath(getPathWithoutFileName(PD.lsaFilename),outfile);
   try
   {
       if(std::remove(outfile.c_str()) == 0)
           cout << "Successfully removed " << outfile << std::endl;
   }
   catch(...)
   {
       cout << "failure to remove " << outfile << std::endl;
       cout << strerror(errno) << std::endl;
   }


   // TEMP, for debugging CommandLine;
   //LOGlevel = ConfigureLOG::Level("DEBUG");

   // process : loop once -----------------------------------------------------
   int i,j;
   string label;
   ostringstream oss;
   int iret;
   for(bool go=true; go; go=false)  {

      // --------------------------------------------------------------
      // process the command line
      iret = ProcessCommandLine(argc,argv);        // lsacmdline.cpp
      LOG(VERBOSE) << PD.cmdlineDump;
      if(iret) break;

      // --------------------------------------------------------------
      // read input LSA file
      iret = ReadInputFile();
      if(iret) break;

      // --------------------------------------------------------------
      // write the output .dat file
      iret = WriteDatFile();
      if(iret) break;

   }  // end loop once

   // error condition ---------------------------------------------------------
   // return codes: 0 ok
   //               1 help
   //               2 cmd line errors
   //               3 requested validation
   //               4 invalid input
   //               5 open output fail
   //               6 requested dump
   //               7 open input failed
   //               8 apquit
   //               9 underdetermined
   //              -3 cmd line definition invalid (CommandLine)
   //LOG(INFO) << "Return code is " << iret;
   if(iret != 0) {
      if(iret != 1) {
         string msg;
         msg = string("Error - ") + PD.PrgmName + string(" is terminating with code ")
                           + asString(iret);
         LOG(ERROR) << msg;
      }

      if(iret == 1) { LOG(INFO) << PD.cmdlineUsage; }
      else if(iret == 2) { LOG(INFO) << PD.cmdlineErrors; }
      else if(iret == 3) { LOG(INFO) << "The user requested input validation."; }
      else if(iret == 4) { LOG(INFO) << "The input is invalid."; }
      else if(iret == 5) { LOG(INFO) << "The output file could not be opened."; }
      else if(iret == 7) { LOG(INFO) << "The input file could not be opened."; }
      else if(iret == 8) { LOG(INFO) << "The user requested early exit."; }
      else if(iret == 9) { LOG(INFO) << "The problem is underdetermined."; }
      else if(iret == 10) { LOG(INFO) << "The problem is singular."; }
      else if(iret == -3) { // cmd line definition invalid
         LOG(INFO) << "The command line definition is invalid.\n" << PD.cmdlineErrors;
      }
      else if(iret == -1) { LOG(INFO) << "The problem is singular."; }
      else                 // fix this
         LOG(INFO) << "temp - Some other return code..." << iret;
   }

   // compute and print run time ----------------------------------------------
   if(iret != 1) {
      wallend.setLocalTime();
      totaltime = clock()-totaltime;
      ostringstream oss;
      oss << endl << PrgmName << " timing: " << fixed << setprecision(3)
         << double(totaltime)/double(CLOCKS_PER_SEC)
         << " seconds. (" << (wallend - wallbegin) << " sec)";
      if(pLOGstrm != &cout) LOG(INFO) << oss.str();
      cout << oss.str() << endl;
   }

   if(iret == 0) return 0; else return -1;
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
int ReadInputFile(void) throw(Exception)
{
try {
   PreprocessorData& PD=PreprocessorData::Instance();

   // read the LSA file
   if (!PD.lsaFile->read(PD.lsaFilename))
   {
       return 7; // could not open file
   }

   return 0;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
int WriteDatFile(void) throw(Exception)
{
try
{
    int i;
    PreprocessorData& PD=PreprocessorData::Instance();

    i = PD.lsaFile->writeDatFile(PD.datFilename);
    if(i<0) return 5;

    return 0;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}



//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
