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
/// @file IOBPLH.cpp  Class IOBPLH, data for the *.iob file record PLH
///                     3-D LLH coordinate position

#include <string>
#include <ostream>
#include <exception>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"
#include "logstream.hpp"

#include "lsaUtils.hpp"
#include "IOBPLH.hpp"
#include "iobUtils.hpp"

#include "iobconverter.hpp" //for GlobalData

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string IOBPLH::DocString = std::string("Columns\tDescription\n"
                                          "002-005\tPLH\n"
                                          "007\tFlag for fixing ellipsoidal latitude\n"
                                          "008\tFlag for fixing ellipsoidal longitude\n"
                                          "009\tFlag for fixing ellipsoidal height\n"
                                          "010\t(blank)\n"
                                          "011-022\tStation name\n"
                                          "024\tIndicator for north (n/N/space) or south (s/S) latitude\n"
                                          "025-027\tDegrees of latitude\n"
                                          "029-030\tMinutes of latitude\n"
                                          "032-040\tSeconds of latitude\n"
                                          "042\tIndicator for east (e/E/space) or west (w/W) longitude\n"
                                          "043-045\tDegrees of longitude\n"
                                          "047-048\tMinutes of longitude\n"
                                          "050-058\tSeconds of longitude\n"
                                          "060-071\tEllipsoidal height\n"
                                          "073-074\tLinear unit name\n\n"
                                          "If column 010 has an *,\n\n"
                                          "010\t*\n"
                                          "011-041\tStation name\n"
                                          "043\tIndicator for north (n/N/space) or south (s/S) latitude\n"
                                          "044-046\tDegrees of latitude\n"
                                          "048-049\tMinutes of latitude\n"
                                          "051-059\tSeconds of latitude\n"
                                          "061\tIndicator for east (e/E/space) or west (w/W) longitude\n"
                                          "062-064\tDegrees of longitude\n"
                                          "066-067\tMinutes of longitude\n"
                                          "069-077\tSeconds of longitude\n"
                                          "079-090\tEllipsoidal height\n"
                                          "092-093\tLinear unit name\n");

const int IOBPLH::widths[] = {-1,4,-1,1,1,1,-1,12,-1,1,3,-1,2,-1,9,-1,1,3,-1,2,-1,9,-1,12,-1,2};
//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param line single line read from IOB file
// return true if successful
bool IOBPLH::fromString(const std::string& line)
{
   vector<string> F;
   std::vector<int> fieldwidths (widths, widths + sizeof(widths)/sizeof(int));
   std::set<int> station_name_indexes;

   station_name_indexes.insert(7);
   if(!IOBRecordParser(fieldwidths,line,F,station_name_indexes))
       return false;

   //convert vector of strings into the IOB class
   *this = IOBPLH(F[4],F[1],F[2],F[3],F[6],F[7],F[8],
         F[10],F[11],F[12],F[5],F[9],F[13],F[14]);

   // success
   if(isValid())
       return true;
   else
       return false;
}

//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param PLHrecord single line containing PLH record from IOB file
// @param crec single line containing COVM record for LSA file
// @param cmat single line containing covariance matrix entries for LSA file
// param applyVSCA bool on whether or not to apply VSCA
// param VSCAtagnim int for unique VSCA tag
   // return true if successful
bool IOBPLH::fromString(const std::string& PLHrecord, const std::string& crec, const std::string cmat,
                        const bool applyVSCA, const int VSCAtagnum)
{
   vector<string> F;
   std::vector<int> fieldwidths (widths, widths + sizeof(widths)/sizeof(int));
   std::set<int> station_name_indexes;

   station_name_indexes.insert(7);
   if(!IOBRecordParser(fieldwidths,PLHrecord,F,station_name_indexes))
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
   *this = IOBPLH(F[4],F[1],F[2],F[3],F[6],F[7],F[8],
         F[10],F[11],F[12],F[5],F[9],F[13],F[14],cmat,crec,modifiers);

   if(isValid())
       return true;
   else
       return false;
}
//------------------------------------------------------------------------------------
std::string IOBPLH::asLSAString(void) const
{
//## POSG <sta_name> [Flt|Con] [[Fix] | [N&|E&|U]] [*+*|-]<degrees | D M S> <N|S> [*+*|-]<degrees | D M S> <E|W> <Height> <cm|m|ft> [CovNN CovNE CovNU CovEE CovEU CovUU] [VSCA vsca_tag]
   std::ostringstream oss;
   std::set<std::string> LUNIT = validLinearUnit();
   GlobalData& GD=GlobalData::Instance();
   std::string fixstr;
   std::string station(label);
   std::string Nfix;
   std::string Efix;
   std::string Ufix;
   std::string posnorthstr;
   std::string poseaststr;
   std::string unitstr;

   station = addQuotes(station);//add quotes around stations with spaces
   unitstr = lowerCase(units);

   //any character other than 0 or space indicates fixing that coordinate
   if(latfix == string(" ") || latfix == string("0") || latfix.empty()) Nfix = string("");
   else Nfix = string("N");
   if(lonfix == string(" ") || lonfix == string("0") || lonfix.empty()) Efix = string("");
   else Efix = string("E");
   if(htfix == string(" ") || htfix == string("0") || htfix.empty()) Ufix = string("");
   else Ufix = string("U");

   if(Nfix == string("N") && Efix == string("E") && Ufix == string("U")) fixstr = string("Fix");
   else fixstr = Nfix + Efix + Ufix;

   //determine which direction is positive longitude/latitude
   if((posnorth == string("n")) || (posnorth == string("N")) || (posnorth == string(" "))) posnorthstr = string("N");
   else if((posnorth == string("s")) || (posnorth == string("S"))) posnorthstr = string("S");
   if((poseast == string("e")) || (poseast == string("E")) || (poseast == string(" "))) poseaststr = string("E");
   else if((poseast == string("w")) || (poseast == string("W"))) poseaststr = string("W");

   if(isCommented)
       oss << "#";
   oss << "POSG " << station << " ";
   if(fixstr != string("Fix"))
   {
       if(cov_mat != string("Undef"))
           oss << "Con ";
       else
           oss << "Flt ";
   }
   if(!fixstr.empty())
       oss << fixstr << " ";

   oss << latdeg << " " << latmin << " " << latsec
       << " " << posnorthstr << " " << londeg << " "
       << lonmin << " " << lonsec << " " << poseaststr
       << " " << height;

   //check that the units are supported
   if(LUNIT.find(unitstr) == LUNIT.end())
   {
       pLOGstrm = &GD.ofwarn;
       LOG(INFO) << "Warning - invalid units in record: " << oss.str() << ".  Assigning meters." << endl;
       pLOGstrm = &GD.oflog;
       GD.invalidUnits = true;
       oss << " m";
   }
   else
       oss << " " << unitstr;

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

   return oss.str();
}

bool IOBPLH::isValid(void)//TBD
{
    //label should contain non-whitespace

    //fix indicators should be space, 0, or 1

    //latdeg should be an integer between +/-90

    //londeg should be an integer between +/-359

    //minutes should integers 0-59

    //seconds should be decimal numbers 0<x<60.0

    //posnorth should be n|N|space|s|S

    //poseast should be e|E|space|w|W

    //ellipsoidal height should be decimal number

    //units should be a valid linear unit

    //cov_mat should have six numbers separated by spaces (or Undef)

    //cov_rec should be ??? (or Undef)

    //tags should contain valid modifier tags followed by non-white-space strings (or Undef)

    return true;
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
