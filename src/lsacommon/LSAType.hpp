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
/// @file LSAtype.hpp  Include file for 'smart enum' class of record types
///                    in *.LSA files

#ifndef INCLUDE_LSA_TYPE
#define INCLUDE_LSA_TYPE

#include <iostream>
#include <string>
#include "Exception.hpp"

/// Class LSAtype encapsulates record types for *.LSA files,
/// including std::string I/O. This is a 'smart enum' class.
class LSAType
{
public:
   /// list of record types that can appear in LSA *.LSA files
   enum Types
   {
      // add new systems BEFORE count,
      // then add to Strings[] in LSAtype.cpp and make parallel to this enum.

      // Unknown must be first, and must = 0
      Unknown = 0,         ///< unknown record type
      /// configuration, not really a *.LSA file record, but combines data
      /// from all the config records: TITLE DIM PREC CONV
      COMMENT,
      INCLUDE,
      CONFIG,
      // position
      POSC,
      FIRST_POSITION = POSC,
      POSG,
      LAST_POSITION = POSG,
      // angular measurements
      AZIM,
      FIRST_MEASUREMENT = AZIM,
      HANG,
      VANG,
      ZANG,
      DIST,
      DXYZ,
      HDIF,
      HDIR,
      DGRP,
      LAST_MEASUREMENT = DGRP,
      // Modifiers
      UNCR,
      FIRST_MODIFIER = UNCR,
      HGHT,
      VSCA,
      LAST_MODIFIER = VSCA,
      MEAN,
      FIRST_POSTPROCESSED = MEAN,
      ENUO,
      LAST_POSTPROCESSED = ENUO,
      count
   };

   /// Constructor, including empty constructor
   LSAType(Types t = Unknown) throw()
   {
      if(t < 0 || t >= count)
         type = Unknown;
      else
         type = t;
   }

   /// constructor from int
   LSAType(int i) throw()
   {
      if(i < 0 || i >= count)
         type = Unknown;
      else
         type = static_cast<Types>(i);
   }

   // (copy constructor and operator= are defined by compiler)
   
   /// set the record type
   void setLSAtype(const Types& t) throw();

   /// get the record type
   Types getLSAtype() const throw()
   { return type; }

   bool isPosition()      { return FIRST_POSITION      <= type && type <= LAST_POSITION;      }
   bool isMeasurement()   { return FIRST_MEASUREMENT   <= type && type <= LAST_MEASUREMENT;   }
   bool isModifier()      { return FIRST_MODIFIER      <= type && type <= LAST_MODIFIER;      }
   bool isPostProcessed() { return FIRST_POSTPROCESSED <= type && type <= LAST_POSTPROCESSED; }

   /// Return a std::string for each type (these strings are const and static).
   /// @return the std::string
   std::string asString() const throw()
   { return Strings[type]; }

   /// Return a std::string containing a description for each type (these strings are const and static).
   /// @return the std::string
   std::string getDescription() const throw()
   { return Descriptions[type]; }

   /// define type based on input string
   /// @param str input string, expected to match output string for given type
   void fromString(const std::string str) throw();
   
   /// boolean operator==
   bool operator==(const LSAType& right) const throw()
   { return type == right.type; }

   /// boolean operator< (used by STL to sort)
   bool operator<(const LSAType& right) const throw()
   { return type < right.type; }

   // the rest follow from Boolean algebra...
   /// boolean operator!=
   bool operator!=(const LSAType& right) const throw()
   { return !operator==(right); }

   /// boolean operator>=
   bool operator>=(const LSAType& right) const throw()
   { return !operator<(right); }

   /// boolean operator<=
   bool operator<=(const LSAType& right) const throw()
   { return (operator<(right) || operator==(right)); }

   /// boolean operator>
   bool operator>(const LSAType& right) const throw()
   { return (!operator<(right) && !operator==(right)); }

private:
   /// record type
   Types type;

   /// set of string labels for elements of Types
   static const std::string Strings[];

   /// set of string descriptions for elements of Types
   static const std::string Descriptions[];

};   // end class LSAtype

/// Write name (asString()) of a LSAtype to an output stream.
/// @param os The output stream
/// @param ts The LSAtype to be written
/// @return reference to the output stream
std::ostream& operator<<(std::ostream os, const LSAType& ts);

#endif // INCLUDE_LSA_TYPE
