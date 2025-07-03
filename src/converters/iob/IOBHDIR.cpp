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
/// @file IOBHDIR.cpp  Class IOBHDIR, data for the *.iob file record HDIR
/// horizontal direction measurement

#include <string>
#include <ostream>
#include <exception>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"
#include "logstream.hpp"

#include "lsaUtils.hpp"
#include "IOBHDIR.hpp"
#include "iobUtils.hpp"
#include "IOBHGHT.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string IOBHDIR::DocString = std::string("Columns\tDescription\n"
"DIR\n"
"010 (blank)\n"
"011-022 From-station name\n"
"024-035 To-station name\n"
"050 Sign (-/+/space) of direction observation\n"
"051-053 Degrees of direction observation\n"
"055-056 Minutes of direction observation\n"
"058-064 Seconds of direction observation\n"
"066-075 Standard deviation of direction observation (seconds)\n"
"If column 010 has an *,\n\n"
"002-005 DIR\n"
"010 * (asterisk)\n"
"011-041 From-station name\n"
"043-073 To-station name\n"
"075 Sign (-/+/space) of direction observation\n"
"076-078 Degrees of direction observation\n"
"080-081 Minutes of direction observation\n"
"083-089 Seconds of direction observation\n"
"091-100 Standard deviation of direction observation (seconds)\n");

const int IOBHDIR::widths[] = {-1,4,-5,12,-1,12,-14,1,3,-1,2,-1,7,-1,10};
                               
//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param line single line read from IOB file
// param lastDSETrecord string of line of last DSET in IOB file
// param dsetCounter int for unit DGRP tag
// param wroteDGRP bool if false need to write out DGRP record in asLSAString
// param applyVSCA bool on whether or not to apply VSCA
// param VSCAtagnim int for unique VSCA tag
// param HGHT_recs vector of strings of current HT records
// return true if successful
bool IOBHDIR::fromString(const std::string& line, const std::string& lastDSETrecord,
                         const int dsetCounter, const bool wroteDGRP,
                         const bool lastDSETisCommented, const bool applyVSCA,
                         const int VSCAtagnum, std::vector<std::string>& HGHT_recs,
                         std::map<std::string,IOBSIGM*> SIGMRecords)
{
   vector<string> F;
   std::vector<int> fieldwidths (widths, widths + sizeof(widths)/sizeof(int));
   vector<string> HT_parts;
   std::string htto = "";
   std::string DGRPrec;
   std::set<int> station_name_indexes;

   station_name_indexes.insert(3);
   station_name_indexes.insert(5);
   if(!IOBRecordParser(fieldwidths,line,F,station_name_indexes))
       return false;
   //deal with missing sigma
   if(F[7].empty()) F[7] = string("0.0");

   if(HGHT_recs.size()>0) {
      //loop over HGHT records looking for last from station
      for(int i=HGHT_recs.size()-1;i>=0;i--) {
          string tag;
          string station = getStationFromHGHTRecord(HGHT_recs[i],tag);
         if(station == F[2]) {
            htto = tag;
            break;
         }
      }
   }

   //compose DGRP record
   DGRPrec = "DGRP DGRP#" + asString(dsetCounter) + " " + addQuotes(F[1]);
   if(lastDSETrecord.size() >= 7)
   {
      vector<string>DSETParts = splitWithDoubleQuotes(lastDSETrecord,' ');
      if(DSETParts.size()>1)//watch out for extra spaces bringing size>=7
      {
          if(DSETParts.size()>2)//spaces in label
          {
              std::string quote = "\"";
              std::string fixedLastDSETrecord = lastDSETrecord;
              fixedLastDSETrecord.insert(6,quote);
              fixedLastDSETrecord.append(quote);
              DSETParts = splitWithDoubleQuotes(fixedLastDSETrecord,' ');
          }
          std::string SIGMLabel = DSETParts[1];
          std::map<std::string,IOBSIGM*>::iterator it = SIGMRecords.find(SIGMLabel);
          if(it!=SIGMRecords.end())
              SIGMLabel = it->second->label;

          DGRPrec += " UNCR " + addQuotes(SIGMLabel);
      }
   }
   if(applyVSCA) {
      DGRPrec += std::string(" VSCA VSCA") + gnsstk::StringUtils::asString(VSCAtagnum);
   }   
   
   if(htto.empty()) {
      *this = IOBHDIR(F[1],F[2],F[3]+F[4],F[5],F[6],F[7],DGRPrec,!wroteDGRP,lastDSETisCommented);
   }
   else {
      *this = IOBHDIR(F[1],F[2],F[3]+F[4],F[5],F[6],F[7],htto,DGRPrec,!wroteDGRP,lastDSETisCommented);
   }

   // success
   return true;
}

//------------------------------------------------------------------------------------
std::string IOBHDIR::asLSAString(void) const
{
   std::ostringstream oss;
   std::string group_tag;
   std::string min, sec;
   if(angle_min.empty())
      min = std::string("0");
   else
      min = angle_min;

   if(angle_sec.empty())
      sec = std::string("0.0");
   else
      sec = angle_sec;

   if(writeDGRP)
   {
       if(DGRPisCommented)
           oss << "#";
       oss << DGRP_record << "\n";
   }
   group_tag = splitWithDoubleQuotes(DGRP_record,' ')[1];

   if(isCommented)
       oss << "#";
// HDIR <dir_group> <TO> [*+*|-]<degrees> [min sec DMS] <sigma rad|deg|soa> [HT < toLabel | htTo cm|m|ft>]
   oss << "HDIR " << group_tag << " " << addQuotes(to) << " " << angle_deg << " "
       << min << " " << sec << " DMS " << sigma << " soa";

   if(!htto_tag.empty() )
      oss << " HTO " << addQuotes(htto_tag);

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
