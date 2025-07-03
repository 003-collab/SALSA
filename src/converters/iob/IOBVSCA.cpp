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
/// @file IOBVSCA.cpp  Class IOBVSCA, data for the *.iob file record variance scaling record

#include <string>
#include <ostream>
#include <exception>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"
#include "logstream.hpp"

#include "lsaUtils.hpp"
#include "IOBVSCA.hpp"
#include "iobUtils.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string IOBVSCA::DocString = std::string("Columns Description\n"
"002-005 VSCA\n"
"007-022 Factor for observation variances\n");

const int IOBVSCA::widths[] = {-1,4,-1,16};
//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param line single line read from IOB file
// param VSCAtagnum int for unique record tag
// return true if successful
bool IOBVSCA::fromString(const std::string& line, const int VSCAtagnum)
{
   vector<string> F;
   std::vector<int> fieldwidths (widths, widths + sizeof(widths)/sizeof(int));
   std::set<int> station_name_indexes;

   if(!IOBRecordParser(fieldwidths,line,F,station_name_indexes))
       return false;

   //convert vector of strings into the IOB class
   *this = IOBVSCA(std::string("VSCA")+gnsstk::StringUtils::asString(VSCAtagnum),F[1]);

   // success
   return true;
}


//------------------------------------------------------------------------------------
std::string IOBVSCA::asLSAString(void) const
{
//## VSCA <tag> scaling
   std::ostringstream oss;

   if(isCommented)
       oss << "#";
   oss << "VSCA " << label << " " << scale_factor;
   if(applyToParent)
       oss << " TOPARENT";

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
