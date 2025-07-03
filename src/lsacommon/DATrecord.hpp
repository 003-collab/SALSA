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
/// @file DATrecord.hpp Pure virtual base class encapsulating records in LSA *.dat
///    files. Includes record type, input from string and output strings.

#ifndef DAT_RECORD_INCLUDE
#define DAT_RECORD_INCLUDE

// disable some MSVC compiler warnings
#ifdef _MSC_VER
    #pragma warning(disable:4290)
#endif

#include <string>
#include <vector>
// lsa
#include "DATtype.hpp"

/// Virtual base class encapsulating records in LSA *.dat files.
/// Only member data is the record type (enum DATtype).
/// Declares an interface that includes input from a string (from *.dat files),
/// output to a (*.dat file) string, and output to a human-readable string.
/// It also declares boolean functions that determine the kind of record.
class DATrecord {
protected:
   // member data
   DATtype type;        ///< record type
   std::string dattag;  ///< optional tag from "tag=str" at end of input line
   double datscale;     ///< optional scale from "scale=double" at end of input line

public:

   /// Constructor
   DATrecord(void) : type(DATtype::Unknown), datscale(1.0) { }

   /// set the "tag" dattag
   void setTag(const std::string& tag) { dattag = tag; }

   /// set the scale
   void setScale(const double& sc) { datscale = sc; }

   /// Destructor
   virtual ~DATrecord() { }

   /// Return the record type
   DATtype getRecType(void) const
      { return type; }

   /// Return the tag - this is for PointMeas(Point) only
   std::string getTag(void) const
      { return dattag; }

   /// Return the scale - this is for PointMeas(Point) only
   double getScale(void) const
      { return datscale; }

   /// Return the record type as a string
   std::string asTypeString(void) const
      { return type.asString(); }

   /// Parse a string from a single line in the DAT file. Pure virtual
   /// @return true if successful
   virtual bool fromString(const std::string& line) = 0;

   /// Output as a string (one line) for the DAT file. Pure virtual
   /// @return a string that can be written to a DAT file
   virtual std::string asDATString(void) const = 0;

   /// Output as a readable string. Pure virtual
   /// @return a read-able string describing the object
   virtual std::string asString(const int prec=3, const int width=8) const
      { return std::string("Unknown DATrecord"); }

   /// Enforce a consistent interface for all children. Pure virtual
   /// True for the configuration records (CONFIG, TITLE, DIM, CONV, etc)
   virtual bool isConfig(void) const = 0;

   /// True for the position records (POS)
   virtual bool isPosition(void) const = 0;

   /// True for the measurement records
   virtual bool isMeasurement(void) const = 0;

   /// True for the angle measurements
   virtual bool isAngle(void) const = 0;

   /// True for the linear measurements
   virtual bool isLength(void) const = 0;

}; // end class DATrecord

#endif   // DAT_RECORD_INCLUDE
