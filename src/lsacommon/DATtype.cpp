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
/// @file DATtype.cpp  'Smart enum' class for record types in *.dat files

#include "DATtype.hpp"

using namespace std;

// -----------------------------------------------------------------------------------
// Static initialization of const std::strings for asString().
// Must parallel enum Types in DATtype.hpp.
const string DATtype::Strings[count] =
  {
    string("Unknown"),
    string("CONFIG"),
    // position
    string("POS"),
    // angular measurements
    string("AZM"),
    string("HAN"),
    string("VAN"),
    string("ZAN"),
    string("DIR"),
    string("DIRSET"),
    // linear measurements
    string("DIS"),
    string("DEL"),
    string("HGT"),               // comma is ok here
   };

// -----------------------------------------------------------------------------------
void DATtype::setDATtype(const Types& sys)
   throw()
{
   if(sys < 0 || sys >= count)
      type = Unknown;
   else
      type = sys;
}

// -----------------------------------------------------------------------------------
void DATtype::fromString(const string str)
   throw()
{
   type = Unknown;
   for(int i=0; i<count; i++) {
      if(Strings[i] == str) {
         type = static_cast<Types>(i);
         break;
      }
   }
}

// -----------------------------------------------------------------------------------
// return true if the given string is one of the DAT types
bool DATtype::isDATtype(const string str) throw()
{
   for(int i=0; i<count; i++)
      if(str == Strings[i]) return true;
   return false;
}

// -----------------------------------------------------------------------------------
ostream& operator<<(ostream os, const DATtype& t)
{
   return os << t.asString();
}

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
