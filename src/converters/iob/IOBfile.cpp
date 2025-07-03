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
/// @file IOBfile.cpp  File I/O for *.iob file: read and parse, write, and fill
///                    IOB* objects.

#include <string>
#include <ostream>
#include <fstream>
#include <exception>
#include <ctype.h>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "IOBfile.hpp"
#include "logstream.hpp"         // TEMP
#include "iobUtils.hpp"
#include "lsaUtils.hpp"
#include "expandpath.hpp"
#include "iobconverter.hpp"//for GlobalData
#include "LSASupportedToVersion.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;


// -----------------------------------------------------------------------------------
// Open, read and parse a .iob file, filling a vector of pointers to IOBrecord.
// param filename file name
// param recPtrs a vector of pointers of 'IOBrecord *' containing records
//           read.
// param badrecs return vector<string> of records that could not be parsed
// param cCounter reference to int of counter for COV/CORR records
// param dsetCounter reference to int of counter for DSET records
// param vscaCounter reference to int of counter for VSCA records
// param numCommentedVSCA reference to int of counter for commented VSCA records
// param projDir reference to string of directory above which .lsa files should not be created (if empty, ignore)
// param isCommentedFile reference to bool to indicate whether the #include for the file was commented out
// param projectIncludedFiles vector of string of absolute file paths to included files (including the parent)
// return number of records stored, or an error code
// throw if file cannot be opened
int IOBfile::ReadAndParseIOBfile(const string& filename,
                                 const string& iobpath,
                                 const string& lsapath,
                                 vector<IOBrecord *>& recPtrs,
                                 vector<string>& badrecs,
                                 int &cCounter, int &dsetCounter,
                                 int &vscaCounter, int &numCommentedVSCA,
                                 bool &applyVSCA,
                                 string& projDir, bool isCommentedFile,
                                 vector<string>& projectIncludedFiles)
{
   int line_num=0;
   int i,j, ret=0;
   string::size_type pos;
   bool applyIncludeLevelVSCA = false;//temp variable to get include-level VSCAs to work with commented includes
   bool unsupportedCOVScaling = false;

   GlobalData& GD=GlobalData::Instance();

   ifstream istrm;
   istrm.open(filename.c_str(), ios::in);
   if(!istrm.is_open())
      return -1;

// ?? recPtrs.clear();
//took this out to use badrecs to capture all parsing errors
//all along the include tree
   //   badrecs.clear();

   //keep list of files included in the project, including the top-level file
   if(std::find(projectIncludedFiles.begin(),
                projectIncludedFiles.end(),
                filename) == projectIncludedFiles.end())
   {
       projectIncludedFiles.push_back(filename);
   }

   // read loop
   string line, word, SIGM_label, lastCrecord, ELEMrecords[4], lastDXYZrecord, lastDSETrecord;
   string last_rec_type, last_line, lastPLHrecord, lastXYZrecord;
   int numELEMrecords = 0;
   int requiredELEMRecords = 0;
   bool wroteDGRP=false;
   bool isCommentedLine=false;
   bool lastDSETisCommented = false;

   while(1) {
      line = string("");
      getline(istrm,line);
      stripTrailing(line,'\r');
      line_num++;

      if((istrm.eof() || !istrm.good()) && line.empty()) break;

      IOBrecord *dptr;
      bool ok = false, isConfig = false;

      isCommentedLine = checkForCommentedIOBRecord(line);

      try
      {
          if(isAllWhiteSpace(line))
          {
              //passthrough blank lines to .lsa file
              IOBComment *ptr = new IOBComment();
              ok = ptr->fromString(std::string("\n"));
              if(ok)
              {
                  dptr = dynamic_cast<IOBrecord *>(ptr);
                  dptr->setSourceFile(filename);
                  dptr->setSourceRecords(line);
              }
              else
                  ok = false;
          }
          //First character of a valid record is a space
          else if((line[0] == ' ') || isCommentedLine || ((line.length()>=8) && (gnsstk::StringUtils::lowerCase(line.substr(0,8))==string("#include"))))
          {
             //characters 1-4 specify the record type
             word = getRecordType(line);

             //disregard previous record block if missing an ELEM record
             if(numELEMrecords>0 && word != string("ELEM"))
             {
                 badrecs.push_back(std::string("Warning - missing record in block before > ") + line +
                                          std::string(" < at line ") + gnsstk::StringUtils::asString(line_num) +
                                          std::string(" in file ") + filename + std::string(".\n"));
                 numELEMrecords = 0;
                 lastCrecord = string("");
             }

             if(word == string("PLH")) {
                if((last_rec_type = getRecordType(last_line))==string("3DC")) {
                   lastPLHrecord = line;
                   last_line = line;
                   continue;
                }
                else {//isolated record
                   IOBPLH *ptr = new IOBPLH();
                   ok = ptr->fromString(line);
                   if(ok)
                   {
                        dptr = dynamic_cast<IOBrecord *>(ptr);
                        dptr->setSourceFile(filename);
                        dptr->setSourceRecords(line);
                   }
                }
             }
             else if(word == string("XYZ")) {
                if((last_rec_type = getRecordType(last_line))==string("3DC")) {
                   lastXYZrecord = line;
                   last_line = line;
                   continue;
                }
                else {//isolated record
                   IOBXYZ *ptr = new IOBXYZ();
                   ok = ptr->fromString(line);
                   if(ok)
                   {
                        dptr = dynamic_cast<IOBrecord *>(ptr);
                        dptr->setSourceFile(filename);
                        dptr->setSourceRecords(line);
                   }
                }
             }
             else if(word == string("SIGM")) {
                IOBSIGM *ptr = new IOBSIGM();
                ok = ptr->fromString(line);
                if(ok)
                {
                    if(!(isCommentedLine || isCommentedFile) &&
                       !uniqifySIGMLabel(ptr))
                        continue;//record is a duplicate, don't write it to the .lsa file
                    dptr = dynamic_cast<IOBrecord *>(ptr);
                    dptr->setSourceFile(filename);
                    dptr->setSourceRecords(line);
                }
             }
             else if(word == string("3DC")) {
                //removed support for SIGMs in PLH/XYZ records on Dec 15, 2015
                last_line = line;
                continue;
             }
             else if(word == string("3DD")) {
                std::vector<std::string> words = splitWithDoubleQuotes(line, ' ');
                if(words.size() > 1)
                {
                    SIGM_label = words[1];
                    std::map<std::string,IOBSIGM*>::iterator it = SIGMRecords.find(SIGM_label);
                    if(it!=SIGMRecords.end())
                        SIGM_label = it->second->label;
                }
                else
                   SIGM_label = "";
                last_line = line;
                continue;
             }
             else if(word == string("DXYZ")) {
                lastDXYZrecord = line;
                last_line = line;
                continue;
             }
             else if(word == string("DIST")) {
                IOBDIST *ptr = new IOBDIST();
                ok = ptr->fromString(line,applyVSCA,vscaCounter-1,instrument_HGHT_recs,target_HGHT_recs,SIGMRecords);
                if(ok)
                {
                    dptr = dynamic_cast<IOBrecord *>(ptr);
                    dptr->setSourceFile(filename);
                    dptr->setSourceRecords(line);
                }
             }
             else if(word == string("ANGL")) {
                IOBANGL *ptr = new IOBANGL();
                ok = ptr->fromString(line,applyVSCA,vscaCounter-1,target_HGHT_recs,SIGMRecords);
                if(ok)
                {
                    dptr = dynamic_cast<IOBrecord *>(ptr);
                    dptr->setSourceFile(filename);
                    dptr->setSourceRecords(line);
                }
             }
             else if(word == string("VANG") || word == string("GVAN")) {
                IOBVANG *ptr = new IOBVANG();
                ok = ptr->fromString(line,applyVSCA,vscaCounter-1,instrument_HGHT_recs,target_HGHT_recs,SIGMRecords);
                if(ok)
                {
                    dptr = dynamic_cast<IOBrecord *>(ptr);
                    dptr->setSourceFile(filename);
                    dptr->setSourceRecords(line);
                }
             }
             else if(word == string("ZANG") || word == string("GZAN")) {
                IOBZANG *ptr = new IOBZANG();
                ok = ptr->fromString(line,applyVSCA,vscaCounter-1,instrument_HGHT_recs,target_HGHT_recs,SIGMRecords);
                if(ok)
                {
                    dptr = dynamic_cast<IOBrecord *>(ptr);
                    dptr->setSourceFile(filename);
                    dptr->setSourceRecords(line);
                }
             }
             else if(word == string("AZIM") || word == string("GAZI")) {
                IOBAZIM *ptr = new IOBAZIM();
                ok = ptr->fromString(line,applyVSCA,vscaCounter-1,instrument_HGHT_recs,target_HGHT_recs,SIGMRecords);
                if(ok)
                {
                    dptr = dynamic_cast<IOBrecord *>(ptr);
                    dptr->setSourceFile(filename);
                    dptr->setSourceRecords(line);
                }
             }
             else if(word == string("EHDF") || word == string("OHDF")) {
                IOBHDIF *ptr = new IOBHDIF();
                ok = ptr->fromString(line,applyVSCA,vscaCounter-1,SIGMRecords);
                if(ok)
                {
                    dptr = dynamic_cast<IOBrecord *>(ptr);
                    dptr->setSourceFile(filename);
                    dptr->setSourceRecords(line);
                }
             }
             else if(word == string("DIR")) {
                IOBHDIR *ptr = new IOBHDIR();
                ok = ptr->fromString(line,lastDSETrecord,dsetCounter,wroteDGRP,lastDSETisCommented,applyVSCA,vscaCounter-1,target_HGHT_recs,SIGMRecords);
                if(ok)
                {
                    dptr = dynamic_cast<IOBrecord *>(ptr);
                    dptr->setSourceFile(filename);
                    dptr->setSourceRecords(line);
                }
                if(!wroteDGRP)//only write DGRP record prior to first HDIR record in the group
                   wroteDGRP = true;
             }
             else if(word == string("HI")) {
                if(isCommentedLine || isCommentedFile)
                {
                    IOBHGHT *ptr = new IOBHGHT();
                    ok = ptr->fromString(line, all_HGHT_recs);
                    if(ok)
                    {
                        dptr = dynamic_cast<IOBrecord *>(ptr);
                        dptr->setSourceFile(filename);
                        dptr->setSourceRecords(line);
                    }
                }
                else if(updateInstrumentHGHTrecs(line))
                {
                    ok = true;
                    IOBHGHT *ptr = new IOBHGHT(instrument_HGHT_recs[instrument_HGHT_recs.size()-1]);
                    dptr = dynamic_cast<IOBrecord *>(ptr);
                    dptr->setSourceFile(filename);
                    dptr->setSourceRecords(line);
                }
                else {
                   last_line = line;
                   continue;
                }
             }
             else if(word == string("HT")) {
                if(isCommentedLine || isCommentedFile)
                {
                    IOBHGHT *ptr = new IOBHGHT();
                    ok = ptr->fromString(line, all_HGHT_recs);
                    if(ok)
                    {
                        dptr = dynamic_cast<IOBrecord *>(ptr);
                        dptr->setSourceFile(filename);
                        dptr->setSourceRecords(line);
                    }
                }
                else if(updateTargetHGHTrecs(line))
                {
                    ok = true;
                    IOBHGHT *ptr = new IOBHGHT(target_HGHT_recs[target_HGHT_recs.size()-1]);
                    dptr = dynamic_cast<IOBrecord *>(ptr);
                    dptr->setSourceFile(filename);
                    dptr->setSourceRecords(line);
                }
                else {
                   last_line = line;
                   continue;
                }
             }
             else if(word == string("DSET")) {
                dsetCounter++;
                lastDSETrecord = line;
                if(isCommentedLine)
                    lastDSETisCommented = true;
                else
                    lastDSETisCommented = false;
                wroteDGRP = false;
                continue;
             }
             else if(word == string("VSCA")) {
                if(std::fabs(asDouble(splitWithDoubleQuotes(line,' ')[1]) - 1.0) > 1.e-15) {//non-trivial scaling
                   IOBVSCA *ptr = new IOBVSCA();
                   ok = ptr->fromString(line,vscaCounter+numCommentedVSCA);
                   if(isCommentedFile || isCommentedLine)
                   {
                       applyVSCA = false;
                       numCommentedVSCA++;
                   }
                   else
                   {
                       vscaCounter += numCommentedVSCA+1;
                       numCommentedVSCA=0;
                       applyVSCA = true;
                   }
                   if(ok)
                   {
                       string new_key(ptr->label);
                       VSCARecords.insert(std::pair<std::string, IOBVSCA*>(new_key, ptr));
                       dptr = dynamic_cast<IOBrecord *>(ptr);
                       dptr->setSourceFile(filename);
                       dptr->setSourceRecords(line);
                   }
                }
                else {
                   applyVSCA = false;
                   continue;
                }
             }
             else if(word == string("COV")) {
                lastCrecord = line;
                cCounter++;
                last_rec_type = getRecordType(last_line);
                last_line = line;
                string mat_form = getMatrixForm(line);
                if(mat_form == std::string("UPPR"))
                {
                    requiredELEMRecords = 3;
                    continue;
                }
                else if(mat_form == std::string("DIAG"))
                {
                    requiredELEMRecords = 1;
                    continue;
                }
                else//invalid matrix form
                {
                    requiredELEMRecords = 0;
                    throw IOBException(line);
                }
             }
             else if(word == string("CORR")) {
                lastCrecord = line;
                cCounter++;
                last_rec_type = getRecordType(last_line);
                last_line = line;
                string mat_form = getMatrixForm(line);
                if(mat_form == std::string("UPPR"))
                {
                    requiredELEMRecords = 4;
                    continue;
                }
                else if(mat_form == std::string("DIAG"))
                {
                    requiredELEMRecords = 2;
                    continue;
                }
                else//invalid matrix form
                {
                    requiredELEMRecords = 0;
                    throw IOBException(line);
                }
             }
             else if(word == string("ELEM")) {
                ELEMrecords[numELEMrecords] = line;
                numELEMrecords++;
                string mat_type = getRecordType(lastCrecord);
                string mat_form = getMatrixForm(lastCrecord);

                if(numELEMrecords == requiredELEMRecords) {
                   string cmat = parseELEMRecords(mat_type,mat_form,numELEMrecords,ELEMrecords);
                   string crec = parseCOVCORRRecords(lastCrecord,cCounter,applyVSCA,vscaCounter-1,VSCARecords,unsupportedCOVScaling);
                   if(last_rec_type == string("PLH")) {
                      IOBPLH *ptr = new IOBPLH();
                      ok = ptr->fromString(lastPLHrecord,crec,cmat,applyVSCA,vscaCounter-1);
                      if(ok)
                      { 
                          dptr = dynamic_cast<IOBrecord *>(ptr);
                          dptr->setSourceFile(filename);
                          string sourceRecords = lastPLHrecord + string("\n") + lastCrecord + "\n";
                          for(int i=0;i<numELEMrecords;i++)
                            sourceRecords += ELEMrecords[i] + string("\n");
                          dptr->setSourceRecords(line);
                          if(unsupportedCOVScaling)
                          {
                              badrecs.push_back(std::string("Error - unsupported COV/CORR scaling > ") + sourceRecords +
                                                   std::string(" < near line ") + gnsstk::StringUtils::asString(line_num) +
                                                   std::string(" in file ") + filename);
                              GD.unsupportedCOVScaling = true;
                              unsupportedCOVScaling = false;
                          }
                      }
                   }
                   else if(last_rec_type == string("XYZ")) {
                      IOBXYZ *ptr = new IOBXYZ();
                      ok = ptr->fromString(lastXYZrecord,crec,cmat,applyVSCA,vscaCounter-1);
                      if(ok)
                      { 
                          dptr = dynamic_cast<IOBrecord *>(ptr);
                          dptr->setSourceFile(filename);
                          string sourceRecords = lastXYZrecord + string("\n") + lastCrecord + "\n";
                          for(int i=0;i<numELEMrecords;i++)
                            sourceRecords += ELEMrecords[i] + string("\n");
                          dptr->setSourceRecords(line);
                          if(unsupportedCOVScaling)
                          {
                              badrecs.push_back(std::string("Error - unsupported COV/CORR scaling > ") + sourceRecords +
                                                   std::string(" < near line ") + gnsstk::StringUtils::asString(line_num) +
                                                   std::string(" in file ") + filename);
                              GD.unsupportedCOVScaling = true;
                              unsupportedCOVScaling = false;
                          }
                      }
                   }
                   else if(last_rec_type == string("DXYZ")) {
                      IOBDXYZ *ptr = new IOBDXYZ();
                      ok = ptr->fromString(lastDXYZrecord,SIGM_label,crec,cmat,applyVSCA,vscaCounter-1);
                      if(ok)
                      { 
                          dptr = dynamic_cast<IOBrecord *>(ptr);
                          dptr->setSourceFile(filename);
                          string sourceRecords = lastDXYZrecord + string("\n") + lastCrecord + "\n";
                          for(int i=0;i<numELEMrecords;i++)
                            sourceRecords += ELEMrecords[i] + string("\n");
                          dptr->setSourceRecords(line);
                          if(unsupportedCOVScaling)
                          {
                              badrecs.push_back(std::string("Error - unsupported COV/CORR scaling > ") + sourceRecords +
                                                   std::string(" < near line ") + gnsstk::StringUtils::asString(line_num) +
                                                   std::string(" in file ") + filename);
                              GD.unsupportedCOVScaling = true;
                              unsupportedCOVScaling = false;
                          }
                      }
                   }
                   numELEMrecords = 0;
                   lastCrecord = string("");
                }
                else {
                   last_line = line;
                   continue;
                }
             }
             else if ((line.length()>=8) && (gnsstk::StringUtils::lowerCase(line.substr(0,8))==string("#include")))
             {
                IOBInclude *ptr = new IOBInclude();
                ok = ptr->fromString(line);                
                if(ok)
                {
                    //TBD: get_lsapath() doesn't end with a delimiter, but getCanonicalPath expects one.  This should be resolved.  Otherwise, hacks, like below, are required.
                    string delimiter("/");

                    //get the relative path of the included IOB file to its parent
                    string IOBProjectPath = iobpath + delimiter;
                    string IOBParentPath = get_lsapath_with_delimiter(filename);
                    string IOBChildPath = ptr->includefile;
                    string tmpPath;
                    if(pathIsRelative(ptr->includefile))
                        include_path(IOBParentPath,IOBChildPath);
                    IOBChildPath = getCanonicalPath(IOBChildPath);//dwt

                    //check if include is commented
                    if(isCommentedLine)
                    {
                        badrecs.push_back(std::string("Warning - the commented #include file ") + addQuotes(ptr->includefile) +
                                             std::string(" in ") + filename + std::string(" at line ") + gnsstk::StringUtils::asString(line_num) +
                                             std::string(" will not be converted.\n"));
                        GD.commentedIncludes = true;
                        IOBComment *ptr = new IOBComment();
                        ok = ptr->fromString(line);
                        if(ok)
                        {
                            dptr = dynamic_cast<IOBrecord *>(ptr);
                            dptr->setSourceFile(filename);
                            dptr->setSourceRecords(line);
                        }
                        else
                           ok = false;

                    }
                    //check if file has already been included
                    else if(std::find(projectIncludedFiles.begin(),
                                 projectIncludedFiles.end(),
                                 IOBChildPath) != projectIncludedFiles.end())
                    {
                        pLOGstrm = &GD.ofwarn;
                        LOG(INFO) << "The #include file " << IOBChildPath << " is already included.  Commenting out redundant #include." << endl;
                        pLOGstrm = &GD.oflog;
                        IOBComment *ptr = new IOBComment();
                        ok = ptr->fromString(string("--include")+line.substr(8,line.size()-8));
                        if(ok)
                        {
                            dptr = dynamic_cast<IOBrecord *>(ptr);
                            dptr->setSourceFile(filename);
                            dptr->setSourceRecords(string("--include")+line.substr(8,line.size()-8));
                        }
                        else
                           ok = false;
                    }
                    else
                    {
                        tmpPath = formatIncludePath(IOBProjectPath,IOBParentPath,IOBChildPath);

                        //reset the IOB #include path to be a relative path
                        if(!pathIsRelative(tmpPath))//included file outside IOB project folder
                            ptr->includefile = get_file(tmpPath);
                        else
                            ptr->includefile = tmpPath;

                        //get the corresponding LSA paths
       //                 string LSAParentPath = getRelativePath(IOBProjectPath,get_lsapath_with_delimiter(filename));//dwt
                        string LSAParentPath = getRelativePath(IOBProjectPath,getPathWithoutFileName(filename));
                        include_lsapath(projDir,LSAParentPath);
                        string LSAChild = ptr->includefile + string(".lsa");
                        if(pathIsRelative(LSAChild))
                        {     //this cuts off subdirectories in LSAChild!
       //                     include_lsapath(LSAParentPath,LSAChild);//dwt
                            if(LSAParentPath.rfind(delimiter)==LSAParentPath.size()-1)
                                LSAChild = LSAParentPath + LSAChild;
                            else
                                LSAChild = LSAParentPath + delimiter + LSAChild;
                        }
                        LSAChild = getCanonicalPath(LSAChild);

                        //create new directories, if necessary
                        string LSAChildPath = get_lsapath_with_delimiter(LSAChild);
                        CreateLSADir(LSAChildPath);

                        //TBD: create different .lsa file if old one exists, and modify include path
                        //JUST OVERWRITES FOR NOW
           /*
                        ifstream check(LSAChild.c_str());
                        if(check)
                        {
                            char rnd[4];
                            gen_random(rnd,3);
                            cout << "Warning - the file " << LSAChild << " already exists." << endl;
                            string tmp = get_file(ptr->includefile)+ string("_") + string(rnd);
                            include_path(get_lsapath(ptr->includefile),tmp);
                            ptr->includefile = tmp;
                            expand_filename(ptr->includefile);
                            LSAChild = ptr->includefile + ".lsa";
                            cout << "         Creating " << LSAChild << " instead." << endl;
                        }
           */
                        int i;

                        // read the iob file
                        std::vector<IOBrecord *> included_records;
                        std::ofstream oflsa;

                        included_records.clear();
                        applyIncludeLevelVSCA = applyVSCA;
                        i = ReadAndParseIOBfile(IOBChildPath, iobpath, lsapath, included_records,
                                                        badrecs, cCounter, dsetCounter, vscaCounter, numCommentedVSCA,
                                                        applyVSCA, projDir, isCommentedFile || isCommentedLine,
                                                        projectIncludedFiles);
                        applyVSCA = applyIncludeLevelVSCA;
                        if(i<0)
                        {
                            if(!(isCommentedLine || isCommentedFile))
                                badrecs.push_back(std::string("Warning - could not open included file: ") + addQuotes(IOBChildPath) +
                                                     std::string(". File included at line ") + gnsstk::StringUtils::asString(line_num) +
                                                     std::string(" in file ") + filename + std::string(".\n"));
                        }
                        else
                        {
                            //reset LOG stream to lsa file
                            //open the output file
                            oflsa.open(LSAChild.c_str(), ios::out);

                            if(!oflsa.is_open())
                            {
                                cerr << "The converted #include file " << LSAChild << " could not be opened." << endl;
                            }
                            else
                            {
                                oflsa << LSAVERSION_FILE_STRING << endl;

                                //check if all records (measurement or includes) have the same VSCA applied.
                                //If so, remove the VSCA references from the measurements include and add the tag to this
                                //iob file
                                string cleanedUpVSCATag = VSCACleanUp(included_records);
                                if(!cleanedUpVSCATag.empty())
                                    ptr->VSCATag = cleanedUpVSCATag;

                                // write the lsa file
                                for(i=0; i<included_records.size(); i++)
                                {
                                   try
                                   {
                                       oflsa << included_records[i]->asLSAString() << endl;
                                   }
                                   catch(...)
                                   {
                                       pLOGstrm = &GD.ofwarn;
                                       LOG(INFO) << "Warning - problem converting > "
                                            << included_records[i]->getSourceRecords() << " < from "
                                            << included_records[i]->getSourceFile() << " to an LSA record." << endl;
                                       pLOGstrm = &GD.oflog;
                                       GD.conversionFailures = true;
                                   }
                                }

                                oflsa.close();
                            }
                         }

                         dptr = dynamic_cast<IOBrecord *>(ptr);
                         dptr->setSourceFile(filename);
                         dptr->setSourceRecords(line);
                     }
                 }
                 else
                    ok = false;
             }
             else if (word.find(' ') != string::npos) {
                 //passthrough comments and blank lines to .lsa file
                 IOBComment *ptr = new IOBComment();
                 //getline() removes \n, but we want to preserve whitespace
                 ok = ptr->fromString(std::string("\n"));
                 if(ok)
                 {
                     dptr = dynamic_cast<IOBrecord *>(ptr);
                     dptr->setSourceFile(filename);
                     dptr->setSourceRecords(line);
                 }
                 else
                    ok = false;
             }
             else
                ok = false;
          }
          else {
             //passthrough comments and blank lines to .lsa file
             IOBComment *ptr = new IOBComment();
             //getline() removes \n, but we want to preserve whitespace
             if(line.empty())
                ok = ptr->fromString(std::string("\n"));
             else
             ok = ptr->fromString(line);
             if(ok)
             {
                 dptr = dynamic_cast<IOBrecord *>(ptr);
                 dptr->setSourceFile(filename);
                 dptr->setSourceRecords(line);
             }
             else
                ok = false;
          }
      }
      catch(IOBException& e)
      {
          //don't warn users for failure to parse a commented out record
          if(isCommentedLine || isCommentedFile)
          {
              //passthrough comments and blank lines to .lsa file
              IOBComment *ptr = new IOBComment();
              //getline() removes \n, but we want to preserve whitespace
              if(line.empty())
                 ok = ptr->fromString(std::string("\n"));
              else
              ok = ptr->fromString(line);
              if(ok)
              {
                  dptr = dynamic_cast<IOBrecord *>(ptr);
                  dptr->setSourceFile(filename);
                  dptr->setSourceRecords(line);
                  if(!isConfig) recPtrs.push_back(dptr);
              }
          }
          else
          {
              badrecs.push_back(std::string("Warning - failure to parse > ") + e.record +
                                   std::string(" < near line ") + gnsstk::StringUtils::asString(line_num) +
                                   std::string(" in file ") + filename + std::string(".\n"));
              GD.parseFailures = true;
          }
          last_line = line;
          continue;
      }
      catch(...)
      {
          //don't warn users for failure to parse a commented out record
          if(isCommentedLine || isCommentedFile)
          {
              //passthrough comments and blank lines to .lsa file
              IOBComment *ptr = new IOBComment();
              //getline() removes \n, but we want to preserve whitespace
              if(line.empty())
                 ok = ptr->fromString(std::string("\n"));
              else
              ok = ptr->fromString(line);
              if(ok)
              {
                  dptr = dynamic_cast<IOBrecord *>(ptr);
                  dptr->setSourceFile(filename);
                  dptr->setSourceRecords(line);
                  if(!isConfig) recPtrs.push_back(dptr);
              }
          }
          else
          {
              badrecs.push_back(std::string("Warning - failure to parse > ") + line +
                                  std::string(" < at line ") + gnsstk::StringUtils::asString(line_num) +
                                  std::string(" in file ") + filename + std::string(".\n"));
              GD.parseFailures = true;
          }
         last_line = line;
         continue;
      }

      if(ok)
      {
         if(isCommentedLine || isCommentedFile)
             dptr->isCommented = true;
         if(!isConfig) recPtrs.push_back(dptr);
      }
      else
      {
         //don't warn users for failure to parse a commented out unsupported record type
         if(isCommentedLine || isCommentedFile)
         {
             //passthrough comments and blank lines to .lsa file
             IOBComment *ptr = new IOBComment();
             //getline() removes \n, but we want to preserve whitespace
             if(line.empty())
                ok = ptr->fromString(std::string("\n"));
             else
             ok = ptr->fromString(line);
             if(ok)
             {
                 dptr = dynamic_cast<IOBrecord *>(ptr);
                 dptr->setSourceFile(filename);
                 dptr->setSourceRecords(line);
                 if(!isConfig) recPtrs.push_back(dptr);
             }
         }
         else
         {
             badrecs.push_back(std::string("Warning - unsupported record type > ") + line +
                                  std::string(" < at line ") + gnsstk::StringUtils::asString(line_num) +
                                  std::string(" in file ") + filename + std::string("."));
             GD.unsupportedRecords = true;
         }
      }
      last_line = line;
   }  // end read loop

   return recPtrs.size();
}

