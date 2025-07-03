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
/// @file DATfile.cpp  File I/O for *.dat file: read and parse, write, and fill
///                    DAT* objects.

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "DATfile.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// -----------------------------------------------------------------------------------
// Open and read a .dat file, parse the contents and fill the data vectors.
// param filename file name
// param badrecs return vector<string> of records that could not be parsed
// return the number of records successfully read and parsed, or an error code
// throw if if reading failed
int DATfile::Read(const string& filename, vector<string>& badrecs) throw(Exception)
{
try {
   int i,j;

   // temporarily store pointers to all records
   std::vector<DATrecord *> Records;

   // read and parse the file
   int iret = ReadAndParseDATfile(filename, Records, badrecs);
   if(iret <= 0) return iret;

   const int n(Records.size());
   for(i=0; i<n; i++) {
      DATtype type = Records[i]->getRecType();
      //LOG(INFO) << "Read record: number " << i
      //   << " is " << Records[i]->asDATString()
      //   << (Records[i]->getRecType() == DATtype::DEL ? " its a Delta":"")
      //   << (Records[i]->getRecType() == DATtype::DIS ? " its a Distance":"")
      //   << (Records[i]->getRecType() == DATtype::HAN ? " its an HAngle":"")
      //   << (Records[i]->getRecType() == DATtype::VAN ? " its an VAngle":"")
      //   << (Records[i]->getRecType() == DATtype::AZM ? " its an Azimuth":"")
      //   << (Records[i]->getRecType() == DATtype::POS ? " its a Position":"");
      //   << (Records[i]->getRecType() == DATtype::DIR ? " its a Direction":"");
      //   << (Records[i]->getRecType() == DATtype::DIRSET ? " its a DirSet":"");

      if(type == DATtype::DEL) {
         // downcasting
         // OR DATDelta *ptrdelta = static_cast<DATDelta *>(Records[i]);
         // OR DATDelta *ptrdelta = (DATDelta *)(Records[i]);
         // OR DATDelta delta(*ptrdelta);
         // OR DATDelta delta(*ptrdelta);
         DATDelta delta(*((DATDelta *)(Records[i])));
         Deltas.push_back(delta);
      }
      else if(type == DATtype::HAN) {
         DATHAngle angle(*(static_cast<DATHAngle *>(Records[i])));
         HAngles.push_back(angle);
      }
      else if(type == DATtype::VAN) {
         DATVAngle angle(*(static_cast<DATVAngle *>(Records[i])));
         VAngles.push_back(angle);
      }
      else if(type == DATtype::AZM) {
         DATAzimuth angle(*(static_cast<DATAzimuth *>(Records[i])));
         Azimuths.push_back(angle);
      }
      else if(type == DATtype::DIS) {
         DATDist angle(*(static_cast<DATDist *>(Records[i])));
         Dists.push_back(angle);
      }
      else if(type == DATtype::POS) {
         DATPoint angle(*(static_cast<DATPoint *>(Records[i]))); 
         Points.push_back(angle);
      }
      else if(type == DATtype::HGT) {
         DATHeight ht(*(static_cast<DATHeight *>(Records[i]))); 
         Heights.push_back(ht);
      }
      else if(type == DATtype::ZAN) {
         DATZAngle angle(*(static_cast<DATZAngle *>(Records[i]))); 
         ZAngles.push_back(angle);
      }
      else if(type == DATtype::DIR) {
         DATDir dir(*(static_cast<DATDir *>(Records[i]))); 
         Dirs.push_back(dir);
      }
      else if(type == DATtype::DIRSET) {
         DATDirSet ds(*(static_cast<DATDirSet *>(Records[i]))); 
         DirSets.push_back(ds);
      }
      else if(type == DATtype::CONFIG) {
         Config = DATConfig(*(static_cast<DATConfig *>(Records[i]))); 
      }
      else {
         GNSSTK_THROW(Exception(string("DAT parser returned unknown type: ")
                              + type.asString()));
      }
   }

   return n;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}


// -----------------------------------------------------------------------------------
// Open and read a .dat file, parse the contents and fill the data vectors.
// param filename file name
// param Records vector of all dat records from the dat file
// param badrecs return vector<string> of records that could not be parsed
// return the number of records successfully read and parsed, or an error code
// throw if if reading failed
int DATfile::ReadAndKeepRecords(const string& filename, vector<DATrecord *>& Records, vector<string>& badrecs) throw(Exception)
{
try {
   int i,j;

   // read and parse the file
   int iret = ReadAndParseDATfile(filename, Records, badrecs);
   if(iret <= 0) return iret;

   const int n(Records.size());
   for(i=0; i<n; i++) {
      DATtype type = Records[i]->getRecType();
      //LOG(INFO) << "Read record: number " << i
      //   << " is " << Records[i]->asDATString()
      //   << (Records[i]->getRecType() == DATtype::DEL ? " its a Delta":"")
      //   << (Records[i]->getRecType() == DATtype::DIS ? " its a Distance":"")
      //   << (Records[i]->getRecType() == DATtype::HAN ? " its an HAngle":"")
      //   << (Records[i]->getRecType() == DATtype::VAN ? " its an VAngle":"")
      //   << (Records[i]->getRecType() == DATtype::AZM ? " its an Azimuth":"")
      //   << (Records[i]->getRecType() == DATtype::POS ? " its a Position":"");
      //   << (Records[i]->getRecType() == DATtype::DIR ? " its a Direction":"");
      //   << (Records[i]->getRecType() == DATtype::DIRSET ? " its a DirSet":"");

      if(type == DATtype::DEL) {
         // downcasting
         // OR DATDelta *ptrdelta = static_cast<DATDelta *>(Records[i]);
         // OR DATDelta *ptrdelta = (DATDelta *)(Records[i]);
         // OR DATDelta delta(*ptrdelta);
         // OR DATDelta delta(*ptrdelta);
         DATDelta delta(*((DATDelta *)(Records[i])));
         Deltas.push_back(delta);
      }
      else if(type == DATtype::HAN) {
         DATHAngle angle(*(static_cast<DATHAngle *>(Records[i])));
         HAngles.push_back(angle);
      }
      else if(type == DATtype::VAN) {
         DATVAngle angle(*(static_cast<DATVAngle *>(Records[i])));
         VAngles.push_back(angle);
      }
      else if(type == DATtype::AZM) {
         DATAzimuth angle(*(static_cast<DATAzimuth *>(Records[i])));
         Azimuths.push_back(angle);
      }
      else if(type == DATtype::DIS) {
         DATDist angle(*(static_cast<DATDist *>(Records[i])));
         Dists.push_back(angle);
      }
      else if(type == DATtype::POS) {
         DATPoint angle(*(static_cast<DATPoint *>(Records[i])));
         Points.push_back(angle);
      }
      else if(type == DATtype::HGT) {
         DATHeight ht(*(static_cast<DATHeight *>(Records[i])));
         Heights.push_back(ht);
      }
      else if(type == DATtype::ZAN) {
         DATZAngle angle(*(static_cast<DATZAngle *>(Records[i])));
         ZAngles.push_back(angle);
      }
      else if(type == DATtype::DIR) {
         DATDir dir(*(static_cast<DATDir *>(Records[i])));
         Dirs.push_back(dir);
      }
      else if(type == DATtype::DIRSET) {
         DATDirSet ds(*(static_cast<DATDirSet *>(Records[i])));
         DirSets.push_back(ds);
      }
      else if(type == DATtype::CONFIG) {
         Config = DATConfig(*(static_cast<DATConfig *>(Records[i])));
      }
      else {
         GNSSTK_THROW(Exception(string("DAT parser returned unknown type: ")
                              + type.asString()));
      }
   }

   return n;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}


// -----------------------------------------------------------------------------------
// Open, read and parse a .dat file, filling a vector of pointers to DATrecord.
// param filename file name
// param recPtrs a vector of pointers of 'DATrecord *' containing records
//           read.
// param badrecs return vector<string> of records that could not be parsed
// return number of records stored, or an error code
// throw if file cannot be opened
int DATfile::ReadAndParseDATfile(const string& filename,
                                 vector<DATrecord *>& recPtrs,
                                 std::vector<std::string>& badrecs)
   throw(Exception)
{
try {
   int i,j;
   string::size_type pos1,pos2;
   string line, word, tag, wtstr;
   double scale;

   ifstream istrm;
   istrm.open(filename.c_str(), ios::in);
   if(!istrm.is_open())
      return -1;

   // ?? recPtrs.clear();
   badrecs.clear();

   // read loop
   DATConfig *conptr(NULL);            // keep just one Config record
   while(1) {
      getline(istrm,line);
      // TD need better handling of read failures : return -x
      if(istrm.eof() || !istrm.good()) break;
      stripTrailing(line,"\n");
      stripTrailing(line,"\r");
      strip(line);
      //LOG(INFO) << "DAT file line is >" << line << "<";

      if(line.empty()) continue;
      if(line[0] == '#') continue;

      // DON'T STRIP TRAILING COMMENTS (See BUG #899)
      // strip trailing comments '<RECORD> # this is a comment'
      //pos1 = line.find_first_of("#",0);
      //if(pos1 != string::npos) line.erase(pos1,string::npos);

      // strip off optional tag and/or scale
      pos1 = line.rfind("tag=");       // find last "tag="
      pos2 = line.rfind("scale=");    // find last "scale="
      if(pos1 == string::npos && pos2 == string::npos) {
         tag = string();               // defaults
         scale = 1.0;
      }
      else if(pos2 == string::npos) {  // tag, but no scale
         tag = line.substr(pos1);
         scale = 1.0;
         line.erase(pos1,string::npos);
      }
      else if(pos1 == string::npos) {  // scale, but no tag
         wtstr = line.substr(pos2);
         tag = string();
         line.erase(pos2,string::npos);
      }
      else if(pos1 > pos2) {           // scale, then tag
         tag = line.substr(pos1);
         line.erase(pos1,string::npos);
         wtstr = line.substr(pos2);
         line.erase(pos2,string::npos);
      }
      else {                           // tag, then scale
         wtstr = line.substr(pos2);
         line.erase(pos2,string::npos);
         tag = line.substr(pos1);
         line.erase(pos1,string::npos);
      }

      // clean up
      stripTrailing(line," ");
      if(pos1 != string::npos) {
         tag.erase(0,4);               // erase tag=
         stripTrailing(tag," ");
      }
      if(pos2 != string::npos) {
         wtstr.erase(0,6);             // erase scale=
         stripTrailing(wtstr," ");
         scale = asDouble(wtstr);
      }

      // get the keyword = first word
      word = firstWord(line, ' ');

      DATrecord *dptr;
      bool ok(false), isConfig(false), isExtr(false), isPostProc(false);

      if(word == string("POS")) {
         DATPoint *ptr = new DATPoint();
         ok = ptr->fromString(line);
         if(ok) dptr = dynamic_cast<DATrecord *>(ptr);
      }
      else if(word == string("DEL")) {
         DATDelta *ptr = new DATDelta();
         ok = ptr->fromString(line);
         if(ok) dptr = dynamic_cast<DATrecord *>(ptr);
      }
      else if(word == string("DIR")) {
         DATDir *ptr = new DATDir();
         ok = ptr->fromString(line);
         if(ok) dptr = dynamic_cast<DATrecord *>(ptr);
      }
      else if(word == string("DIRSET")) {
         DATDirSet *ptr = new DATDirSet();
         ok = ptr->fromString(line);
         if(ok) dptr = dynamic_cast<DATrecord *>(ptr);
      }
      else if(word == string("DIS")) {
         DATDist *ptr = new DATDist();
         ok = ptr->fromString(line);
         if(ok) dptr = dynamic_cast<DATrecord *>(ptr);
      }
      else if(word == string("HAN")) {
         DATHAngle *ptr = new DATHAngle();
         ok = ptr->fromString(line);
         if(ok) dptr = dynamic_cast<DATrecord *>(ptr);
      }
      else if(word == string("VAN")) {
         DATVAngle *ptr = new DATVAngle();
         ok = ptr->fromString(line);
         if(ok) dptr = dynamic_cast<DATrecord *>(ptr);
      }
      else if(word == string("AZM")) {
         DATAzimuth *ptr = new DATAzimuth();
         ok = ptr->fromString(line);
         if(ok) dptr = dynamic_cast<DATrecord *>(ptr);
      }
      else if(word == string("HGT")) {
         DATHeight *ptr = new DATHeight();
         ok = ptr->fromString(line);
         if(ok) dptr = dynamic_cast<DATrecord *>(ptr);
      }
      else if(word == string("ZAN")) {
         DATZAngle *ptr = new DATZAngle();
         ok = ptr->fromString(line);
         if(ok) dptr = dynamic_cast<DATrecord *>(ptr);
      }
      else if(word == string("TITLE") ||
              word == string("PREC") ||
              word == string("DIM") ||
              word == string("CONV") ||
              word == string("OUT") ||
              word == string("GEOIDFILE") ||
              word == string("INTERPOLATION") ||
              word == string("COMMENT") ||
              word == string("CONFIDENCE") ||
              word == string("EXTRELVECT") ||
              word == string("HASH"))
      {
         if(!conptr) conptr = new DATConfig();
         ok = conptr->fromString(line);
         isConfig = true;
      }
      else if(word == string("EXTR")) {
         Extract.push_back(line);
         ok = isExtr = true;
      }
      else if(word == string("MEAN")) {
         DerivedPoints.push_back(line);
         ok = isPostProc = true;
      }
      else if(word == string("ENUO")) {
         DerivedPoints.push_back(line);
         ok = isPostProc = true;
      }
      else
         ok = false;

      if(!ok)
         badrecs.push_back(line);
      else if(!isConfig && !isExtr && !isPostProc) {
         dptr->setTag(tag);
         dptr->setScale(scale);          // NB ignored on DIRs (use DIRSET's)
         recPtrs.push_back(dptr);
      }

   }  // end read loop

   istrm.close();

   if(conptr) {
      DATrecord *dptr = dynamic_cast<DATrecord *>(conptr);
      recPtrs.push_back(dptr);
   }

   return recPtrs.size();
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

// -----------------------------------------------------------------------------------
// Open and write a .dat file, writing DAT records for all the data vectors.
// param filename file name
// return the number of records successfully written.
// throw if file cannot be opened
int DATfile::Write(const string& filename) throw(Exception)
{
try {

   return 0;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

// -----------------------------------------------------------------------------------
// Write all the DocStrings from all the DAT record types, and return as a string
// @return the entire documentation for the DAT records
string DATfile::asString(void)
{
   ostringstream oss;
   oss << "# DAT files are ASCII and whitespace-delimited with one record per line\n";
   oss << "# Labels (From, At, To) denote positions (Points or POS records)\n";
   oss << "# Each POS record (Point) must have a unique label; but not all labels"
            << " need have a POS record.\n";
   oss << "# keywords are in ALL CAPS\n";
   oss << "# POS CONS c[c'] constrain the position solution to a plane[line] "
            << "using c,c' = one of{XYZ} OR {NEU} and c != c'\n";
   oss << "# LUNIT means linear unit MM|M|CM|KM|FT\n";
   oss << "# AUNIT means angular unit RAD|DEG|SOA but not DMS\n";
   oss << "# DMS denotes 'deg/min/sec' and follows an angle specified by "
            << "deg(int) min(int) sec(float)\n";
   oss << "# fcsig acsig and tcsig are centering errors on From, At and To stations"
            << " in LUNITs\n";
   oss << "# CORR is optional but must be followed by corrections in the order shown;"
            << " use zero placeholders.\n";
   oss << "# ht[f|t] is height at From|To; target height difference is implemented using From+To\n";
   oss << "# refract is a float refraction, CURV and OHC are flags\n";
   oss << "# Title is for output only\n";
   oss << "# comment lines begin with '#' and are ignored; "
            << "also #-to-EOL is ignored\n";
   oss << "#\n# the Records (each a single line):\n";
   // must do for ALL DATrecord types
   oss << "# Configuration: output precision; problem dimension (2D is XY); "
            << "convergence criteria\n";
   oss << DATConfig::DocString << "\n\n";
   oss << "# position (Point)\n" << DATPoint::DocString << "\n\n";
   oss << "# 3-D delta XYZ (Delta)\n" << DATDelta::DocString << "\n\n";
   oss << "# Distance (Dist)\n" << DATDist::DocString << "\n\n";
   oss << "# Orthometric/Geodetic Ht (Height)\n" << DATHeight::DocString << "\n\n";
   oss << "# Azimuth (Azimuth)\n" << DATAzimuth::DocString << "\n\n";
   oss << "# Horizontal angle (HAngle)\n" << DATHAngle::DocString << "\n\n";
   oss << "# Direction (Dir)\n" << DATDir::DocString << "\n\n";
   oss << "# DirectionSet (DirSet)\n" << DATDirSet::DocString << "\n\n";
   oss << "# Vertical angle (VAngle)\n" << DATVAngle::DocString << "\n\n";
   oss << "# Zenith angle (ZAngle)\n" << DATZAngle::DocString;
   // etc

   string str(oss.str());
   stripTrailing(str,'\n');
   return str;
}

// -----------------------------------------------------------------------------------
string DATfile::exampleOutput(void)
{
   ostringstream oss;

   DATConfig config;
   config.title = "This is the title";
   config.linprec = 4;
   config.angprecM = 3;    // SOA
   config.angprecP = 5;    // SOA
   config.dim = 3;
   config.maxiterations = 10;
   config.noExtRelVect = false;
   config.convergence = 1.e-10;
   oss << config.asDATString() << "\n";

   DATPoint point("APNT",1.,2.,3.,0.01,0,0,0.02,0,0.03);
   oss << point.asDATString() << "\n";

   DATDelta delta("F","T",1.,2.,3.,0.01,0,0,0.02,0,0.03);
   delta.setAddSigma(0.01,true);
   delta.setCenterSigmas(0.01,0.03);
   oss << delta.asDATString() << "\n";

   DATDist dist("F","T",1.0,2.0);
   dist.setAddSigma(0.01,true);
   dist.setCenterSigmas(0.01,0.03);
   oss << dist.asDATString() << "\n";

   DATHeight height("H","B",1.0,0.02);
   height.setAddSigma(0.01,true);
   oss << height.asDATString() << "\n";

   DATAzimuth azimuth("F","T",1.0,0.01);
   azimuth.setAddSigma(0.01);
   azimuth.setCenterSigmas(0.01,0.03);
   oss << azimuth.asDATString() << "\n";

   DATHAngle hangle("F","A","T",1.0,0.01);
   hangle.setAddSigma(0.01);
   hangle.setCenterSigmas(0.01,0.02,0.03);
   oss << hangle.asDATString() << "\n";

   DATVAngle vangle("F","T",1.0,0.01);
   vangle.setAddSigma(0.01);
   vangle.setCenterSigmas(0.01,0.03);
   oss << vangle.asDATString() << "\n";

   DATZAngle zangle("F","T",1.0,0.01);
   zangle.setAddSigma(0.01);
   zangle.setCenterSigmas(0.01,0.03);
   oss << zangle.asDATString() << "\n";

   string str(oss.str());
   stripTrailing(str,'\n');
   return str;
}

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
