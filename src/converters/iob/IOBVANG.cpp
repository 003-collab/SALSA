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
/// @file IOBVANG.cpp  Class IOBVANG, data for the *.iob file record VANG
/// vertical angle measurement

#include <string>
#include <ostream>
#include <exception>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"
#include "logstream.hpp"

#include "lsaUtils.hpp"
#include "IOBVANG.hpp"
#include "iobUtils.hpp"
#include "IOBHGHT.hpp"
#include "iobconverter.hpp" //for GlobalData

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string IOBVANG::DocString = std::string("Columns\tDescription\n"
"002-005 VANG\n"
"007-009 Sigma record identifier (see the SIGM record)\n"
"010 (blank)\n"
"011-022 From-station name\n"
"024-035 To-station name\n"
"050 Sign (-/+/ ) of vertical angle observation\n"
"051-053 Degrees of vertical angle observation\n"
"055-056 Minutes of vertical angle observation\n"
"058-064 Seconds of vertical angle observation\n"
"066-075 Standard deviation of vertical angle observation\n"
"077-086 Coefficient of refraction\n"
"If column 010 has an *,\n\n"
"002-005 VANG\n"
"007-009 Sigma record identifier (see the SIGM record)\n"
"010 * (asterisk)\n"
"011-041 From-station name\n"
"043-073 To-station name\n"
"075 Sign (-/+/ ) of vertical angle observation\n"
"076-078 Degrees of vertical angle observation\n"
"080-081 Minutes of vertical angle observation\n"
"083-089 Seconds of vertical angle observation\n"
"091-100 Standard deviation of vertical angle observation\n"
"102-111 Coefficient of refraction\n");



const int IOBVANG::widths[] = {-1,4,-1,3,-1,12,-1,12,-14,1,3,-1,2,-1,7,-1,10,-1,10};
//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param line single line read from IOB file
// param applyVSCA bool on whether or not to apply VSCA
// param VSCAtagnim int for unique VSCA tag
// param instrument_HGHT_recs vector of strings of current HI records
// param target_HGHT_recs vector of strings of current HT records
// return true if successful
bool IOBVANG::fromString(const std::string& line, const bool applyVSCA, const int VSCAtagnum,
                         std::vector<std::string>& instrument_HGHT_recs,
                         std::vector<std::string>& target_HGHT_recs,
                         std::map<std::string,IOBSIGM*> SIGMRecords)
{
   vector<string> F;
   std::vector<int> fieldwidths (widths, widths + sizeof(widths)/sizeof(int));
   vector<string> HT_parts;
   std::string htfrom = "";
   std::string htto = "";
   std::string modifier_tags;
   std::set<int> station_name_indexes;

   station_name_indexes.insert(5);
   station_name_indexes.insert(7);
   if(!IOBRecordParser(fieldwidths,line,F,station_name_indexes))
       return false;
   //deal with missing sigma
   if(F[8].empty()) F[8] = string("0.0");

   if(instrument_HGHT_recs.size()>0) {
      //loop over HGHT records looking for last from station
      for(int i=instrument_HGHT_recs.size()-1;i>=0;i--) {
          string tag;
          string station = getStationFromHGHTRecord(instrument_HGHT_recs[i],tag);
         if(station == F[2]) {
            htfrom = tag;
            break;
         }
      }
   }
   if(target_HGHT_recs.size()>0) {
      //loop over HGHT records looking for last to station
      for(int i=target_HGHT_recs.size()-1;i>=0;i--) {
          string tag;
          string station = getStationFromHGHTRecord(target_HGHT_recs[i],tag);
         if(station == F[3]) {
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
   if(std::fabs(asDouble(F[9])) > 1.e-9) {
      if(!modifier_tags.empty())
         modifier_tags += std::string(" ");
      modifier_tags += std::string("REFRACT ") + F[9];
   }
   if(F[0] == std::string("GVAN")) {
      if(!modifier_tags.empty())
         modifier_tags += std::string(" ");
      modifier_tags += std::string("REDUCED");
   }
   if(applyVSCA) {
      if(!modifier_tags.empty())
         modifier_tags += std::string(" ");
      modifier_tags += std::string("VSCA VSCA") + gnsstk::StringUtils::asString(VSCAtagnum);
   }

   if(htfrom.empty() && htto.empty()) {
      *this = IOBVANG(F[2],F[3],F[4]+F[5],F[6],F[7],F[8],modifier_tags);
   }
   else {
      *this = IOBVANG(F[2],F[3],F[4]+F[5],F[6],F[7],F[8],htfrom,htto,modifier_tags);
   }

   // success
   return true;
}

//------------------------------------------------------------------------------------
std::string IOBVANG::asLSAString(void) const
{
   std::ostringstream oss;
   std::string min, sec;
   GlobalData& GD=GlobalData::Instance();
   bool noHFROM=false;
//VANG <FROM> <TO> [*+*|-]<degrees> [min sec DMS] <sigma rad|deg|soa> [HT <fromLabel toLabel| htFrom cm|m|ft htTo cm|m|ft>] [SIGM sigm_tag] [COVM covm_tag] [REFRACT coeff] [REDUCED]
   if(angle_min.empty())
      min = std::string("0");
   else
      min = angle_min;

   if(angle_sec.empty())
      sec = std::string("0.0");
   else
      sec = angle_sec;

   if(isCommented)
       oss << "#";
   oss << "VANG " << addQuotes(from) << " " << addQuotes(to) << " " << angle_deg
       << " " << min << " " << sec << " DMS " << sigma << " soa";

   if( !htfrom_tag.empty())
      oss << " HFROM " << addQuotes(htfrom_tag);
   else
       noHFROM=true;
   if( !htto_tag.empty())
      oss << " HTO " << addQuotes(htto_tag);

   if( !tags.empty() )
      oss << " " << tags;

   if(noHFROM)//Fix for Bug #1361
   {
       pLOGstrm = &GD.ofwarn;
       LOG(INFO) << "Warning - no HFROM specified for record: " << oss.str() << endl;
       pLOGstrm = &GD.oflog;
       GD.missingHFROM = true;
   }

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