// -----------------------------------------------------------------------------------
// Add new entry into the HGHT_recs vector, if unique station/value combination
// @return true if new, false if duplicate record exists
bool IOBfile::updateTargetHGHTrecs(const string& line)
{
    IOBHGHT *ptr = new IOBHGHT();
    if(ptr->fromString(line,all_HGHT_recs))//new in all_HGHT_recs
    {
        target_HGHT_recs.push_back(ptr->text);
        all_HGHT_recs.push_back(ptr->text);
        return true;
    }
    else if(!ptr->text.empty())//duplicate in all_HGHT_recs, check if in target_HGHT_recs
    {
        IOBHGHT *ptr2 = new IOBHGHT();
        if(ptr2->fromString(line,target_HGHT_recs))//new in target_HGHT_recs
        {
            target_HGHT_recs.push_back(ptr->text);
            return false;//don't add new HGHT record
        }
        else
            return false;//duplicate in target_HGHT_recs
    }
    else
    {
        return false;
    }
}

bool IOBfile::updateInstrumentHGHTrecs(const string& line)
{
    IOBHGHT *ptr = new IOBHGHT();
    if(ptr->fromString(line,all_HGHT_recs))
    {
        instrument_HGHT_recs.push_back(ptr->text);
        all_HGHT_recs.push_back(ptr->text);
        return true;
    }
    else if(!ptr->text.empty())//duplicate in all_HGHT_recs, check if in instrument_HGHT_recs
    {
        IOBHGHT *ptr2 = new IOBHGHT();
        if(ptr2->fromString(line,instrument_HGHT_recs))//new in instrument_HGHT_recs
        {
            instrument_HGHT_recs.push_back(ptr->text);
            return false;//don't add new HGHT record
        }
        else
            return false;//duplicate in instrument_HGHT_recs
    }
    else
    {
        return false;
    }
}

