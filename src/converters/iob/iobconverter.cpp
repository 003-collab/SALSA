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
/// @file lsasolver.cpp  ARL:UT least squares solver for LSA. Read *.dat file for
/// a priori information, unknowns and data, compute a priori values for unknowns
/// where necessary, and solve the LS problem.

//------------------------------------------------------------------------------------
#include "iobconverter.hpp"
#include "lsaUtils.hpp"
#include "IOBfile.hpp"//VSCACleanUp()
#include <set>
#include "LSAVersion.hpp"
#include "LSAFile.hpp"

//------------------------------------------------------------------------------------
using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string GlobalData::Version(LSAVERSION_MAJOR_MINOR_PATCH);

//------------------------------------------------------------------------------------
// prototypes
int ProcessCommandLine(int argc, char **argv) throw(Exception);
int ReadAndParseInputFile(IOBfile &iobfile) throw(Exception);
void cleanup(std::vector<IOBrecord *> &records, IOBfile &iobfile);

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
   IOBfile iobfile;

   // Build title
   Epoch ttag;
   ttag.setLocalTime();
   //comment in .lsa file
   GD.RunStr = GD.PrgmName + ", Ver. "
      + GD.Version + ttag.printf(", Run %04Y/%02m/%02d at %02H:%02M:%02S");
 
   // display title on screen
   cout << GD.RunStr <<endl;

   //initialize warning/error message booleans
   GD.conversionFailures = false;
   GD.parseFailures = false;
   GD.unsupportedRecords = false;
   GD.invalidUnits = false;
   GD.unsupportedCOVScaling = false;
   GD.commentedIncludes = false;
   GD.missingHFROM = false;

   //redirect standard log output to screen
   pLOGstrm = &cout;

   int iret;
   for(bool go=true; go; go=false)  {

      // --------------------------------------------------------------
      // process the command line
      iret = ProcessCommandLine(argc,argv);        // lsacmdline.cpp
      LOG(VERBOSE) << GD.cmdlineDump;
      if(iret) break;

      // read input file and write output file
      iret = ReadAndParseInputFile(iobfile);
      if(iret) break;
   }

   if(iret != 0) {
      if(iret != 1) {
         string msg;
         msg = GD.PrgmName + string(" is terminating with code ")
                           + asString(iret);
         LOG(ERROR) << msg;
      }

      if(iret == 1) { LOG(INFO) << GD.cmdlineUsage; }
      else if(iret == 2) { LOG(ERROR) << GD.cmdlineErrors; }
      else if(iret == 3) { LOG(ERROR) << "the user requested input validation."; }
      else if(iret == 4) { LOG(ERROR) << "the input is invalid."; }
      else if(iret == 5) { LOG(ERROR) << "the output file could not be opened."; }
      else if(iret == 7) { LOG(ERROR) << "the input file could not be opened."; }
      else if(iret == -3) { // cmd line definition invalid
         LOG(ERROR) << "the command line definition is invalid.\n" << GD.cmdlineErrors;
      }
      else                 // fix this
         LOG(ERROR) << "temp - Some other return code..." << iret;
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

   GD.oflog.close();
   GD.ofwarn.close();

   //fix to Bug #1200
   LOG(INFO).flush();
   LOG(VERBOSE).flush();
   LOG(ERROR).flush();

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
int ReadAndParseInputFile(IOBfile &iobfile) throw(Exception)
{
try {
   int i, cCounter=0, dsetCounter=0, vscaCounter=0, numCommentedVSCA=0;
   bool nonTrivialVSCA=false, isCommentedFile=false;

   GlobalData& GD=GlobalData::Instance();

   // read the iob file
   std::vector<IOBrecord *> records;
   std::vector<string> badrecs;
   std::vector<string> projectIncludedFiles;
   string cwd = getWorkingDirectory();//directory converter was invoked from
   if(pathIsRelative(GD.infile))
       include_path(cwd,GD.infile);
   if(pathIsRelative(GD.logfile))
       include_path(cwd,GD.logfile);
   string iobpath = getPathWithoutFileName(GD.infile);
   string lsapath = getPathWithoutFileName(GD.logfile);
   if(GD.projDir.empty())
       GD.projDir = lsapath;

   i = iobfile.ReadAndParseIOBfile(GD.infile, iobpath, lsapath, records,
                                   badrecs,cCounter, dsetCounter, vscaCounter,
                                   numCommentedVSCA, nonTrivialVSCA, GD.projDir,
                                   isCommentedFile, projectIncludedFiles);
   if(i < 0) return 7;              // could not open file

   // were there parsing errors?
   pLOGstrm = &GD.ofwarn;
   for(i=0; i<badrecs.size(); i++)
       LOG(INFO) << badrecs[i];
   pLOGstrm = &cout;

   //Replace COVMs with VSCAs and make include-level VSCAs apply to include records instead of measurement records
   cleanup(records,iobfile);

   //reset LOG stream to lsa file
   pLOGstrm = &GD.oflog;

    // write the lsa file
    for(i=0; i<records.size(); i++) {
        try
        {
            LOG(INFO) << records[i]->asLSAString();
        }
        catch(...)
        {
            pLOGstrm = &GD.ofwarn;
            LOG(INFO) << "Warning - problem converting > "
                 << records[i]->getSourceRecords() << " < from "
                 << records[i]->getSourceFile() << " to an LSA record." << endl;
            pLOGstrm = &GD.oflog;
            GD.conversionFailures = true;
        }
    }

    //load and save the lsa project/file to apply LSA spec precisions (c.f. Git Issue #124)
    LSAFile *lsaFile = new LSAFile(false);
    lsaFile->read(GD.logfile);
    lsaFile->save();

   //redirect LOG stream back to stdout
   pLOGstrm = &cout;

   if(GD.invalidUnits)
   {
       LOG(INFO) << "Warning - invalid units in IOB record(s).  See " << quoteIfSpaces(GD.warnfile) << " for details.";
   }
   if(GD.conversionFailures)
   {
       LOG(INFO) << "Warning - failure converting IOB record(s).  See " << quoteIfSpaces(GD.warnfile) << " for details.";
   }
   if(GD.parseFailures)
   {
       LOG(INFO) << "Warning - failure parsing IOB record(s).  See " << quoteIfSpaces(GD.warnfile) << " for details.";
   }
   if(GD.missingHFROM)
   {
       LOG(INFO) << "Warning - missing HI on IOB record(s).  See " << quoteIfSpaces(GD.warnfile) << " for details.";
   }
   if(GD.unsupportedRecords)
   {
       std::set<std::string> unsupRecTypes;
       for(i=0; i<badrecs.size(); i++)
       {
           if(badrecs[i].find("Unsupported record type") != string::npos)
           {
               std::string tmpStr(badrecs[i]);
               tmpStr = tmpStr.substr(tmpStr.find(" > "));
               std::vector<std::string> parts = gnsstk::StringUtils::split(tmpStr," ");
               if(unsupRecTypes.find(parts[1]) == unsupRecTypes.end())
                   unsupRecTypes.insert(parts[1]);
           }
       }
       ostringstream oss;
       oss << "Warning - unsupported IOB record types - ";
       std::set<std::string>::iterator it;
       for(it=unsupRecTypes.begin();it!=unsupRecTypes.end();it++)
           oss << *it << ", ";
       std::string tmpStr(oss.str());
       tmpStr = tmpStr.substr(0,tmpStr.size()-2) + std::string(".  See ") + quoteIfSpaces(GD.warnfile) + std::string(" for details.");
       LOG(INFO) << tmpStr;
   }
   if(GD.unsupportedCOVScaling)
   {
       cerr << "Error - unsupported COV/CORR scaling encountered.  See " << quoteIfSpaces(GD.warnfile) << " for details.";
   }
   if(GD.commentedIncludes)
   {
       LOG(INFO) << "Warning - commented IOB include record(s) not converted.  Use menu option 'Record->Insert Include from IOB' if desired in project.  See " << quoteIfSpaces(GD.warnfile) << " for details.";
   }

   return 0;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
void cleanup(std::vector<IOBrecord *> &records, IOBfile &iobfile)
{
    std::string VSCATag = iobfile.VSCACleanUp(records);
    //all children have same scaling.  Add TOPARENT tag to the VSCA record at the top level
    if(!VSCATag.empty())
    {
        for(int i=0;i<records.size();i++)
        {
            IOBtype recType = records.at(i)->getRecType();
            if(recType == IOBtype::VSCA && !records.at(i)->isCommented)
            {
                IOBVSCA *vscaRecord = static_cast<IOBVSCA*>(records.at(i));
                vscaRecord->applyToParent = true;
                break;
            }
        }
    }
}

//------------------------------------------------------------------------------------
//There are cases when parsing an IOB record will not cause a problem
//but writing the LSA record does.  At this point, information on the IOB record(s) aren't
//attached.  This is a patch to deal with this until a more robust method can be implemented.
//TBD: Need to either have a validation method for each record upon parsing OR
//carry original record(s) and file information around with each record so that it
//can be referenced when calling the asLSAString methods.
/*
void outputWriteWarning(int record_index, std::vector<IOBrecord *> records)
{
    if(record_index==0)
        cout << "Error writing first LSA record" << endl;
    else
    {
        try
        {
            cout << "Error writing LSA record following > " << records[record_index-1]->asLSAString() << " <" << endl;
        }
        catch(...)
        {
            outputWriteWarning(record_index-1,std::vector<IOBrecord *> records);
        }
    }
}
*/
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
