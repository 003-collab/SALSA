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
/// @file IOBXYZ.cpp  Class IOBXYZ, data for the *.iob file record XYZ
///                     3-D LLH coordinate position

#include <string>
#include <ostream>
#include <exception>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"
#include "logstream.hpp"

#include "lsaUtils.hpp"
#include "IOBXYZ.hpp"
#include "iobUtils.hpp"

#include "iobconverter.hpp" //for GlobalData

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string IOBXYZ::DocString = std::string("Columns\tDescription\n"
                                             "002-005\tXYZ\n"
                                             "007\tFlag for fixing ellipsoidal latitude\n"
                                             "008\tFlag for fixing ellipsoidal longitude\n"
                                             "009\tFlag for fixing ellipsoidal height\n"
                                             "010\t(blank)\n"
                                             "011-022\tStation name\n"
                                             "024-041\tX-coordinate\n"
                                             "043-060\tY-coordinate\n"
                                             "062-079\tZ-coordinate\n"
                                             "081-082\tLinear unit name\n"
                                             "If column 010 has an *,\n\n"
                                             "010\t*\n"
                                             "011-041\tStation name\n"
                                             "043-060\tX-coordinate\n"
                                             "062-079\tY-coordinate\n"
                                             "081-098\tZ-coordinate\n"
                                             "100-101\tLinear unit name\n");

const int IOBXYZ::widths[] = {-1,4,-1,1,1,1,-1,12,-1,18,-1,18,-1,18,-1,2};
//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param line single line read from IOB file
// return true if successful
bool IOBXYZ::fromString(const std::string& line)
{
   vector<string> F;
   std::vector<int> fieldwidths (widths, widths + sizeof(widths)/sizeof(int));
   std::set<int> station_name_indexes;

   station_name_indexes.insert(7);
   if(!IOBRecordParser(fieldwidths,line,F,station_name_indexes))
       return false;
   //convert vector of strings into the IOB class
   *this = IOBXYZ(F[4],F[1],F[2],F[3],F[5],F[6],F[7],F[8]);

   // success
   return true;
}

//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param XYZrecord single line containing PLH record from IOB file
// @param crec single line containing COVM record for LSA file
// @param cmat single line containing covariance matrix entries for LSA file
// param applyVSCA bool on whether or not to apply VSCA
// param VSCAtagnim int for unique VSCA tag
// return true if successful
bool IOBXYZ::fromString(const std::string& XYZrecord, const std::string& crec,
                        const std::string cmat, const bool applyVSCA, const int VSCAtagnum)
{
   vector<string> F;
   std::vector<int> fieldwidths (widths, widths + sizeof(widths)/sizeof(int));
   std::set<int> station_name_indexes;

   station_name_indexes.insert(7);
   if(!IOBRecordParser(fieldwidths,XYZrecord,F,station_name_indexes))
       return false;

   std::string modifiers = std::string("");

   if(applyVSCA)
   {
       if(!crec.empty())//VSCA COV#
       {
          std::vector<string> parts = splitWithDoubleQuotes(crec,' ');
          modifiers = "VSCA " + parts[1];
       }
       else
       {
           if(!modifiers.empty())
             modifiers += std::string(" ");
           modifiers += std::string("VSCA VSCA") + gnsstk::StringUtils::asString(VSCAtagnum);
       }
   }
   else if(!crec.empty())//VSCA VALUE ###
   {
       modifiers = crec;
   }

   //convert vector of strings into the IOB class
   *this = IOBXYZ(F[4],F[1],F[2],F[3],F[5],F[6],F[7],F[8],cmat,crec,modifiers);

   // success
   return true;
}

//------------------------------------------------------------------------------------
std::string IOBXYZ::asLSAString(void) const
{
//## POSC <sta_name> [[Fix] | [N&|E&|U]] <X> <Y> <Z> <m|km|ft> [CovXX CovXY CovXZ CovYY CovYZ CovZZ cm|m|ft]
   std::ostringstream oss;
   std::set<std::string> LUNIT = validLinearUnit();
   GlobalData& GD=GlobalData::Instance();
   std::string unitstr;
   std::string fixstr;
   std::string station(label);
   std::string Nfix;
   std::string Efix;
   std::string Ufix;

   station = addQuotes(station);//add quotes around stations with spaces
   unitstr = lowerCase(units);//map units to lower case

   //any character other than 0 or space indicates fixing that coordinate
   if(latfix == string(" ") || latfix == string("0") || latfix.empty()) Nfix = string("");
   else Nfix = string("N");
   if(lonfix == string(" ") || lonfix == string("0") || lonfix.empty()) Efix = string("");
   else Efix = string("E");
   if(htfix == string(" ") || htfix == string("0") || htfix.empty()) Ufix = string("");
   else Ufix = string("U");

   if(Nfix == string("N") && Efix == string("E") && Ufix == string("U")) fixstr = string("Fix");
   else fixstr = Nfix + Efix + Ufix;

   if(isCommented)
       oss << "#";
   oss << "POSC " << station << " ";

   if(fixstr != string("Fix"))
   {
       if(cov_mat != string("Undef"))
           oss << "Con ";
       else
           oss << "Flt ";
   }
   if(!fixstr.empty())
       oss << fixstr << " ";
   oss << x << " " << y << " " << z << " " << unitstr;

   if(cov_mat != string("Undef"))
       oss << prettifyCovMat(cov_mat,isCommented);
   if(tags != string("Undef") && !tags.empty())
   {
       if(isCommented)
           oss << setw(5) << " ...\n" << "#" << tags;
       else
           oss << setw(5) << " ...\n" << tags;
   }
   if(cov_rec != string("Undef") && !cov_rec.empty() && (cov_rec.find("VALUE")==string::npos))//scaling - add COVM record
   {
       if(isCommented)
           oss << "\n" << "#" << cov_rec;
       else
           oss << "\n" << cov_rec;
   }
   //check that the units are supported
   if(LUNIT.find(unitstr) == LUNIT.end())
   {
       pLOGstrm = &GD.ofwarn;
       LOG(INFO) << "Warning - invalid units in record: " << oss.str() << ".";
       pLOGstrm = &GD.oflog;
       GD.invalidUnits = true;
   }

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