bool IOBfile::uniqifySIGMLabel(IOBSIGM *sigmRecord)
{
    ostringstream newLabel;
    int counter=2;
    std::size_t beg, end;
    map<std::string,IOBSIGM*>::iterator it;

    //see if a SIGM record with this label has already been read
    for(it = SIGMRecords.begin(); it != SIGMRecords.end(); it++)
    {
        if(it->first == sigmRecord->label)
        {
/* if just wanted to eliminate complete duplicates.  Clark directed to keep them in for modifying by section.
            if((it->second->std == sigmRecord->std) &&
               (it->second->ppm == sigmRecord->ppm) &&
               (it->second->at_c == sigmRecord->at_c) &&
               (it->second->fr_c == sigmRecord->fr_c) &&
               (it->second->to_c == sigmRecord->to_c) &&
               (it->second->at_c == sigmRecord->at_c) &&
               (lowerCase(it->second->units) == lowerCase(sigmRecord->units)))
            {
                //record is a duplicate of a previous record and doesn't need to be written out
                return false;
            }
            else//record has unique value, give it a unique label
            {
*/
                //Has there already been a duplicate found for this SIGM label?
                beg = it->second->label.find("UNCR#");
                if(beg != string::npos)
                {
                    end = it->second->label.find("_");
                    counter = atoi(it->second->label.substr(beg+5,end-(beg+5)).c_str());
                    counter += 1;
                }
                //uniqify the record label in the map
                newLabel << "UNCR#" << counter << "_" << sigmRecord->label;
                sigmRecord->label = newLabel.str();
                IOBSIGM *new_record = new IOBSIGM(sigmRecord);
                it->second = new_record;
                return true;
//            }
        }
    }
    //label is unique.  Add it to SIGMRecords
    IOBSIGM *new_record = new IOBSIGM(sigmRecord);
    string new_key(sigmRecord->label);

    SIGMRecords.insert(std::pair<std::string, IOBSIGM*>(new_key, new_record));
    return true;
}

