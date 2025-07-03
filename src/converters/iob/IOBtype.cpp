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
/// @file IOBtype.cpp  'Smart enum' class for record types in *.IOB files

#include "IOBtype.hpp"

using namespace std;

// -----------------------------------------------------------------------------------
// Static initialization of const std::strings for asString().
// Must parallel enum Types in IOBtype.hpp.
const string IOBtype::Strings[count] =
  {
    string("Unknown"),
    string("Comment"),
    string("Include"),
    // position
    string("PLH"),
    string("XYZ"),
    // linear measurements
    string("DXYZ"),
    string("DIST"),
    string("HDIF"),
    // angular measurements
    string("ANGL"),
    string("VANG"),
    string("ZANG"),
    string("AZIM"),
    string("HDIR"),
    //modifiers
    string("HGHT"),
    string("VSCA"),
    string("SIGM"),
//    string("EHT"),         // comma is ok here
   };

// -----------------------------------------------------------------------------------
void IOBtype::setIOBtype(const Types& sys)
   throw()
{
   if(sys < 0 || sys >= count)
      type = Unknown;
   else
      type = sys;
}

// -----------------------------------------------------------------------------------
void IOBtype::fromString(const string str)
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
ostream& operator<<(ostream os, const IOBtype& t)
{
   return os << t.asString();
}

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
