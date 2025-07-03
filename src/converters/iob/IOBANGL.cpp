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
/// @file IOBANGL.cpp  Class IOBANGL, data for the *.iob file record ANGL
/// horizontal angle measurement

#include <string>
#include <ostream>
#include <exception>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"
#include "logstream.hpp"

#include "lsaUtils.hpp"
#include "iobUtils.hpp"
#include "IOBHGHT.hpp"
#include "IOBANGL.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string IOBANGL::DocString = std::string("Columns\tDescription\n"
"002-005 ANGL\n"
"007-009 Sigma record identifier (see the SIGM record)\n"
"010 (blank)\n"
"011-022 At-station name\n"
"024-035 From-station name\n"
"037-048 To-station name\n"
"050 Sign (-/+/space) of angle observation\n"
"051-053 Degrees of angle observation\n"
"055-056 Minutes of angle observation\n"
"058-064 Seconds of angle observation\n"
"066-075 Standard deviation of angle observation (seconds)\n"
"If column 010 has an *,\n\n"
"002-005 ANGL\n"
"007-009 Sigma record identifier (see the SIGM record)\n"
"010 * (asterisk)\n"
"011-041 At-station name\n"
"043-073 From-station name\n"
"075-105 To-station name\n"
"107 Sign (-/+/space) of angle observation\n"
"108-110 Degrees of angle observation\n"
"112-113 Minutes of angle observation\n"
"115-121 Seconds of angle observation\n"
"123-132 Standard deviation of angle observation (seconds)\n");



const int IOBANGL::widths[] = {-1,4,-1,3,-1,12,-1,12,-1,12,-1,1,3,-1,2,-1,7,-1,10};
//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param line single line read from IOB file
// param applyVSCA bool on whether or not to apply VSCA
// param VSCAtagnim int for unique VSCA tag
// param target_HGHT_recs vector of strings of current HT records
// return true if successful
bool IOBANGL::fromString(const std::string& line, const bool applyVSCA, const int VSCAtagnum,
                         std::vector<std::string>& HGHT_recs,
                         std::map<std::string,IOBSIGM*> SIGMRecords)
{
   vector<string> F;
   std::vector<int> fieldwidths (widths, widths + sizeof(widths)/sizeof(int));
   vector<string> HT_parts;
   std::string modifier_tags;
   std::string htfrom = "";
   std::string htto = "";
   std::set<int> station_name_indexes;

   station_name_indexes.insert(5);
   station_name_indexes.insert(7);
   station_name_indexes.insert(9);
   if(!IOBRecordParser(fieldwidths,line,F,station_name_indexes))
       return false;
   //deal with missing sigma
   if(F[9].empty()) F[9] = string("0.0");

   //currently no at station height specified on HANG records (Why not?)
   if(HGHT_recs.size()>0) {
      //loop over HGHT records looking for last from station
      for(int i=HGHT_recs.size()-1;i>=0;i--) {
         string tag;
         string station = getStationFromHGHTRecord(HGHT_recs[i],tag);
         if(station == F[3]) {
            htfrom = tag;
            break;
         }
      }
      //loop over HGHT records looking for last to station
      for(int i=HGHT_recs.size()-1;i>=0;i--) {
         string tag;
         string station = getStationFromHGHTRecord(HGHT_recs[i],tag);
         if(station == F[4]) {
            htto = tag;
            break;
         }
      }
   }

   if(!F[1].empty())
   {
       std::map<std::string,IOBSIGM*>::iterator it = SIGMRecords.find(F[1]);
       if(it!=SIGMRecords.end())
           F[1] = it->second->label;
       modifier_tags = std::string("UNCR ") + addQuotes(F[1]);
   }
   else
      modifier_tags = std::string("");
   if(applyVSCA) {
      if(!modifier_tags.empty())
         modifier_tags += std::string(" ");
      modifier_tags += std::string("VSCA VSCA") + gnsstk::StringUtils::asString(VSCAtagnum);
   }

   //TBD: handle extracting whether or not the refraction coefficient is applied
   if(htfrom.empty() && htto.empty()) {
      *this = IOBANGL(F[2],F[3],F[4],F[5]+F[6],F[7],F[8],F[9],modifier_tags);
   }
   else {
      *this = IOBANGL(F[2],F[3],F[4],F[5]+F[6],F[7],F[8],F[9],htfrom,htto,modifier_tags);
   }

   // success
   return true;
}

//------------------------------------------------------------------------------------
std::string IOBANGL::asLSAString(void) const
{
   std::ostringstream oss;
   std::string degStr(angle_deg), minStr(angle_min), secStr(angle_sec);

   if(isCommented)
       oss << "#";

   //Fix to Bug #1233
   if(angle_deg.empty())
       degStr = "0";
   if(angle_min.empty())
       minStr = "0";
   if(angle_sec.empty())
       secStr = "0";

   oss << "HANG " << addQuotes(from) << " " << addQuotes(at) << " " << addQuotes(to) << " " << degStr
       << " " << minStr << " " << secStr << " DMS " << sigma << " soa";

   if( !htfrom_tag.empty() )
      oss << " HFROM " << addQuotes(htfrom_tag);
   if( !htto_tag.empty() )
      oss << " HTO " << addQuotes(htto_tag);

   if( !tags.empty() )
      oss << " " << tags;

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