// -----------------------------------------------------------------------------------
// Write all the DocStrings from all the IOB record types, and return as a string
// @return the entire documentation for the IOB records
string IOBfile::asString()
{
   ostringstream oss;
   oss << "# IOB files are ASCII and fixed field-width with one record per line\n";
   oss << "# 3-D Geodetic position\n" << IOBPLH::DocString << "\n\n";
   oss << "# 3-D Cartesian position\n" << IOBXYZ::DocString << "\n\n";
   oss << "# 3-D delta XYZ (Delta)\n" << IOBDXYZ::DocString << "\n\n";
   oss << "# Comments and blank lines\n" << IOBComment::DocString << "\n\n";
   oss << "# #include statements\n" << IOBInclude::DocString << "\n\n";
   oss << "# sigma modifier records\n" << IOBSIGM::DocString << "\n\n";
   oss << "# target/instrument height records\n" << IOBHGHT::DocString << "\n\n";
   oss << "# distance records\n" << IOBDIST::DocString << "\n\n";
   oss << "# horizontal angle records\n" << IOBANGL::DocString << "\n\n";
   oss << "# vertical angle records\n" << IOBVANG::DocString << "\n\n";
   oss << "# zenith angle records\n" << IOBZANG::DocString << "\n\n";
   oss << "# azimuthal angle records\n" << IOBAZIM::DocString << "\n\n";
   oss << "# horizontal direction records\n" << IOBHDIR::DocString << "\n\n";
   oss << "# height difference records\n" << IOBHDIF::DocString << "\n\n";
   oss << "# variance scaling records\n" << IOBVSCA::DocString << "\n\n";
   //Add line for each supported record type with a corresponding class
   string str(oss.str());
   return str;
}

