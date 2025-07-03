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
/// @file IOBInclude.hpp  Include file for class IOBInclude, Include for the *.IOB file

#ifndef LSA_IOB_INCLUDE_DATA_INCLUDE
#define LSA_IOB_INCLUDE_DATA_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include "Matrix.hpp"
#include "IOBrecord.hpp"

/// Class IOBInclude encapsulates GeoLab Includes and blank lines.
class IOBInclude : public IOBrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving IOB file specification for this record
   static const std::string DocString;
   std::string includefile;         ///< file name
   std::string VSCATag;

   // member functions -------------------------------------------------

   /// empty constructor. label is set to "Undef", all coordinates fixed,
   /// coordinates = 0,0,0
   IOBInclude() : includefile(std::string("Undef")), VSCATag(std::string(""))
      { type = IOBtype::Include; }

   /// destructor
   virtual ~IOBInclude() { }

   /// constructor for Include
   /// @param txt    string containing the text for this Include
   IOBInclude(std::string fname)
      : includefile(fname),VSCATag(std::string(""))
      { type = IOBtype::Include; }

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @return true if successful
   virtual bool fromString(const std::string& line);

   /// write the object as a 1-line string
   /// @return string, a single line for an LSA file
   virtual std::string asLSAString(void) const;

   // this ends the base class interface
}; // end class IOBPoint

#endif   // LSA_IOB_INCLUDE_DATA_INCLUDE

