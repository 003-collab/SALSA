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
/// @file IOBHGHT.cpp  Class IOBHGHT, height record for the *.iob file

#include <string>
#include <ostream>
#include <set>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "lsaUtils.hpp"
#include "iobUtils.hpp" //for validLinearUnit and IOBRecordParser
#include "IOBHGHT.hpp"
#include "LSAConstants.hpp" //for AUTO_HGHT_PREPEND_TAG
#include "iobconverter.hpp" //for GlobalData

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string IOBHGHT::DocString = std::string("Instrument and target heights");
const int IOBHGHT::HT_widths[] = {-1,4,-5,12,-1,10,-1,2};

//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param line single line read from IOB file
// return true if successful
bool IOBHGHT::fromString(const std::string& line, const std::vector<std::string> HGHT_recs)
{
   bool isDuplicate = false;
   if(line.empty()) return false;
   string str(line);// copy const line

   str = parseHGHTrec(str, HGHT_recs, isDuplicate);
   if(str.empty())//unable to parse
       return false;
   else if(isDuplicate)
   {
       text = str;
       return false;
   }
   else
   {
       text = str;
       return true;
   }
}

//------------------------------------------------------------------------------------
std::string IOBHGHT::asLSAString(void) const
{
   std::ostringstream oss;
   
   if(isCommented)
       oss << "#";
   oss << text;

   return oss.str();
}

std::string IOBHGHT::parseHGHTrec(const string& line, const std::vector<std::string> HGHT_recs, bool &isDuplicate)
{
   std::vector<std::string> old_fields;
   int last_block_number = 1;
   ostringstream oss;
   std::vector<int> hfieldwidths (HT_widths, HT_widths + sizeof(HT_widths)/sizeof(int));
   vector<std::string> HT;
   std::set<int> station_name_indexes;
   std::set<std::string> LUNIT = validLinearUnit();
   string label;
   GlobalData& GD=GlobalData::Instance();

   station_name_indexes.insert(3);
   if(!IOBRecordParser(hfieldwidths,line,HT,station_name_indexes))
   {
       pLOGstrm = &GD.ofwarn;
       LOG(INFO) << "Warning - problem converting > " << line << " < to an LSA record.";
       pLOGstrm = &GD.oflog;
       GD.conversionFailures = true;

       return std::string("");
   }

   for(int i=HGHT_recs.size()-1;i>=0;i--) {//look at last HT/HI record for this station
      string tag;
      string station = getStationFromHGHTRecord(HGHT_recs[i],tag);
      if(station == HT[1])
      {
         old_fields = splitWithDoubleQuotes(HGHT_recs[i],' ');
         if(std::fabs(asDouble(HT[2])-asDouble(old_fields[2]))<1e-9)
         {
            isDuplicate = true;
            return HGHT_recs[i];//redundant station/value combination
         }
         else
         {
            std::size_t beg = old_fields[1].find('#');
            std::size_t end = old_fields[1].find('_');
            string numStr = old_fields[1].substr(beg+1,end-beg-1);
            last_block_number = asInt(numStr);
            label = lsa::AUTO_HGHT_PREPEND_TAG + StringUtils::asString(last_block_number + 1) + string("_") + HT[1];
            oss << "HGHT " << addQuotes(label) << " " << HT[2];
            //check that the units are supported
            if(LUNIT.find(lowerCase(HT[3])) == LUNIT.end())
            {
                pLOGstrm = &GD.ofwarn;
                LOG(INFO) << "Warning - invalid units in record: " << oss.str() << ".  Assigning meters." << endl;
                pLOGstrm = &GD.oflog;
                GD.invalidUnits = true;
                oss << " m";
            }
            else
                oss << " " << HT[3];

            return oss.str();
         }
      }
   }

   //First HGHT record for this station
   oss << "HGHT " << addQuotes(lsa::AUTO_HGHT_PREPEND_TAG + string("1_") + HT[1]) << " " << HT[2];
   //check that the units are supported
   if(LUNIT.find(lowerCase(HT[3])) == LUNIT.end())
   {
       pLOGstrm = &GD.ofwarn;
       LOG(INFO) << "Warning - invalid units in record: " << oss.str() << ".  Assigning meters." << endl;
       pLOGstrm = &GD.oflog;
       GD.invalidUnits = true;
       oss << " m";
   }
   else
       oss << " " << HT[3];

   return oss.str();
}

std::string getStationFromHGHTRecord(std::string HGHT_rec, std::string &tag)
{
    tag = splitWithDoubleQuotes(HGHT_rec,' ')[1];
    string station = tag.substr(tag.find('_')+1);
    return station;
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
