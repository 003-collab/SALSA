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
/// @file IOBrecord.hpp Pure virtual base class encapsulating records in LSA *.dat
///    files. Includes record type, input from string and output strings.


// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <string>
#include <vector>
#include "IOBtype.hpp"

#ifndef IOB_RECORD_INCLUDE
#define IOB_RECORD_INCLUDE

/// Virtual base class encapsulating records in LSA *.dat files.
/// Only member data is the record type (enum IOBtype).
/// Declares an interface that includes input from a string (from *.dat files),
/// output to a (*.dat file) string, and output to a human-readable string.
/// It also declares boolean functions that determine the kind of record.
class IOBrecord {
private:
    std::string sourceFile;
    std::string sourceRecords;

protected:
   // member data
   IOBtype type;

public:
   //member data
   bool isCommented;

   /// Constructor
   IOBrecord(void) : type(IOBtype::Unknown), isCommented(false) { }

   /// Destructor
   virtual ~IOBrecord() { }

   /// Return the record type
   IOBtype getRecType(void) const
      { return type; }

   /// Return the record type as a string
   std::string asTypeString(void) const
      { return type.asString(); }

   /// Parse a string from a single line in the IOB file. Pure virtual
   /// @return true if successful
   virtual bool fromString(const std::string& line) = 0;

   /// Output as a string (one line) for the IOB file. Pure virtual
   /// @return a string that can be written to a IOB file
   virtual std::string asLSAString(void) const = 0;

   /// Store source file name in record in case of error when writing
   /// @param filename - name of the source file
   void setSourceFile(std::string filename) { sourceFile = filename; }

   /// Store IOB records in case of error when writing
   /// @param records - IOB records from which the record was parsed
   void setSourceRecords(std::string records) {sourceRecords = records; }

   /// Retrieve source file name in record in case of error when writing
   /// @return sourceFile - name of the source file
   std::string getSourceFile(void) { return sourceFile; }

   /// Retrieve IOB records in case of error when writing
   /// @return sourceRecords - IOB records from which the record was parsed
   std::string getSourceRecords(void) {return sourceRecords; }
}; // end class IOBrecord

#endif   // IOB_RECORD_INCLUDE
