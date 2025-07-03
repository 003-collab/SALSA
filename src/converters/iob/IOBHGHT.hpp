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
/// @file IOBHGHT.hpp  Include file for class IOBHGHT, height record for the *.IOB file

#ifndef LSA_IOB_HGHT_DATA_INCLUDE
#define LSA_IOB_HGHT_DATA_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include "Matrix.hpp"
#include "IOBrecord.hpp"

/// Class IOBComment encapsulates GeoLab comments and blank lines.
class IOBHGHT : public IOBrecord {
public:
   static const int HT_widths[8];
   // member data ------------------------------------------------------
   /// static string giving IOB file specification for this record
   static const std::string DocString;
   std::string text;         ///< line content

   // member functions -------------------------------------------------

   /// empty constructor. test is emtpy
   IOBHGHT() : text(std::string(""))
      { type = IOBtype::HGHT; }

   /// destructor
   virtual ~IOBHGHT() { }

   /// constructor for HGHT
   /// @param txt    string containing the text for this record
   IOBHGHT(std::string txt)
      : text(txt)
      { type = IOBtype::HGHT; }

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @return true if successful
   virtual bool fromString(const std::string& line) {if(line.empty()) {return false;}else{text=line; return true;}}
   bool fromString(const std::string& line, const std::vector<std::string> HGHT_recs);
   std::string parseHGHTrec(const std::string& line, const std::vector<std::string> HGHT_recs, bool &isDuplicate);

   /// write the object as a 1-line string
   /// @return string, a single line for an LSA file
   virtual std::string asLSAString(void) const;

   // this ends the base class interface
}; // end class IOBHGHT

std::string getStationFromHGHTRecord(std::string HGHT_rec, std::string &tag);


#endif   // LSA_IOB_HGHT_DATA_INCLUDE

