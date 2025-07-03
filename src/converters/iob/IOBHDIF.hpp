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
/// @file IOBHDIF.hpp  Include file for class IOBHDIF, data for the *.IOB file
///                     record HDIF - height difference

#ifndef LSA_IOB_HDIF_INCLUDE
#define LSA_IOB_HDIF_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include <vector>
#include <map>
#include "Matrix.hpp"
#include "IOBrecord.hpp"
#include "IOBSIGM.hpp"
#include "Exception.hpp"

/// Class IOBHDIF encapsulates data for a height difference record
class IOBHDIF : public IOBrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving IOB file specification for this record
   static const std::string DocString;
   static const int widths[16];

   std::string from;         ///< unique label for from station
   std::string to;           ///< unique label for to station
   std::string htdiff;       ///< height difference measurement
   std::string sigma;        ///< standard deviation of measurement
   std::string units;        ///< linear units of height difference
   std::string tags;         ///< tags for relevant SIGM record (if any)

   // member functions -------------------------------------------------

   /// empty constructor. label is set to "Undef", all coordinates fixed,
   /// coordinates = 0,0,0
   IOBHDIF() : from(std::string("Undef")),to(std::string("Undef")),htdiff(std::string("0.0")),
               sigma(std::string("0.0")),units(std::string("m")),tags(std::string(""))
      { type = IOBtype::HDIF; }

   /// destructor
   virtual ~IOBHDIF() { }

   /// constructor
   /// @param fr       string containing label for from station
   /// @param t        string containing label for to station
   /// @param d        string containing height difference measurement
   /// @param dsig     string containing standard deviation of measurement
   /// @param du       string containing linear units of height difference
   /// @param tgs      string containing tags for relevant SIGM record (if any)
   IOBHDIF(std::string fr, std::string t, std::string d, std::string dsig,
           std::string du, std::string tgs)
      : from(fr),to(t),htdiff(d),sigma(dsig),units(du),tags(tgs)
      { type = IOBtype::HDIF; }

   virtual bool fromString(const std::string& line) {return false;}

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @param applyVSCA bool on whether or not to apply VSCA
   /// @param VSCAtagnim int for unique VSCA tag
   /// @return true if successful
   bool fromString(const std::string& line, const bool applyVSCA, const int VSCAtagnum, std::map<std::string,IOBSIGM*> SIGMRecords);

   /// write the object as a 1-line string
   /// @return string, a single line for an LSA file
   virtual std::string asLSAString(void) const;

   // this ends the base class interface
}; // end class IOBHDIF

#endif   // LSA_IOB_HDIF_INCLUDE

