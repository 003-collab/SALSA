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
/// @file IOBComment.hpp  Include file for class IOBComment, comment for the *.IOB file

#ifndef LSA_IOB_COMMENT_DATA_INCLUDE
#define LSA_IOB_COMMENT_DATA_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include "Matrix.hpp"
#include "IOBrecord.hpp"

/// Class IOBComment encapsulates GeoLab comments and blank lines.
class IOBComment : public IOBrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving IOB file specification for this record
   static const std::string DocString;
   std::string text;         ///< line content

   // member functions -------------------------------------------------

   /// empty constructor. label is set to "Undef", all coordinates fixed,
   /// coordinates = 0,0,0
   IOBComment() : text(std::string("Comment"))
      { type = IOBtype::Comment; }

   /// destructor
   virtual ~IOBComment() { }

   /// constructor for comment
   /// @param txt    string containing the text for this Comment
   IOBComment(std::string txt)
      : text(txt)
      { type = IOBtype::Comment; }

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @return true if successful
   virtual bool fromString(const std::string& line);

   /// write the object as a 1-line string
   /// @return string, a single line for an LSA file
   virtual std::string asLSAString(void) const;

   // this ends the base class interface
}; // end class IOBPoint

#endif   // LSA_IOB_POINT_DATA_INCLUDE

