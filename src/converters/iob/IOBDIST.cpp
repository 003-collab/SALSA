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
/// @file IOBDIST.cpp  Class IOBDIST, data for the *.iob file record DIST
///                     slant distance

#include <string>
#include <ostream>
#include <exception>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"
#include "logstream.hpp"

#include "lsaUtils.hpp"
#include "IOBDIST.hpp"
#include "iobUtils.hpp"
#include "IOBHGHT.hpp"
#include "iobconverter.hpp" //for GlobalData

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string IOBDIST::DocString = std::string("Columns\tDescription\n"
"002-005 DIST\n"
"007-009 Sigma record identifier (see the SIGM record)\n"
"010 (blank)\n"
"011-022 Station name of from-station\n"
"024-035 Station name of to-station\n"
"050-064 Distance observation\n"
"066-075 Standard deviation of distance observation\n"
"077-078 Linear unit name\n"
"If column 010 has an *,\n\n"
"002-005 DIST\n"
"007-009 Sigma record identifier (see the SIGM record)\n"
"010 * (asterisk)\n"
"011-041 Station name of from-station\n"
"043-073 Station name of to-station\n"
"075-089 Distance observation\n"
"091-100 Standard deviation of distance observation\n"
"102-103 Linear unit name\n\n");

const int IOBDIST::widths[] = {-1,4,-1,3,-1,12,-1,12,-14,15,-1,10,-1,2};
//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param line single line read from IOB file
// param applyVSCA bool on whether or not to apply VSCA
// param VSCAtagnim int for unique VSCA tag
// param instrument_HGHT_recs vector of strings of current HI records
// param target_HGHT_recs vector of strings of current HT records
// return true if successful
bool IOBDIST::fromString(const std::string& line, const bool applyVSCA, const int VSCAtagnum,
                         std::vector<std::string>& instrument_HGHT_recs, std::vector<std::string>& target_HGHT_recs,
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
   if(!IOBRecordParser(fieldwidths,line,F,station_name_indexes))
       return false;

   //deal with missing sigma
   if(F[5].empty()) F[5] = string("0.0");

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

   if(F[1].empty())
      modifier_tags = std::string("");
   else
   {
       std::map<std::string,IOBSIGM*>::iterator it = SIGMRecords.find(F[1]);
       if(it!=SIGMRecords.end())
           F[1] = it->second->label;
       modifier_tags = std::string("UNCR ") + addQuotes(F[1]);
   }
   if(applyVSCA) {
      if(modifier_tags.empty())
         modifier_tags = std::string("VSCA VSCA") + gnsstk::StringUtils::asString(VSCAtagnum);
      else
         modifier_tags += std::string(" VSCA VSCA") + gnsstk::StringUtils::asString(VSCAtagnum);
   }

   //TBD: handle extracting whether or not the refraction coefficient is applied
   if(htfrom.empty() && htto.empty()) {
      *this = IOBDIST(F[2],F[3],F[4],F[5],F[6],string("0.0"),modifier_tags);
   }
   else {
      *this = IOBDIST(F[2],F[3],F[4],F[5],F[6],htfrom,htto,string("0.0"),modifier_tags);
   }

   // success
   return true;
}

//------------------------------------------------------------------------------------
std::string IOBDIST::asLSAString(void) const
{
//## DIST <from_sta> <to_sta> <distance> <sigma> <cm|km|m|ft> [hfrom hto cm|m|ft] [REFRACT refract] [optional tags]
   std::ostringstream oss;
   std::set<std::string> LUNIT = validLinearUnit();
   GlobalData& GD=GlobalData::Instance();
   std::string unitstr;
   bool noHFROM=false;

   //remove leading and trailing spaces
   unitstr = lowerCase(units);//map units to lower case

   if(isCommented)
       oss << "#";

   oss << "DIST " << addQuotes(from) << " " << addQuotes(to) << " " << distance << " " << sigma;

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

   if(!htfrom_tag.empty())
   {
      oss << " HFROM " << addQuotes(htfrom_tag);
   }
   else
       noHFROM = true;
   if(!htto_tag.empty() )
      oss << " HTO " << addQuotes(htto_tag);

   if( refract != std::string("0.0") )
      oss << " REFRACT " << refract;

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
