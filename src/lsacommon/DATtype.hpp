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
/// @file DATtype.hpp  Include file for 'smart enum' class of record types
///                    in *.dat files

#ifndef INCLUDE_DAT_RECORD_TYPE
#define INCLUDE_DAT_RECORD_TYPE

#include <iostream>
#include <string>
#include "Exception.hpp"

/// Class DATtype encapsulates record types for *.dat files,
/// including std::string I/O. This is a 'smart enum' class.
class DATtype
{
public:
   /// list of record types that can appear in LSA *.dat files
   enum Types
   {
      // add new systems BEFORE count,
      // then add to Strings[] in DATtype.cpp and make parallel to this enum.

      // Unknown must be first, and must = 0
      Unknown = 0,         ///< unknown record type
      /// configuration, not really a *.dat file record, but combines data
      /// from all the config records: TITLE DIM PREC CONV
      CONFIG,
      // position
      POS,                 ///< 3-D XYZ position
      // angular measurements
      AZM,                 ///< azimuth angle
      HAN,                 ///< horizontal angle
      VAN,                 ///< vertical angle
      ZAN,                 ///< zenith angle
      DIR,                 ///< direction
      DIRSET,              ///< set of directions
      // linear measurements
      DIS,                 ///< distance
      DEL,                 ///< 3-D delta (dx,dy,dz)
      HGT,                 ///< orthometric or geodetic heights
      // count must be last
      count                ///< the number types, not itself a type
   };

   /// Constructor, including empty constructor
   DATtype(Types t = Unknown) throw()
   {
      if(t < 0 || t >= count)
         type = Unknown;
      else
         type = t;
   }

   /// constructor from int
   DATtype(int i) throw()
   {
      if(i < 0 || i >= count)
         type = Unknown;
      else
         type = static_cast<Types>(i);
   }

   // (copy constructor and operator= are defined by compiler)
   
   /// set the record type
   void setDATtype(const Types& t) throw();

   /// get the record type
   Types getDATtype() const throw()
   { return type; }

   /// Return a std::string for each type (these strings are const and static).
   /// @return the std::string
   std::string asString() const throw()
   { return Strings[type]; }

   /// define type based on input string
   /// @param str input string, expected to match output string for given type
   void fromString(const std::string str) throw();

   /// return true if the given string is one of the DAT types
   static bool isDATtype(const std::string str) throw();
   
   /// boolean operator==
   bool operator==(const DATtype& right) const throw()
   { return type == right.type; }

   /// boolean operator< (used by STL to sort)
   bool operator<(const DATtype& right) const throw()
   { return type < right.type; }

   // the rest follow from Boolean algebra...
   /// boolean operator!=
   bool operator!=(const DATtype& right) const throw()
   { return !operator==(right); }

   /// boolean operator>=
   bool operator>=(const DATtype& right) const throw()
   { return !operator<(right); }

   /// boolean operator<=
   bool operator<=(const DATtype& right) const throw()
   { return (operator<(right) || operator==(right)); }

   /// boolean operator>
   bool operator>(const DATtype& right) const throw()
   { return (!operator<(right) && !operator==(right)); }

private:
   /// record type
   Types type;

   /// set of string labels for elements of Types
   static const std::string Strings[];

};   // end class DATtype

/// Write name (asString()) of a DATtype to an output stream.
/// @param os The output stream
/// @param ts The DATtype to be written
/// @return reference to the output stream
std::ostream& operator<<(std::ostream os, const DATtype& ts);

#endif // INCLUDE_DAT_RECORD_TYPE
