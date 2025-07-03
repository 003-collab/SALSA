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
/// @file IOBVSCA.hpp  Include file for class IOBVSCA, data for the *.IOB file
///                     record VSCA, variance scaling record

#ifndef LSA_IOB_VSCA_INCLUDE
#define LSA_IOB_VSCA_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include "Matrix.hpp"
#include "IOBrecord.hpp"
#include "Exception.hpp"

/// Class IOBXYZ encapsulates data for a 3-D coordinate XYZ position and includes
/// a label and whether or not it is considered fixed.
class IOBVSCA : public IOBrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving IOB file specification for this record
   static const std::string DocString;
   static const int widths[4];

   std::string label;         ///< unique label for this record
   std::string scale_factor;  ///< amount to scale all measurement variances
   bool applyToParent;

   // member functions -------------------------------------------------

   /// empty constructor. label is set to "Undef", all coordinates fixed,
   /// coordinates = 0,0,0
   IOBVSCA() : label(std::string("Undef")),scale_factor(std::string("1.0")),applyToParent(false)
      { type = IOBtype::VSCA; }

   /// destructor
   virtual ~IOBVSCA() { }

   /// constructor from label llh coordinates;
   /// fixtype is set to 111 (fixed)
   /// @param lab    string containing the label for this record
   /// @param s      string for standard deviation
   IOBVSCA(std::string lab, std::string s)
      : label(lab),scale_factor(s),applyToParent(false)
      { type = IOBtype::VSCA; }

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @return true if successful
   virtual bool fromString(const std::string& line) {return false;}

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @param VSCAtagnum int for unique record tag
   /// @return true if successful
   bool fromString(const std::string& line, const int VSCAtagnum);

   /// write the object as a 1-line string
   /// @return string, a single line for an LSA file
   virtual std::string asLSAString(void) const;

   // this ends the base class interface
}; // end class IOBVSCA

#endif   // LSA_IOB_VSCA_INCLUDE

