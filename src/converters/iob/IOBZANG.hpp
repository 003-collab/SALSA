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
/// @file IOBZANG.hpp  Include file for class IOBZANG, data for the *.IOB file
///                     record ZANG - vertical angle measurement

#ifndef LSA_IOB_ZANG_INCLUDE
#define LSA_IOB_ZANG_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include <vector>
#include <map>
#include "Matrix.hpp"
#include "IOBrecord.hpp"
#include "IOBSIGM.hpp"
#include "Exception.hpp"

/// Class IOBZANG encapsulates data for a slant distance record
class IOBZANG : public IOBrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving IOB file specification for this record
   static const std::string DocString;
   static const int widths[19];
   std::string from;         ///< unique label for from station
   std::string to;           ///< unique label for to station
   std::string angle_deg;    ///< angle degrees measurement
   std::string angle_min;    ///< angle minutes measurement
   std::string angle_sec;    ///< angle seconds measurement
   std::string sigma;        ///< standard deviation of measurement in seconds of arc
   std::string htfrom_tag;   ///< tag of HGHT record containing height of from station
   std::string htto_tag;     ///< tag of HGHT record containing height of to station
   std::string tags;         ///< tags for relevant SIGM record (if any)

   // member functions -------------------------------------------------

   /// empty constructor. label is set to "Undef", all coordinates fixed,
   /// coordinates = 0,0,0
   IOBZANG() : from(std::string("Undef")),to(std::string("Undef")),
               angle_deg(std::string("0.0")),angle_min(std::string("0.0")),
               angle_sec(std::string("0.0")),sigma(std::string("0.0")),htfrom_tag(std::string("")),htto_tag(std::string("")),
               tags(std::string(""))
      { type = IOBtype::ZANG; }

   /// destructor
   virtual ~IOBZANG() { }

   /// constructor
   /// @param fr       string containing label for from station
   /// @param t        string containing label for to station
   /// @param d        string containing angle degree measurement
   /// @param m        string containing angle minute measurement
   /// @param s        string containing angle second measurement
   /// @param sig      string containing standard deviation of measurement in seconds of arc
   /// @param htf      string containing height tag of from station
   /// @param htt      string containing height tag of to station
   /// @param tgs      string containing tags for relevant SIGM record (if any)
   IOBZANG(std::string fr, std::string t, std::string d, std::string m,
           std::string s, std::string sig, std::string htf, std::string htt,
           std::string tgs)
          : from(fr),to(t),angle_deg(d),angle_min(m),angle_sec(s),sigma(sig),
           htfrom_tag(htf),htto_tag(htt),tags(tgs)
      { type = IOBtype::ZANG; }

   /// constructor
   /// @param fr       string containing label for from station
   /// @param t        string containing label for to station
   /// @param d        string containing angle degree measurement
   /// @param m        string containing angle minute measurement
   /// @param s        string containing angle second measurement
   /// @param sig      string containing standard deviation of measurement in seconds of arc
   /// @param tgs      string containing tags for relevant SIGM record (if any)
   IOBZANG(std::string fr, std::string t, std::string d, std::string m,
           std::string s, std::string sig, std::string tgs)
          : from(fr),to(t),angle_deg(d),angle_min(m),angle_sec(s),sigma(sig),
            htfrom_tag(std::string("")),htto_tag(std::string("")),tags(tgs)
      { type = IOBtype::ZANG; }

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @param applyVSCA bool on whether or not to apply VSCA
   /// @param VSCAtagnim int for unique VSCA tag
   /// @param instrument_HGHT_recs vector of strings of current HI records
   /// @param target_HGHT_recs vector of strings of current HT records
   /// @return true if successful
   bool fromString(const std::string& line, const bool applyVSCA, const int VSCAtagnum,
                   std::vector<std::string>& instrument_HGHT_recs,
                   std::vector<std::string>& target_HGHT_recs,
                   std::map<std::string,IOBSIGM*> SIGMRecords);

   virtual bool fromString(const std::string& line) {return false;}

   /// write the object as a 1-line string
   /// @return string, a single line for an LSA file
   virtual std::string asLSAString(void) const;

   // this ends the base class interface
}; // end class IOBZANG

#endif   // LSA_IOB_ZANG_INCLUDE