// -----------------------------------------------------------------------------------
string IOBfile::getRecordType(string line)
{
    string word;

    if(line.length() < 5)
        word = line;
    else
        word = line.substr(1,4);
    stripTrailing(word,' ');
    stripLeading(word,' ');
    return word;
}

// -----------------------------------------------------------------------------------
string IOBfile::getMatrixForm(string line)
{
    string word;

    word = line.substr(9,4);
    stripTrailing(word,' ');
    stripLeading(word,' ');
    return word;
}

// -----------------------------------------------------------------------------------
//NOTE: If we used C++11 this could be done with one line
//std::all_of(line.begin(),line.end(),isspace)
bool IOBfile::isAllWhiteSpace(string line)
{
    for(int i=0;i<line.length();i++)
        if(!isspace(line.at(i)))
            return false;
    return true;
}

//TBD: This is buggy.  It will eliminate comments like "* 3DC XYZ record..."
bool IOBfile::checkForCommentedIOBRecord(string &line)
{
    std::string dumdum[] = {"3DC","XYZ","PLH","AZIM","GAZI","ANGL","DIR","DSET",
                            "VANG","GVAN","ZANG","GZAN","DIST","3DD","DXYZ",
                            "ELEM","OHDF","EHDF","HI","HT","VSCA","SIGM","COV",
                            "CORR"};
    const std::set<std::string> TYPES(dumdum, dumdum + sizeof(dumdum) / sizeof(dumdum[0]));
    std::string word;
    string modLine = line;

    if(line[0]==' ') return false;
    if(isAllWhiteSpace(line)) return false;
    if(line.size() < 5) return false;
    if(gnsstk::StringUtils::lowerCase(line.substr(0,8))==string("#include")) return false;
    if(gnsstk::StringUtils::lowerCase(line.substr(1,8))==string("#include"))//*#include
    {
        modLine = modLine.substr(1);
        line = modLine;
        return true;
    }
    if(gnsstk::StringUtils::lowerCase(line.substr(1,7))==string("include"))//GeoLab comments out #include as *include
    {
        line.at(0) = '#';
        return true;
    }

    modLine = line;
    if(modLine.at(1) != ' ')
    {
        modLine[0] = ' ';//see if a comment character was placed in the 1st column
        word = getRecordType(modLine);
        if(TYPES.find(word) != TYPES.end())
        {
            line = modLine;
            return true;
        }
    }
    modLine = line;
    modLine = modLine.substr(1);//see if a comment character was inserted into the 1st column
    word = getRecordType(modLine);
    if(TYPES.find(word) != TYPES.end())
    {
        line = modLine;
        return true;
    }

    return false;
}

