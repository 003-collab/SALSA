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
/// @file IOBHDIF.cpp  Class IOBHDIF, data for the *.iob file record HDIF
///                     height difference

#include <string>
#include <ostream>
#include <exception>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"
#include "logstream.hpp"

#include "lsaUtils.hpp"
#include "IOBHDIF.hpp"
#include "iobUtils.hpp"

#include "iobconverter.hpp" //for GlobalData

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string IOBHDIF::DocString = std::string("Columns\tDescription\n"
"002-005 EHDF\n"
"007-009 Sigma record identifier (see SIGM record)\n"
"010 (blank)\n"
"011-022 Station name of from-station\n"
"024-035 Station name of to-station\n"
"050-064 Ellipsoidal height difference observation\n"
"066-075 Standard deviation\n"
"077-091 Distance between stations\n"
"093-094 Linear unit name\n"
"If column 010 has an *,\n\n"
"002-005 EHDF\n"
"007-009 Sigma record identifier (see SIGM record)\n"
"010 * (asterisk)\n"
"011-041 Station name of from-station\n"
"043-073 Station name of to-station\n"
"075-089 Ellipsoidal height difference observation\n"
"091-100 Standard deviation\n"
"102-116 Distance between stations\n"
"118-119 Linear unit name\n\n");

const int IOBHDIF::widths[] = {-1,4,-1,3,-1,12,-1,12,-14,15,-1,10,-1,15,-1,2};
//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param line single line read from IOB file
// param applyVSCA bool on whether or not to apply VSCA
// param VSCAtagnim int for unique VSCA tag
// return true if successful
bool IOBHDIF::fromString(const std::string& line, const bool applyVSCA, const int VSCAtagnum,
                         std::map<std::string,IOBSIGM*> SIGMRecords)
{
   vector<string> F;
   std::vector<int> fieldwidths (widths, widths + sizeof(widths)/sizeof(int));
   std::string modifier_tags;
   std::set<int> station_name_indexes;

   station_name_indexes.insert(5);
   station_name_indexes.insert(7);
   if(!IOBRecordParser(fieldwidths,line,F,station_name_indexes))
       return false;
   //deal with missing sigma
   if(F[5].empty()) F[5] = string("0.0");

   if(F[1].empty())
      modifier_tags = std::string("");
   else
   {
       std::map<std::string,IOBSIGM*>::iterator it = SIGMRecords.find(F[1]);
       if(it!=SIGMRecords.end())
           F[1] = it->second->label;
       modifier_tags = std::string("UNCR ") + addQuotes(F[1]);
   }
   if(F[0] == std::string("EHDF")) {
      if(!modifier_tags.empty())
         modifier_tags += std::string(" ");
      modifier_tags += std::string("REDUCED");
   }
   if(applyVSCA) {
      if(!modifier_tags.empty())
         modifier_tags += std::string(" ");
      modifier_tags += std::string("VSCA VSCA") + gnsstk::StringUtils::asString(VSCAtagnum);
   }

   *this = IOBHDIF(F[2],F[3],F[4],F[5],F[7],modifier_tags);

   // success
   return true;
}

//------------------------------------------------------------------------------------
std::string IOBHDIF::asLSAString(void) const
{
//## HDIF <FROM> <TO> <ht diffs> <sigma> <cm|m|ft> [distance between stations cm|m|km|ft] [SIGM sigm_tag] [COVM covm_tag] [REFRACT coeff] [REDUCED] 

   std::ostringstream oss;
   std::set<std::string> LUNIT = validLinearUnit();
   GlobalData& GD=GlobalData::Instance();
   std::string unitstr;

   //remove leading and trailing spaces
   unitstr = lowerCase(units);//map units to lower case

   if(isCommented)
       oss << "#";

   oss << "HDIF " << addQuotes(from) << " " << addQuotes(to) << " " << htdiff << " " << sigma;

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

   if( !tags.empty() )
      oss << " " << tags;

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
