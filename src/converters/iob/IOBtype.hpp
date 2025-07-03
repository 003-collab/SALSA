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
/// @file IOBtype.hpp  Include file for 'smart enum' class of record types
///                    in *.IOB files

#ifndef INCLUDE_IOB_RECORD_TYPE
#define INCLUDE_IOB_RECORD_TYPE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <iostream>
#include <string>
#include "Exception.hpp"

/// Class IOBtype encapsulates record types for *.IOB files,
/// including std::string I/O. This is a 'smart enum' class.
class IOBtype
{
public:
   /// list of record types that can appear in LSA *.IOB files
   enum Types
   {
      // add new systems BEFORE count,
      // then add to Strings[] in IOBtype.cpp and make parallel to this enum.

      // Unknown must be first, and must = 0
      Unknown = 0,         ///< unknown record type
      Comment,             ///< includes blank lines
      Include,             ///< #include records
      PLH,                 ///< 3-D geodetic position
      XYZ,                 ///< 3-D cartesian position
      DXYZ,                ///< 3-D cartesian difference
      SIGM,                ///< sigma record modifier
      DIST,                ///< distance between stations
      HGHT,                ///< instrument or target height
      ANGL,                ///< horizontal angle observation
      VANG,                ///< vertical angle observation
      ZANG,                ///< zenith angle observation
      AZIM,                ///< azimuthal angle observation
      HDIR,                ///< horizontal direction observation
      HDIF,                ///< height difference observation
      VSCA,                ///< variance scale factor
      // count must be last
      count                ///< the number types, not itself a type
   };

   /// Constructor, including empty constructor
   IOBtype(Types t = Unknown) throw()
   {
      if(t < 0 || t >= count)
         type = Unknown;
      else
         type = t;
   }

   /// constructor from int
   IOBtype(int i) throw()
   {
      if(i < 0 || i >= count)
         type = Unknown;
      else
         type = static_cast<Types>(i);
   }

   // (copy constructor and operator= are defined by compiler)
   
   /// set the record type
   void setIOBtype(const Types& t) throw();

   /// get the record type
   Types getIOBtype() const throw()
   { return type; }

   /// Return a std::string for each type (these strings are const and static).
   /// @return the std::string
   std::string asString() const throw()
   { return Strings[type]; }

   /// define type based on input string
   /// @param str input string, expected to match output string for given type
   void fromString(const std::string str) throw();
   
   /// boolean operator==
   bool operator==(const IOBtype& right) const throw()
   { return type == right.type; }

   /// boolean operator< (used by STL to sort)
   bool operator<(const IOBtype& right) const throw()
   { return type < right.type; }

   // the rest follow from Boolean algebra...
   /// boolean operator!=
   bool operator!=(const IOBtype& right) const throw()
   { return !operator==(right); }

   /// boolean operator>=
   bool operator>=(const IOBtype& right) const throw()
   { return !operator<(right); }

   /// boolean operator<=
   bool operator<=(const IOBtype& right) const throw()
   { return (operator<(right) || operator==(right)); }

   /// boolean operator>
   bool operator>(const IOBtype& right) const throw()
   { return (!operator<(right) && !operator==(right)); }

private:
   /// record type
   Types type;

   /// set of string labels for elements of Types
   static const std::string Strings[];

};   // end class IOBtype

/// Write name (asString()) of a IOBtype to an output stream.
/// @param os The output stream
/// @param ts The IOBtype to be written
/// @return reference to the output stream
std::ostream& operator<<(std::ostream os, const IOBtype& ts);

#endif // INCLUDE_IOB_RECORD_TYPE