//Check if all measurement and include records have the same
//VSCA tag.  If so, remove those tags from the records and
//return that tag.
string IOBfile::VSCACleanUp(vector<IOBrecord *>& recPtrs)
{
    string VSCAReturnTag = string("");
    string tagsString = string("");
    bool oneSansVSCA = false;//checks for some with tags and some without

    //loop over all records and look for VSCA tags
    for(int i=0;i<recPtrs.size();i++)
    {
        if(!recPtrs.at(i)->isCommented)
        {
            IOBtype recType = recPtrs.at(i)->getRecType();
            if(recType == IOBtype::ANGL)
            {
                IOBANGL *iobRecord = static_cast<IOBANGL*>(recPtrs.at(i));
                tagsString = iobRecord->tags;
            }
            else if(recType == IOBtype::AZIM)
            {
                IOBAZIM *iobRecord = static_cast<IOBAZIM*>(recPtrs.at(i));
                tagsString = iobRecord->tags;
            }
            else if(recType == IOBtype::DIST)
            {
                IOBDIST *iobRecord = static_cast<IOBDIST*>(recPtrs.at(i));
                tagsString = iobRecord->tags;
            }
            else if(recType == IOBtype::DXYZ)
            {
                IOBDXYZ *iobRecord = static_cast<IOBDXYZ*>(recPtrs.at(i));
                tagsString = iobRecord->tags;
            }
            else if(recType == IOBtype::HDIF)
            {
                IOBHDIF *iobRecord = static_cast<IOBHDIF*>(recPtrs.at(i));
                tagsString = iobRecord->tags;
            }
            else if(recType == IOBtype::PLH)
            {
                IOBPLH *iobRecord = static_cast<IOBPLH*>(recPtrs.at(i));
                tagsString = iobRecord->tags;
            }
            else if(recType == IOBtype::VANG)
            {
                IOBVANG *iobRecord = static_cast<IOBVANG*>(recPtrs.at(i));
                tagsString = iobRecord->tags;
            }
            else if(recType == IOBtype::XYZ)
            {
                IOBXYZ *iobRecord = static_cast<IOBXYZ*>(recPtrs.at(i));
                tagsString = iobRecord->tags;
            }
            else if(recType == IOBtype::ZANG)
            {
                IOBZANG *iobRecord = static_cast<IOBZANG*>(recPtrs.at(i));
                tagsString = iobRecord->tags;
            }
            else if(recType == IOBtype::HDIR)
            {
                IOBHDIR *iobRecord = static_cast<IOBHDIR*>(recPtrs.at(i));
                tagsString = iobRecord->DGRP_record;
            }
            else if(recType == IOBtype::Include)
            {
                IOBInclude *iobRecord = static_cast<IOBInclude*>(recPtrs.at(i));
                if(!iobRecord->VSCATag.empty())
                {
                    if(VSCAReturnTag.empty())//first measurement or include record with a VSCA tag
                    {
                        VSCAReturnTag = iobRecord->VSCATag;
                        continue;
                    }
                    else if(VSCAReturnTag != iobRecord->VSCATag)//found conflicting VSCA tags, can't apply cleanup
                        return string("");
                    else
                        continue;//found another VSCA tag of the type already found
                }
                else
                    oneSansVSCA = true;
            }
            else
                continue;

            //all standard measurement types - find the VSCA tag
            if(tagsString.find(string(" VSCA ")) != string::npos)
            {
                vector<string> words = splitWithDoubleQuotes(tagsString, ' ');
                for(int j=0;j<words.size();j++)
                {
                    if(words.at(j) == string("VSCA"))
                    {
                        if(VSCAReturnTag.empty())//first measurement record with a VSCA tag
                        {
                            if(words.at(j+1) == string("VALUE"))
                            {
                                VSCAReturnTag = words.at(j+1) + string(" ") + words.at(j+2);
                            }
                            else
                                VSCAReturnTag = words.at(j+1);
                            break;
                        }
                        else if(words.at(j+1) == string("VALUE"))
                        {
                            if(VSCAReturnTag != (words.at(j+1) + string(" ") + words.at(j+2)) )
                                return string("");//found conflicting VSCA tags, can't apply cleanup
                        }
                        else
                        {
                            if(VSCAReturnTag != words.at(j+1))
                                return string("");//found conflicting VSCA tags, can't apply cleanup
                        }
                    }
                }
            }
            else
                oneSansVSCA = true;
        }
    }

    if(VSCAReturnTag.empty() || oneSansVSCA)//no variance scaling applied in this include file
        return string("");

    //remove VSCA tags from all measurement and include records
    for(int i=0;i<recPtrs.size();i++)
    {
        IOBtype recType = recPtrs.at(i)->getRecType();
        removeVSCATag(recPtrs.at(i),VSCAReturnTag);
    }

    return VSCAReturnTag;
}

