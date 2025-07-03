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
/// @file IOBSIGM.cpp  Class IOBSIGM, data for the *.iob file record sigma modifier record

#include <string>
#include <ostream>
#include <exception>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"
#include "logstream.hpp"

#include "lsaUtils.hpp"
#include "IOBSIGM.hpp"
#include "iobUtils.hpp"

#include "iobconverter.hpp" //for GlobalData

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string IOBSIGM::DocString = std::string("Columns Description\n"
"002-005 SIGM\n"
"007-009 Sigma record identifier (1 to 3 characters)\n"
"010 (blank)\n"
"011-020 Standard deviation of observation (STD)\n"
"022-031 PPM (PPM)\n"
"033-042 At-station centering error (AT_C)\n"
"044-053 From-station centering error (FR_C)\n"
"055-064 To-station centering error (TO_C)\n"
"066-077 Auxiliary parameter name (AUX)\n"
"079-080 Linear unit name\n");

const int IOBSIGM::widths[] = {-1,4,-1,3,-1,10,-1,10,-1,10,-1,10,-1,10,-1,11,-1,2};
//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param line single line read from IOB file
// return true if successful
bool IOBSIGM::fromString(const std::string& line)
{
   vector<string> F;
   std::vector<int> fieldwidths (widths, widths + sizeof(widths)/sizeof(int));
   std::set<int> station_name_indexes;

   station_name_indexes.insert(15);
   if(!IOBRecordParser(fieldwidths,line,F,station_name_indexes))
       return false;
   //handle empty fields
   for(int i=0;i<F.size();i++)
       if(F[i].empty()) F[i] = string("[]");

   //convert vector of strings into the IOB class
   *this = IOBSIGM(F[1],F[2],F[3],F[4],F[5],F[6],F[8]);

   // success
   return true;
}


//------------------------------------------------------------------------------------
std::string IOBSIGM::asLSAString(void) const
{
//## SIGM <tag> <sigma of measurement> <cm|m|km|ft|rad|deg|soa> <PPM> <AT centering error> <FROM centering error> <TO centering error> <cm|m|km|ft>
   std::ostringstream oss;
   std::set<std::string> LUNIT = validLinearUnit();
   GlobalData& GD=GlobalData::Instance();
   std::string unitstr;
   unitstr = lowerCase(units);//map units to lower case

   if(isCommented)
       oss << "#";

   oss << "UNCR " << addQuotes(label) << " " << std;

   //check that the units are supported
   if(LUNIT.find(unitstr) == LUNIT.end())
   {
       pLOGstrm = &GD.ofwarn;
       LOG(INFO) << "Warning - invalid units in record: " << oss.str() << ".  Assigning meters." << endl;
       pLOGstrm = &GD.oflog;
       GD.invalidUnits = true;
       oss << " m" << " " << ppm << " "
           << at_c << " " << fr_c << " " << to_c << " m";
   }
   else
       oss << " " << unitstr << " " << ppm << " "
           << at_c << " " << fr_c << " " << to_c << " " << unitstr;

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
