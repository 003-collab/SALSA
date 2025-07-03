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
/// @file IOBInclude.cpp  Class IOBInclude, Include for the *.iob file

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "lsaUtils.hpp"
#include "IOBInclude.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string IOBInclude::DocString = std::string("#include record handling");

//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param line single line read from IOB file
// return true if successful
bool IOBInclude::fromString(const std::string& line)
{
   string str(line);                // copy const line
   std::string::size_type start_pos, end_pos;
   std::string include_iob_name;

   if(str.empty()) return false;

   //take everything between the quotes or <> or the second word
   if((start_pos = str.find("<")) != std::string::npos) {
      ++start_pos;
      end_pos = str.find(">");
      if(end_pos != std::string::npos)
         include_iob_name = str.substr(start_pos, end_pos-start_pos);
   }
   else if((start_pos = str.find("\"")) != std::string::npos) {
      ++start_pos;
      end_pos = str.rfind("\"");
      if(end_pos != std::string::npos)
         include_iob_name = str.substr(start_pos, end_pos-start_pos);
   }
   else {
      start_pos = str.find(" ");
      ++start_pos;
      include_iob_name = stripTrailing(str.substr(start_pos,str.length()-start_pos),'\n');
      include_iob_name = stripTrailing(include_iob_name,'\r');
   }
   *this = IOBInclude(include_iob_name);
//   else return false;

   // success
   return true;
}

//------------------------------------------------------------------------------------
std::string IOBInclude::asLSAString(void) const
{
   std::ostringstream oss;
   
   if(isCommented)
       oss << "#";

   if(includefile.find(" ") != std::string::npos)
      oss << "--include \"" << includefile << ".lsa\"";        
   else
      oss << "--include " << includefile << ".lsa";
   if(!VSCATag.empty())
      oss << " VSCA " << VSCATag;

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