void IOBfile::removeVSCATag(IOBrecord *rec, string VSCAReturnTag)
{
    IOBtype recType = rec->getRecType();

    if(recType == IOBtype::ANGL)
    {
        IOBANGL *iobRecord = static_cast<IOBANGL*>(rec);
        size_t pos = iobRecord->tags.find(string(" VSCA "));
        if(pos != string::npos)
        {
            vector<string> words = splitWithDoubleQuotes(iobRecord->tags, ' ');
            for(int j=0;j<words.size();j++)
            {
                if(words.at(j) == string("VSCA"))
                {
                    iobRecord->tags.erase(pos,VSCAReturnTag.length()+6);
                    break;
                }
            }
        }
    }
    else if(recType == IOBtype::AZIM)
    {
        IOBAZIM *iobRecord = static_cast<IOBAZIM*>(rec);
        size_t pos = iobRecord->tags.find(string(" VSCA "));
        if(pos != string::npos)
        {
            vector<string> words = splitWithDoubleQuotes(iobRecord->tags, ' ');
            for(int j=0;j<words.size();j++)
            {
                if(words.at(j) == string("VSCA"))
                {
                    iobRecord->tags.erase(pos,VSCAReturnTag.length()+6);
                    break;
                }
            }
        }
    }
    else if(recType == IOBtype::DIST)
    {
        IOBDIST *iobRecord = static_cast<IOBDIST*>(rec);
        size_t pos = iobRecord->tags.find(string(" VSCA "));
        if(pos != string::npos)
        {
            vector<string> words = splitWithDoubleQuotes(iobRecord->tags, ' ');
            for(int j=0;j<words.size();j++)
            {
                if(words.at(j) == string("VSCA"))
                {
                    iobRecord->tags.erase(pos,VSCAReturnTag.length()+6);
                    break;
                }
            }
        }
    }
    else if(recType == IOBtype::DXYZ)
    {
        IOBDXYZ *iobRecord = static_cast<IOBDXYZ*>(rec);
        size_t pos = iobRecord->tags.find(string(" VSCA "));
        if(pos != string::npos)
        {
            vector<string> words = splitWithDoubleQuotes(iobRecord->tags, ' ');
            for(int j=0;j<words.size();j++)
            {
                if(words.at(j) == string("VSCA"))
                {
                    iobRecord->tags.erase(pos,VSCAReturnTag.length()+6);
                    break;
                }
            }
        }
    }
    else if(recType == IOBtype::HDIF)
    {
        IOBHDIF *iobRecord = static_cast<IOBHDIF*>(rec);
        size_t pos = iobRecord->tags.find(string(" VSCA "));
        if(pos != string::npos)
        {
            vector<string> words = splitWithDoubleQuotes(iobRecord->tags, ' ');
            for(int j=0;j<words.size();j++)
            {
                if(words.at(j) == string("VSCA"))
                {
                    iobRecord->tags.erase(pos,VSCAReturnTag.length()+6);
                    break;
                }
            }
        }
    }
    else if(recType == IOBtype::PLH)
    {
        IOBPLH *iobRecord = static_cast<IOBPLH*>(rec);
        size_t pos = iobRecord->tags.find(string(" VSCA "));
        if(pos != string::npos)
        {
            vector<string> words = splitWithDoubleQuotes(iobRecord->tags, ' ');
            for(int j=0;j<words.size();j++)
            {
                if(words.at(j) == string("VSCA"))
                {
                    iobRecord->tags.erase(pos,VSCAReturnTag.length()+6);
                    break;
                }
            }
        }
    }
    else if(recType == IOBtype::VANG)
    {
        IOBVANG *iobRecord = static_cast<IOBVANG*>(rec);
        size_t pos = iobRecord->tags.find(string(" VSCA "));
        if(pos != string::npos)
        {
            vector<string> words = splitWithDoubleQuotes(iobRecord->tags, ' ');
            for(int j=0;j<words.size();j++)
            {
                if(words.at(j) == string("VSCA"))
                {
                    iobRecord->tags.erase(pos,VSCAReturnTag.length()+6);
                    break;
                }
            }
        }
    }
    else if(recType == IOBtype::XYZ)
    {
        IOBXYZ *iobRecord = static_cast<IOBXYZ*>(rec);
        size_t pos = iobRecord->tags.find(string(" VSCA "));
        if(pos != string::npos)
        {
            vector<string> words = splitWithDoubleQuotes(iobRecord->tags, ' ');
            for(int j=0;j<words.size();j++)
            {
                if(words.at(j) == string("VSCA"))
                {
                    iobRecord->tags.erase(pos,VSCAReturnTag.length()+6);
                    break;
                }
            }
        }
    }
    else if(recType == IOBtype::ZANG)
    {
        IOBZANG *iobRecord = static_cast<IOBZANG*>(rec);
        size_t pos = iobRecord->tags.find(string(" VSCA "));
        if(pos != string::npos)
        {
            vector<string> words = splitWithDoubleQuotes(iobRecord->tags, ' ');
            for(int j=0;j<words.size();j++)
            {
                if(words.at(j) == string("VSCA"))
                {
                    iobRecord->tags.erase(pos,VSCAReturnTag.length()+6);
                    break;
                }
            }
        }
    }
    else if(recType == IOBtype::HDIR)
    {
        IOBHDIR *iobRecord = static_cast<IOBHDIR*>(rec);
        size_t pos = iobRecord->DGRP_record.find(string(" VSCA "));
        if(pos != string::npos)
        {
            vector<string> words = splitWithDoubleQuotes(iobRecord->DGRP_record, ' ');
            for(int j=0;j<words.size();j++)
            {
                if(words.at(j) == string("VSCA"))
                {
                    iobRecord->DGRP_record.erase(pos,VSCAReturnTag.length()+6);
                    break;
                }
            }
        }
    }
    else if(recType == IOBtype::Include)
    {
        IOBInclude *iobRecord = static_cast<IOBInclude*>(rec);
        iobRecord->VSCATag = string("");
    }
}
